#ifndef COMPOSITOR_HPP
#define COMPOSITOR_HPP

#include <windows.h>
#include <d3d11.h>
#include <dcomp.h>
#include <wrl.h>
#include <dwmapi.h>

#include <QApplication>
#include <QObject>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QQuickView>
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
    RenderControl(QWindow *w) : m_window(w) { }

    QWindow *renderWindow(QPoint *offset) override
    {
        if (offset)
            *offset = QPoint(0, 0);
        return m_window;
    }

private:
    QWindow *m_window;
};

class DirectCompositor : public QObject {
    Q_OBJECT

public:
    DirectCompositor(HWND hWnd);

    bool initDComposition();

    void createDcompositionAngleSurface(QWindow* window, int width, int height);

    bool loadQML(const QUrl &qmlFile, const QSize &size);

private slots:
    void createFbo();
    void destroyFbo();
    void requestUpdate();

    void handleScreenChange();


private:

    QWindow* m_rootwindow;
    HWND m_hwnd;
    Microsoft::WRL::ComPtr<ID3D11Device> m_D3D11Device;

    Microsoft::WRL::ComPtr<IDCompositionDevice> m_DCompDevice;
    Microsoft::WRL::ComPtr<IDCompositionTarget> m_DCompTarget;
    Microsoft::WRL::ComPtr<IDCompositionVisual> rootVisual;

    Microsoft::WRL::ComPtr<IDCompositionVisual> uiVisual;
    Microsoft::WRL::ComPtr<IDCompositionSurface> uiSurface;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> uiDrawTexture;

    QOpenGLContext* m_context = nullptr;
    EGLDisplay m_eglDisplay  = 0;
    EGLContext m_eglCtx  = 0;
    EGLConfig m_eglConfig = 0;

    EGLSurface m_UIsurface;

    //dummy offscreen surface
    QOffscreenSurface* m_uiOffscreen = nullptr;
    RenderControl* m_uiRenderControl = nullptr;

    QQuickWindow* m_uiWindow = nullptr;
    QQmlEngine* m_qmlEngine = nullptr;
    QQmlComponent* m_qmlComponent = nullptr;
    QQuickItem* m_rootItem = nullptr;

};

#endif /* COMPOSITOR_HPP */
