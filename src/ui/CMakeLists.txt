# mymoney_ui: QML module "MyMoney.UI" - QML files now, C++ models and
# controllers from M4 on.
#
# The module is a static library. qt_add_qml_module also creates the plugin
# target mymoney_uiplugin; the app must link it and import it with
# Q_IMPORT_QML_PLUGIN, otherwise QML reports "module not installed" at run time.

qt_add_library(mymoney_ui STATIC)

qt_add_qml_module(mymoney_ui
  URI MyMoney.UI
  VERSION 1.0
  QML_FILES
    qml/Main.qml
)

target_link_libraries(mymoney_ui PRIVATE Qt6::Quick)

mymoney_target_defaults(mymoney_ui)
