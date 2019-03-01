#include <windows.h>
#include <d3d11.h>
#include <dcomp.h>
#include <wrl.h>
#include <dwmapi.h>
#include <iostream>

//#include <QApplication>
//#include <QObject>
//#include <QQuickWidget>
//#include <QQuickWindow>
//#include <QQuickView>
//#include <QQuickRenderControl>
//#include <QOpenGLContext>
//#include <QOffscreenSurface>
//#include <QOpenGLTexture>

#include <QtANGLE/EGL/eglplatform.h>
#include <QtANGLE/EGL/egl.h>
#include <QtANGLE/EGL/eglext.h>

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
        std::cout  << msg << "Failed" << std::endl;
        throw ComException(result);
    }
    else
    {
        std::cout << msg << "OK" << std::endl;
    }
}

void eglCheck( EGLBoolean result, const char* msg = "" )
{
#define EGL_PRINT_CODE(val) case val: std::cout << msg << "Failed (" << #val << ")" << std::endl; break;
    if (result) {
        std::cout << msg << "OK" << std::endl;
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
            std::cout << msg << "Failed (Unknown)" << std::endl;
            break;
        }
    }
#undef EGL_PRINT_CODE
}


int main()
{
    Microsoft::WRL::ComPtr<ID3D11Device> m_D3D11Device;

    Microsoft::WRL::ComPtr<IDCompositionDevice> m_DCompDevice;
    Microsoft::WRL::ComPtr<IDCompositionTarget> m_DCompTarget;
    Microsoft::WRL::ComPtr<IDCompositionVisual> rootVisual;

    Microsoft::WRL::ComPtr<IDCompositionVisual> uiVisual;
    Microsoft::WRL::ComPtr<IDCompositionSurface> uiSurface;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> uiDrawTexture;

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


    //HR(m_DCompDevice->CreateTargetForHwnd((HWND)window->winId(), TRUE, &m_DCompTarget), "create target");

    HR(m_DCompDevice->CreateVisual(&rootVisual), "create root visual");
    HR(m_DCompTarget->SetRoot(rootVisual.Get()), "set root visual");

}
