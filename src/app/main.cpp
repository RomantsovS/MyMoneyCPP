// MyMoney: the Qt Quick application - composition root of the graphical UI.

#include <cstdlib>

#include <QCoreApplication>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QString>
#include <QUtf8StringView>
#include <QVariantMap>

#include "core/version.h"

// Registers the static QML module MyMoney.UI (see src/ui/CMakeLists.txt).
Q_IMPORT_QML_PLUGIN(MyMoney_UIPlugin)

int main(int argc, char* argv[]) {
  const QGuiApplication app(argc, argv);
  QGuiApplication::setApplicationName(QStringLiteral("MyMoney"));
  QGuiApplication::setApplicationVersion(
      QUtf8StringView(mymoney::core::Version()).toString());

  QQmlApplicationEngine engine;
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);

  // Main.qml declares "required property string appVersion".
  engine.setInitialProperties(
      {{QStringLiteral("appVersion"), QGuiApplication::applicationVersion()}});
  engine.loadFromModule("MyMoney.UI", "Main");

  return QGuiApplication::exec();
}
