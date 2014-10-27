#include <QApplication>
#include <QQmlApplicationEngine>
#include <QtQml>
#include <QGuiApplication>
#include <QtQuick/QQuickView>
#include "waves.h"

# if defined (Q_OS_IOS)
extern "C" int qtmn (int argc, char * argv [])
#else
int main (int argc, char * argv [])
# endif
{
    qmlRegisterType<Waves>("Waves", 1, 0, "Waves");

    QApplication app(argc, argv);
    QQuickView view;

    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl("qrc:///Waves.qml"));
    view.show();

    return app.exec();
}
