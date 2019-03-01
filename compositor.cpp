#include "compositor.hpp"
#include <comdef.h>
#include <QQuickView>
#include <QOpenGLFunctions>
#include <QOpenGLExtraFunctions>
#include <QtQml/QQmlError>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>

#include <QtGui/qpa/qplatformnativeinterface.h>

using namespace Microsoft::WRL;


struct ComException
{
    HRESULT result;
    ComException(HRESULT const value) :
        result(value)
    {}
};
void HR(HRESULT const result, const char* msg = "")
{
#define DCOMPOSITION_PRINT(CODE) case CODE: qWarning() << msg << "Failed (" << #CODE << ")"; break;
    if (S_OK != result)
    {
        _com_error error(result);
        LPCTSTR errorText = error.ErrorMessage();

        //switch (result) {
        //DCOMPOSITION_PRINT(DCOMPOSITION_ERROR_ACCESS_DENIED)
        //DCOMPOSITION_PRINT(DCOMPOSITION_ERROR_SURFACE_BEING_RENDERED)
        //DCOMPOSITION_PRINT(DCOMPOSITION_ERROR_SURFACE_NOT_BEING_RENDERED)
        //DCOMPOSITION_PRINT(DCOMPOSITION_ERROR_WINDOW_ALREADY_COMPOSED)
        //default:
        //    qWarning() << msg << "Failed (Unknown)";
        //    break;
        //}
        qWarning() << msg << "Failed (" << errorText << ")";
        throw ComException(result);
    }
    else
    {
        qWarning() << msg << "OK";
    }
#undef DCOMPOSITION_PRINT
}

void check(bool result, const char* msg = "")
{
    if (result) {
        qWarning() << msg << "OK";
    } else {
        qWarning() << msg << "Failed";
    }
}


void eglCheck( EGLBoolean result, const char* msg = "" )
{
#define EGL_PRINT_CODE(val) case val: qWarning() << msg << "Failed (" << #val << ")"; break;
    if (result) {
        qWarning() << msg << "OK";
    } else {
        switch (eglGetError()) {
        EGL_PRINT_CODE(EGL_SUCCESS)
        EGL_PRINT_CODE(EGL_NOT_INITIALIZED)
        EGL_PRINT_CODE(EGL_BAD_ACCESS)
        EGL_PRINT_CODE(EGL_BAD_ALLOC)
        EGL_PRINT_CODE(EGL_BAD_ATTRIBUTE)
        EGL_PRINT_CODE(EGL_BAD_CONTEXT)
        EGL_PRINT_CODE(EGL_BAD_CONFIG)
        EGL_PRINT_CODE(EGL_BAD_CURRENT_SURFACE)
        EGL_PRINT_CODE(EGL_BAD_DISPLAY)
        EGL_PRINT_CODE(EGL_BAD_SURFACE)
        EGL_PRINT_CODE(EGL_BAD_MATCH)
        EGL_PRINT_CODE(EGL_BAD_PARAMETER)
        EGL_PRINT_CODE(EGL_BAD_NATIVE_PIXMAP)
        EGL_PRINT_CODE(EGL_BAD_NATIVE_WINDOW)
        EGL_PRINT_CODE(EGL_CONTEXT_LOST)
        default:
            qWarning() << msg << "Failed (Unknown)";
            break;
        }
    }
#undef EGL_PRINT_CODE
}

DirectCompositor::DirectCompositor(HWND hWnd)
    : QObject(nullptr) //FIXME
    , m_hwnd(hWnd)
{
}

bool DirectCompositor::initDComposition()
{
    return true;
}

void DirectCompositor::createDcompositionAngleSurface(QWindow* window, int width, int height)
{
    m_rootWindow = window;

    QSurfaceFormat format;
    // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
    format.setDepthBufferSize(8);
    format.setStencilBufferSize(8);
    format.setAlphaBufferSize(8);

    check(window->screen() != nullptr, "window->screen() is not null");
    m_context = new QOpenGLContext(this);
    m_context->setScreen(window->screen());
    m_context->setFormat(format);
    check(m_context->create(), "create context");
    check(m_context->isValid(), "create is valid");

    QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
    m_eglDisplay = static_cast<EGLDisplay>(nativeInterface->nativeResourceForContext("eglDisplay", m_context));
    m_eglCtx = static_cast<EGLConfig>(nativeInterface->nativeResourceForContext("eglContext", m_context));
    m_eglConfig = static_cast<EGLConfig>(nativeInterface->nativeResourceForContext("eglConfig", m_context));

    qWarning()
            << " display " << m_eglDisplay
            << " ctx " << m_eglCtx
            << " config " << m_eglConfig;

    PFNEGLQUERYDISPLAYATTRIBEXTPROC eglQueryDisplayAttribEXT = (PFNEGLQUERYDISPLAYATTRIBEXTPROC)eglGetProcAddress("eglQueryDisplayAttribEXT");
    PFNEGLQUERYDEVICEATTRIBEXTPROC eglQueryDeviceAttribEXT = (PFNEGLQUERYDEVICEATTRIBEXTPROC)eglGetProcAddress("eglQueryDeviceAttribEXT");

    check(eglQueryDisplayAttribEXT != NULL, "eglQueryDisplayAttribEXT != NULL");
    check(eglQueryDeviceAttribEXT != NULL, "eglQueryDeviceAttribEXT != NULL");

    eglCheck(eglQueryDisplayAttribEXT(m_eglDisplay, EGL_DEVICE_EXT, reinterpret_cast<EGLAttrib*>(&m_eglDevice)), "query device");
    eglCheck(eglQueryDeviceAttribEXT(m_eglDevice, EGL_D3D11_DEVICE_ANGLE, reinterpret_cast<EGLAttrib*>(m_D3D11Device.GetAddressOf())), "query D3D device");

    qWarning("m_eglDevice %p, m_D3D11Device %p\n", m_eglDevice, m_D3D11Device.Get());

    ComPtr<IDXGIDevice> dxgiDevice;
    m_D3D11Device.As(&dxgiDevice);

    // Create the DirectComposition device object.
    HR(DCompositionCreateDevice(dxgiDevice.Get(),
                                __uuidof(IDCompositionDevice),
                                reinterpret_cast<void**>(m_DCompDevice.GetAddressOf())), "create device" );


    HR(m_DCompDevice->CreateTargetForHwnd((HWND)window->winId(), TRUE, &m_DCompTarget), "create target");
    HR(m_DCompDevice->CreateVisual(&rootVisual), "create root visual");
    HR(m_DCompTarget->SetRoot(rootVisual.Get()), "set root visual");

    m_uiOffscreen = new QOffscreenSurface();
    m_uiOffscreen->setFormat(format);;
    m_uiOffscreen->create();

    EGLint configureAttributes[] = {
        EGL_RED_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 8,
        EGL_STENCIL_SIZE, 8,
        EGL_BIND_TO_TEXTURE_RGBA, EGL_TRUE,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_NONE
    };
    EGLConfig eglConfig;
    EGLint config_found;
    eglCheck(eglChooseConfig( m_eglDisplay, configureAttributes, &eglConfig, 1, &config_found), "egl choose config");

    qWarning() << "found " << config_found << "configs";

    //create target surface
    qreal dpr = m_rootWindow->devicePixelRatio();
    HR(m_DCompDevice->CreateVirtualSurface( dpr * m_rootWindow->width(), dpr * m_rootWindow->height(),
                                     DXGI_FORMAT_B8G8R8A8_UNORM,
                                     DXGI_ALPHA_MODE_PREMULTIPLIED,
                                     uiSurface.GetAddressOf()), "create surface");

    //put UI in interface
    HR(m_DCompDevice->CreateVisual(&uiVisual), "create ui visual");
    HR(uiVisual->SetContent(uiSurface.Get()), "set ui content");
    //const D2D1_MATRIX_3X2_F matrix =  {
    //    1.f, 0.f,
    //    0.f, 1.f,
    //    0.f, 0.f
    //};
    //HR(m_DCompDevice->CreateMatrixTransform(uiTransform.GetAddressOf()), "create transformation matrix");
    qWarning("transformation matrix %p", uiTransform.Get());
    //HR(uiTransform->SetMatrixElement(0, 0, 2.f), "set matrix value");
    //iTransform->SetMatrix(matrix);
    //HR(uiVisual->SetTransform(uiTransform.Get()), "set Matrix");
    HR(rootVisual->AddVisual(uiVisual.Get(), TRUE, NULL), "add ui visual to root");
    HR(m_DCompDevice->Commit(), "commit");

    m_uiRenderControl = new RenderControl(window);

    m_uiWindow = new QQuickWindow(m_uiRenderControl);
    m_uiWindow->setDefaultAlphaBuffer(true);
    m_uiWindow->setFormat(format);
    //m_uiWindow->setGeometry(0, 0, width, height);
    //m_uiWindow->setResizeMode(QQuickView::SizeRootObjectToView);
    m_uiWindow->setClearBeforeRendering(true);

    m_qmlEngine = new QQmlEngine();
    if (!m_qmlEngine->incubationController())
        m_qmlEngine->setIncubationController(m_uiWindow->incubationController());

    m_updateTimer.setSingleShot(true);
    m_updateTimer.setInterval(5);
    connect(&m_updateTimer, &QTimer::timeout, this, &DirectCompositor::render);


    connect(m_uiWindow, &QQuickWindow::sceneGraphInitialized, this, &DirectCompositor::createFbo);
    connect(m_uiWindow, &QQuickWindow::sceneGraphInvalidated, this, &DirectCompositor::destroyFbo);
    connect(m_uiRenderControl, &QQuickRenderControl::renderRequested, this, &DirectCompositor::requestUpdate);
    connect(m_uiRenderControl, &QQuickRenderControl::sceneChanged, this, &DirectCompositor::requestUpdate);
    //
    ////connect(window, &QWindow::screenChanged, this, &DirectCompositor::handleScreenChange);
    //

    m_rootWindow->installEventFilter(this);

    startQuick(QStringLiteral("qrc:/main2.qml"));
}

void DirectCompositor::run()
{
    disconnect(m_qmlComponent, &QQmlComponent::statusChanged, this, &DirectCompositor::run);

    if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError &error : errorList)
            qWarning() << error.url() << error.line() << error;
        return;
    }

    QObject *rootObject = m_qmlComponent->create();
    if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError &error : errorList)
            qWarning() << error.url() << error.line() << error;
        return;
    }


    m_rootItem = qobject_cast<QQuickItem *>(rootObject);

    if (!m_rootItem) {
        qWarning("run: Not a QQuickItem");
        delete rootObject;
        return;
    }

    // The root item is ready. Associate it with the window.
    //QMLTransformNode* transformNode = new QMLTransformNode(m_uiWindow->contentItem());
    //m_rootItem->setParentItem(transformNode);
    m_rootItem->setParentItem(m_uiWindow->contentItem());

    // Update item and rendering related geometries.
    updateSizes();

    // Initialize the render control and our OpenGL resources.
    QSurface *surface = m_uiOffscreen;
    m_context->makeCurrent(surface);
    m_uiRenderControl->initialize(m_context);
    m_quickInitialized = true;
}

void DirectCompositor::startQuick(const QString &filename)
{
    m_qmlComponent = new QQmlComponent(m_qmlEngine, QUrl(filename));
    if (m_qmlComponent->isLoading())
        connect(m_qmlComponent, &QQmlComponent::statusChanged, this, &DirectCompositor::run);
    else
        run();
}

void DirectCompositor::createFbo()
{
    qWarning() << "DirectCompositor::createFbo";
    QSize size(m_rootWindow->width(), - m_rootWindow->height());
    m_uiWindow->setRenderTarget(0, size);
}

void DirectCompositor::destroyFbo()
{
    qWarning() << "DirectCompositor::destroyFbo";
}

void DirectCompositor::updateSizes()
{
    qWarning() << "updateSizes";
    // Behave like SizeRootObjectToView.
    m_rootItem->setWidth(m_rootWindow->width());
    m_rootItem->setHeight(m_rootWindow->height());
    m_uiWindow->setGeometry(0, 0, m_rootWindow->width(), m_rootWindow->height());
}



static bool g_isDrawing = false;

void DirectCompositor::requestUpdate()
{
    if (!m_updateTimer.isActive())
        m_updateTimer.start();
}

void DirectCompositor::render()
{
    POINT update_offset;

    int realWith = (EGLint)(m_rootWindow->width() * m_rootWindow->devicePixelRatio());
    int realHeight = (EGLint)(m_rootWindow->height() * m_rootWindow->devicePixelRatio());
    uiSurface->Resize(realWith, realHeight);
    HR(uiSurface->BeginDraw(NULL, IID_PPV_ARGS(uiDrawTexture.GetAddressOf()), &update_offset), "BeginDraw");

    EGLClientBuffer buffer = reinterpret_cast<EGLClientBuffer>(uiDrawTexture.Get());
    EGLint pBufferAttributes[] =
    {
        EGL_WIDTH, realWith,
        EGL_HEIGHT, realHeight,
        EGL_FLEXIBLE_SURFACE_COMPATIBILITY_SUPPORTED_ANGLE, EGL_TRUE,
        //EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
        //EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
        EGL_NONE
    };
    m_UIsurface = eglCreatePbufferFromClientBuffer(m_eglDisplay, EGL_D3D_TEXTURE_ANGLE, buffer, m_eglConfig, pBufferAttributes);

    D3D11_TEXTURE2D_DESC desc;
    uiDrawTexture->GetDesc(&desc);;
    qWarning("DirectCompositor::render update offset %lix%li, desc %u:%u, real %ix%i",
             update_offset.x, update_offset.y,
             desc.Width, desc.Height,
             realWith, realHeight);

    //m_uiRenderControl->setOffset(update_offset.x, update_offset.y);
    m_context->makeCurrent(m_uiOffscreen);
    eglCheck(eglMakeCurrent(m_eglDisplay, m_UIsurface, m_UIsurface, m_eglCtx), "egl make current");
    m_context->functions()->glViewport(20, 50, realWith, realHeight);
    m_context->functions()->glScissor(20, 50, realWith, realHeight);

    m_uiRenderControl->polishItems();
    m_uiRenderControl->sync();
    m_uiRenderControl->render();


    m_context->functions()->glFlush(); //present
    m_context->doneCurrent();


    eglDestroySurface(m_eglDisplay, m_UIsurface);
    HR(uiSurface->EndDraw(), "end draw");
    HR(m_DCompDevice->Commit(), "commit");
}

bool DirectCompositor::eventFilter(QObject* object, QEvent* event)
{
    switch (event->type()) {
    case QEvent::Resize:
        qWarning() << ">>>>>>>>>>>> RESIZE <<<<<<<<<<<<";
        updateSizes();
        break;
    case QEvent::Enter:
    case QEvent::Leave:
    case QEvent::HoverEnter:
    case QEvent::HoverLeave:
    case QEvent::HoverMove:
    case QEvent::MouseMove:
        return QApplication::sendEvent(m_uiWindow, event);
    default:
        break;
    }
    return QObject::eventFilter(object, event);
}

void DirectCompositor::handleScreenChange()
{
    m_uiWindow->setGeometry(0, 0, m_rootWindow->width(), m_rootWindow->height());;
    requestUpdate();
}
