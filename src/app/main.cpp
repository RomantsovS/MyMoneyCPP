// MyMoney: the Qt Quick application - composition root of the graphical UI
// (ADR-0016). Opens the database, creates the services and the C++ objects
// QML uses, connects them and loads the QML.

#include <cstdlib>
#include <string>
#include <utility>

#include <QCoreApplication>
#include <QDir>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QStandardPaths>
#include <QString>
#include <QUtf8StringView>
#include <QVariantMap>

#include "core/error.h"
#include "core/version.h"
#include "storage/app_services.h"
#include "storage/sqlite/database.h"
#include "storage/storage.h"
#include "ui/controllers/account_editor.h"
#include "ui/controllers/currency_editor.h"
#include "ui/models/account_list_model.h"
#include "ui/models/currency_list_model.h"
#include "ui/singleton_instance.h"

// Registers the static QML module MyMoney.UI (see src/ui/CMakeLists.txt).
Q_IMPORT_QML_PLUGIN(MyMoney_UIPlugin)

namespace {

using mymoney::ui::AccountEditor;
using mymoney::ui::AccountListModel;
using mymoney::ui::CurrencyEditor;
using mymoney::ui::CurrencyListModel;
using mymoney::ui::SingletonInstance;

// The per-user app data folder: on Windows and Linux the same file as the
// CLI uses (ADR-0014), on Android the app's private folder. No organization
// name is set, so the folder is just "MyMoney".
QString DatabasePath() {
  const QString folder =
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  QDir().mkpath(folder);  // if this fails, opening the database reports it
  return QDir(folder).filePath(QStringLiteral("mymoney.sqlite"));
}

// Loads the QML component `name` of MyMoney.UI and runs the event loop.
int RunQml(const QGuiApplication& app, const char* name,
           const QVariantMap& properties) {
  QQmlApplicationEngine engine;
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      [] { QCoreApplication::exit(EXIT_FAILURE); }, Qt::QueuedConnection);
  engine.setInitialProperties(properties);
  engine.loadFromModule("MyMoney.UI", name);
  return QGuiApplication::exec();
}

}  // namespace

int main(int argc, char* argv[]) {
  const QGuiApplication app(argc, argv);
  QGuiApplication::setApplicationName(QStringLiteral("MyMoney"));
  QGuiApplication::setApplicationVersion(
      QUtf8StringView(mymoney::core::Version()).toString());

  mymoney::core::Result<mymoney::storage::sqlite::Database> db =
      mymoney::storage::OpenStorage(DatabasePath().toStdString());
  if (!db) {
    return RunQml(app, "StartupError",
                  {{QStringLiteral("errorText"),
                    QString::fromStdString(std::move(db).error().message)}});
  }

  // Everything QML uses is created before the engine (in RunQml), so it is
  // destroyed after it.
  mymoney::storage::AppServices services(std::move(*db));
  AccountListModel account_list(services.accounts(), services.currencies(),
                                services.balance_service());
  CurrencyListModel currency_list(services.currencies());
  AccountEditor account_editor(services.accounts(), services.account_service());
  CurrencyEditor currency_editor(services.currencies());
  account_list.reload();
  currency_list.reload();

  // After a save, the lists show the new state.
  QObject::connect(&account_editor, &AccountEditor::saved, &account_list,
                   &AccountListModel::reload);
  QObject::connect(&currency_editor, &CurrencyEditor::saved, &currency_list,
                   &CurrencyListModel::reload);

  // D12, option B: QML gets exactly these objects as its singletons.
  const SingletonInstance<AccountListModel>::Registration account_list_qml(
      &account_list);
  const SingletonInstance<CurrencyListModel>::Registration currency_list_qml(
      &currency_list);
  const SingletonInstance<AccountEditor>::Registration account_editor_qml(
      &account_editor);
  const SingletonInstance<CurrencyEditor>::Registration currency_editor_qml(
      &currency_editor);

  // Main.qml declares "required property string appVersion".
  return RunQml(
      app, "Main",
      {{QStringLiteral("appVersion"), QGuiApplication::applicationVersion()}});
}
