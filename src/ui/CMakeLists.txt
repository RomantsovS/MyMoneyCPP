# mymoney_ui: QML module "MyMoney.UI" - the QML pages and the C++ models and
# controllers behind them (ADR-0016). Depends on core and Qt, never on storage:
# the app's main() hands in the repositories and services.
#
# The module is a static library. qt_add_qml_module also creates the plugin
# target mymoney_uiplugin; the app must link it and import it with
# Q_IMPORT_QML_PLUGIN, otherwise QML reports "module not installed" at run time.

qt_add_library(mymoney_ui STATIC)

set(mymoney_ui_qml_files
  AccountForm
  AccountsPage
  CurrencyForm
  Main
  StartupError
)
list(TRANSFORM mymoney_ui_qml_files REPLACE "(.+)" "qml/\\1.qml"
  OUTPUT_VARIABLE mymoney_ui_qml_paths)

# The QML compiler turns every .qml file into C++ (.rcc/qmlcache/...). MSVC
# warns about unreachable code inside Qt headers when compiling it (C4702).
# That is generated code, not ours (ADR-0011): only there the warning is off.
foreach(name IN LISTS mymoney_ui_qml_files)
  set_source_files_properties(
    "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/mymoney_ui_qml_${name}_qml.cpp"
    PROPERTIES COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:/wd4702>")
endforeach()

qt_add_qml_module(mymoney_ui
  URI MyMoney.UI
  VERSION 1.0
  # Classes marked QML_ELEMENT here are registered as QML types of the module.
  SOURCES
    controllers/account_editor.cpp
    controllers/account_editor.h
    controllers/currency_editor.cpp
    controllers/currency_editor.h
    models/account_list_model.cpp
    models/account_list_model.h
    models/currency_list_model.cpp
    models/currency_list_model.h
    money_format.cpp
    money_format.h
    singleton_instance.h
  QML_FILES ${mymoney_ui_qml_paths}
)

target_include_directories(mymoney_ui PUBLIC "${PROJECT_SOURCE_DIR}/src")
# The generated type registrations include each QML_ELEMENT header by its bare
# file name (#include <account_list_model.h>). Only for that generated file;
# our own code includes from src/ ("ui/models/account_list_model.h").
target_include_directories(mymoney_ui PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/controllers"
  "${CMAKE_CURRENT_SOURCE_DIR}/models")
target_link_libraries(mymoney_ui
  PUBLIC mymoney::core Qt6::Qml
  PRIVATE Qt6::Quick)

mymoney_target_defaults(mymoney_ui)
