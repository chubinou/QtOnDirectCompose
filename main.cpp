#include <iostream>


#include <QApplication>
#include <QObject>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QOpenGLContext>
#include <QOffscreenSurface>

#include "compositor.hpp"
#include <QLoggingCategory>

#include <QtPlugin>
Q_IMPORT_PLUGIN(QtQuick2Plugin)
Q_IMPORT_PLUGIN(QtQuickControls2Plugin)
//Q_IMPORT_PLUGIN(QtQuickLayoutsPlugin)
//Q_IMPORT_PLUGIN(QtQuick2WindowPlugin)
//Q_IMPORT_PLUGIN(QtQuickTemplates2Plugin)
Q_IMPORT_PLUGIN(QtQmlModelsPlugin)

#ifdef _WIN32
Q_IMPORT_PLUGIN(QWindowsVistaStylePlugin)
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif



int main(int argc, char** argv)
{
    qputenv("QT_LOGGING_TO_CONSOLE", "1");
    //QLoggingCategory::setFilterRules("*.debug=true");

    QApplication::setAttribute( Qt::AA_EnableHighDpiScaling );
    QApplication::setAttribute( Qt::AA_UseOpenGLES ); //force usage of ANGL backend
    QQuickWindow::setDefaultAlphaBuffer(true);

    qWarning()  << "hello hello hello hello";

    QApplication app(argc, argv);

    QWidget qw;
    qw.setAttribute(Qt::WA_NativeWindow);
    qw.setAttribute(Qt::WA_DontCreateNativeAncestors);
    qw.setAttribute(Qt::WA_TranslucentBackground);
    qw.winId();
    qw.show();

    DirectCompositor compositor{ (HWND)qw.winId() };

    //QQuickWidget qml1;
    //qml1.setWindowFlag(Qt::ToolTip);
    //qml1.setSource(QUrl(QStringLiteral("qrc:/main1.qml")));
    //qml1.setResizeMode(QQuickWidget::SizeRootObjectToView);
    //qml1.setClearColor(Qt::transparent);
    //qml1.setAttribute(Qt::WA_TranslucentBackground);
    //qml1.winId();
    //qml1.show();

    compositor.initDComposition();
    compositor.createDcompositionAngleSurface(qw.windowHandle(), 800, 600);

    return app.exec();
}

