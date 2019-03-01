#include "compositor.hpp"
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
    if (S_OK != result)
    {
        qWarning() << msg << "Failed";
        throw ComException(result);
    }
    else
    {
        qWarning() << msg << "OK";
    }
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
    /** Init DComposittion */
    D3D_FEATURE_LEVEL featureLevelSupported;
    HR(D3D11CreateDevice(
           nullptr,    // Adapter
           D3D_DRIVER_TYPE_HARDWARE,
           nullptr,    // Module
           D3D11_CREATE_DEVICE_BGRA_SUPPORT,
           nullptr, 0, // Highest available feature level
           D3D11_SDK_VERSION,
           &m_D3D11Device,
           &featureLevelSupported,    // Actual feature level
           nullptr), "create d3d11 device");

    ComPtr<IDXGIDevice> dxgiDevice;
    m_D3D11Device.As(&dxgiDevice);

    // Create the DirectComposition device object.
    HR(DCompositionCreateDevice(dxgiDevice.Get(),
                                __uuidof(IDCompositionDevice),
                                reinterpret_cast<void**>(m_DCompDevice.GetAddressOf())), "create device" );


    HR(m_DCompDevice->CreateTargetForHwnd((HWND)window->winId(), TRUE, &m_DCompTarget), "create target");

    HR(m_DCompDevice->CreateVisual(&rootVisual), "create root visual");
    HR(m_DCompTarget->SetRoot(rootVisual.Get()), "set root visual");


    m_rootwindow = window;

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


    m_uiOffscreen = new QOffscreenSurface();
    m_uiOffscreen->setFormat(format);;
    m_uiOffscreen->create();

    QPlatformNativeInterface *nativeInterface = QGuiApplication::platformNativeInterface();
    m_eglDisplay = static_cast<EGLDisplay>(nativeInterface->nativeResourceForContext("eglDisplay", m_context));
    m_eglCtx = static_cast<EGLConfig>(nativeInterface->nativeResourceForContext("eglContext", m_context));
    m_eglConfig = static_cast<EGLConfig>(nativeInterface->nativeResourceForContext("eglConfig", m_context));

    qWarning()
            << " display " << m_eglDisplay
            << " ctx " << m_eglCtx
            << " config " << m_eglConfig;

    check(m_context->isValid(), "create is valid");

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
    HR(m_DCompDevice->CreateSurface( width, height,
                                     DXGI_FORMAT_B8G8R8A8_UNORM,
                                     DXGI_ALPHA_MODE_PREMULTIPLIED,
                                     uiSurface.GetAddressOf()), "create surface");

    RECT drawRect;
    POINT update_offset;
    uiSurface->BeginDraw(NULL, IID_PPV_ARGS(uiDrawTexture.GetAddressOf()), &update_offset);
    EGLint pBufferAttributes[] =
    {
        EGL_WIDTH, width,
        EGL_HEIGHT, height,
        EGL_FLEXIBLE_SURFACE_COMPATIBILITY_SUPPORTED_ANGLE, EGL_TRUE,
        //EGL_TEXTURE_TARGET, EGL_TEXTURE_2D,
        //EGL_TEXTURE_FORMAT, EGL_TEXTURE_RGBA,
        EGL_NONE
    };

    EGLClientBuffer buffer = reinterpret_cast<EGLClientBuffer>(uiDrawTexture.Get());
    m_UIsurface = eglCreatePbufferFromClientBuffer(m_eglDisplay, EGL_D3D_TEXTURE_ANGLE, buffer, eglConfig, pBufferAttributes);
    check(m_UIsurface == EGL_NO_SURFACE, "eglCreatePbufferFromClientBuffer");

    m_uiRenderControl = new RenderControl(window);

    m_uiWindow = new QQuickWindow(m_uiRenderControl);
    m_uiWindow->setFormat(format);
    m_uiWindow->setDefaultAlphaBuffer(true);
    //m_uiWindow->setGeometry(0, 0, width, height);
    //m_uiWindow->setResizeMode(QQuickView::SizeRootObjectToView);
    //m_uiWindow->setClearBeforeRendering(true);

    m_qmlEngine = new QQmlEngine();
    if (!m_qmlEngine->incubationController())
        m_qmlEngine->setIncubationController(m_uiWindow->incubationController());

    //put UI in interface
    HR(m_DCompDevice->CreateVisual(&uiVisual), "create ui visual");
    HR(uiVisual->SetContent(uiSurface.Get()), "set ui content");

    HR(rootVisual->AddVisual(uiVisual.Get(), TRUE, NULL), "add ui visual to root");

    HR(m_DCompDevice->Commit(), "commit");



    connect(m_uiWindow, &QQuickWindow::sceneGraphInitialized, this, &DirectCompositor::createFbo);
    connect(m_uiWindow, &QQuickWindow::sceneGraphInvalidated, this, &DirectCompositor::destroyFbo);
    connect(m_uiRenderControl, &QQuickRenderControl::renderRequested, this, &DirectCompositor::requestUpdate);
    connect(m_uiRenderControl, &QQuickRenderControl::sceneChanged, this, &DirectCompositor::requestUpdate);

    //connect(window, &QWindow::screenChanged, this, &DirectCompositor::handleScreenChange);

    loadQML(QUrl(QStringLiteral("qrc:/main2.qml")), QSize(width, height));
    requestUpdate();
}

bool DirectCompositor::loadQML(const QUrl& qmlFile, const QSize &size)
{
    if (m_qmlComponent != nullptr)
        delete m_qmlComponent;
    m_qmlComponent = new QQmlComponent(m_qmlEngine, qmlFile, QQmlComponent::PreferSynchronous);


    if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError &error : errorList)
            qWarning() << error.url() << error.line() << error;
        return false;
    }

    QObject *rootObject = m_qmlComponent->create();
    if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError &error : errorList)
            qWarning() << error.url() << error.line() << error;
        return false;
    }

    m_rootItem = qobject_cast<QQuickItem *>(rootObject);
    if (!m_rootItem) {
        qWarning("run: Not a QQuickItem");
        delete rootObject;
        return false;
    }

    // The root item is ready. Associate it with the window.
    m_rootItem->setParentItem(m_uiWindow->contentItem());

    m_rootItem->setWidth(size.width());
    m_rootItem->setHeight(size.height());

    m_uiWindow->setGeometry(0, 0, size.width(), size.height());

    m_context->makeCurrent(m_uiOffscreen);
    m_uiRenderControl->initialize(m_context);

    return true;
}


void DirectCompositor::createFbo()
{
    qWarning() << "DirectCompositor::createFbo";
    m_uiWindow->setRenderTarget(0, QSize(800, 600));
    //m_uiWindow->setRenderTarget();
}

void DirectCompositor::destroyFbo()
{
    qWarning() << "DirectCompositor::destroyFbo";
}

void DirectCompositor::requestUpdate()
{
    qWarning() << "DirectCompositor::requestUpdate";
    //try {

    m_context->makeCurrent(m_uiOffscreen);
    m_context->functions()->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    eglCheck(eglMakeCurrent(m_eglDisplay, m_UIsurface, m_UIsurface, m_eglCtx), "egl make current");

    //m_context->makeCurrent(m_rootwindow);

    //m_uiRenderControl->polishItems();
    //m_uiRenderControl->sync();
    //m_uiRenderControl->render();
    m_context->functions()->glClearColor(1, 0, 0, 0.5);
    m_context->functions()->glClear(GL_COLOR_BUFFER_BIT);
    //m_context->doneCurrent();
    //m_context->functions()->glFlush(); //present
    //eglCheck(eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, m_eglCtx), "egl make current");
    m_context->doneCurrent();

    HR(uiSurface->EndDraw(), "end draw");
    //HR(m_DCompDevice->Commit(), "commit");

    //} catch (...){
    //    qWarning() << "gota catch them all";
    //}
}

void DirectCompositor::handleScreenChange()
{
    m_uiWindow->setGeometry(0, 0, m_rootwindow->width(), m_rootwindow->height());;
    requestUpdate();
}
