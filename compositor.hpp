#ifndef COMPOSITOR_HPP
#define COMPOSITOR_HPP

#include <windows.h>
#include <d3d11.h>
#include <dcomp.h>
#include <wrl.h>
#include <dwmapi.h>

#include <QApplication>
#include <QObject>
#include <QTimer>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QQuickView>
#include <QQuickItem>
#include <QSGTransformNode>
#include <QSGRectangleNode>
#include <QQuickRenderControl>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QOpenGLTexture>

#include <QtANGLE/EGL/egl.h>
#include <QtANGLE/EGL/eglext.h>
#include <QtPlatformHeaders/QEGLNativeContext>

class RenderControl : public QQuickRenderControl
{
    Q_OBJECT
public:
    RenderControl(QWindow *w)
        : m_window(w)
        , offset(0, 0)
    {
    }

    QWindow *renderWindow(QPoint *offset) override
    {
        if (offset) {
            qWarning("get offset");
            offset->setX(offset->x());
            offset->setX(offset->y());
        }
        return m_window;
    }

    void setOffset( int x, int y )
    {
        offset = QPoint(x, y);
    }

private:
    QWindow *m_window;
    QPoint offset;
};

class QMLTransformNode : public QQuickItem {
    Q_OBJECT
public:
    QMLTransformNode(QQuickItem* parent = nullptr)
        : QQuickItem(parent)
    {
        setFlag(QQuickItem::ItemHasContents);
        setTransformOrigin(QQuickItem::Center);
    }

    //Meh, it's working but it doesn't look right
    QSGNode * updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData * nodeData)
    {
        qWarning() << "updatePaintNode " << m_offset.x() << " " << m_offset.y();
        QMatrix4x4 matrix(
             1, 0, 0, m_offset.x(),
             0, -1, 0, height() + m_offset.y(),
             0, 0, 1, 0,
             0, 0, 0, 1
            );
        nodeData ->transformNode->setMatrix(matrix);
        nodeData ->transformNode->markDirty(QSGNode::DirtyMatrix);

        return oldNode;
    }

    void setOffset(int x, int y)
    {
        if (m_offset.x() == x && m_offset.y() == y)
            return;
        m_offset.setX(x);
        m_offset.setY(y);
        update();
    }

    QPoint m_offset;
};


class DirectCompositor : public QObject {
    Q_OBJECT

public:
    DirectCompositor(HWND hWnd);

    bool initDComposition();

    void createDcompositionAngleSurface(QWindow* window, int width, int height);

    void startQuick(const QString &filename);

    void updateSizes();

private slots:
    void createFbo();
    void destroyFbo();
    void requestUpdate();

    void handleScreenChange();

    void run();
    void render();

    bool eventFilter(QObject *object, QEvent *event);

private:

    QWindow* m_rootWindow = nullptr;
    HWND m_hwnd;
    Microsoft::WRL::ComPtr<ID3D11Device> m_D3D11Device;

    Microsoft::WRL::ComPtr<IDCompositionDevice> m_DCompDevice;
    Microsoft::WRL::ComPtr<IDCompositionTarget> m_DCompTarget;
    Microsoft::WRL::ComPtr<IDCompositionVisual> rootVisual;

    Microsoft::WRL::ComPtr<IDCompositionVisual> uiVisual;
    Microsoft::WRL::ComPtr<IDCompositionMatrixTransform> uiTransform;
    Microsoft::WRL::ComPtr<IDCompositionVirtualSurface> uiSurface;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> uiDrawTexture;

    QSize m_surfaceSize;

    QTimer m_updateTimer;

    QOpenGLContext* m_context = nullptr;
    EGLDisplay m_eglDisplay  = 0;
    EGLDeviceEXT m_eglDevice  = 0;
    EGLContext m_eglCtx  = 0;
    EGLConfig m_eglConfig = 0;

    EGLSurface m_UIsurface = 0;

    //dummy offscreen surface
    QOffscreenSurface* m_uiOffscreenSurface = nullptr;
    QWindow* m_uiOffscreenWindow = nullptr;
    RenderControl* m_uiRenderControl = nullptr;

    QQuickWindow* m_uiWindow = nullptr;
    QQmlEngine* m_qmlEngine = nullptr;
    QQmlComponent* m_qmlComponent = nullptr;
    QQuickItem* m_rootItem = nullptr;
    bool m_quickInitialized = false;

    QMLTransformNode* m_transformNode = nullptr;
};

#endif /* COMPOSITOR_HPP */
