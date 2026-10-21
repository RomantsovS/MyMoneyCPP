# mymoney_app: the Qt Quick application (Android, Windows, Linux).

qt_add_executable(mymoney_app main.cpp)

# Android requires the version code to grow with every release it installs.
math(EXPR mymoney_version_code
  "${PROJECT_VERSION_MAJOR} * 10000 + ${PROJECT_VERSION_MINOR} * 100 + ${PROJECT_VERSION_PATCH}")

set_target_properties(mymoney_app PROPERTIES
  OUTPUT_NAME MyMoney
  WIN32_EXECUTABLE ON
  # Permanent once installed on a device (ADR-0005).
  QT_ANDROID_PACKAGE_NAME "io.github.romantsovs.mymoney"
  QT_ANDROID_APP_NAME "MyMoney"
  QT_ANDROID_VERSION_NAME "${PROJECT_VERSION}"
  QT_ANDROID_VERSION_CODE "${mymoney_version_code}"
  # The app has no QML files of its own; they live in the MyMoney.UI module.
  # qmlimportscanner must scan them to know which Qt QML modules (QtQuick, ...)
  # to ship. Qt reads two different properties for this:
  #   QT_QML_ROOT_PATH                        - androiddeployqt (Android APK)
  #   QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS  - desktop deploy script
  QT_QML_ROOT_PATH "${PROJECT_SOURCE_DIR}/src/ui"
  QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS "${PROJECT_SOURCE_DIR}/src/ui"
)

# The composition root knows every layer (ADR-0016).
target_link_libraries(mymoney_app PRIVATE
  mymoney::core
  mymoney::storage
  mymoney_ui
  mymoney_uiplugin
  Qt6::Gui
  Qt6::Qml
)

mymoney_target_defaults(mymoney_app)

# Deployment (desktop): "cmake --install" creates a self-contained folder with
# MyMoney + Qt libraries, plugins and the QML modules it imports, so it runs
# without Qt on PATH. Android packages are built by androiddeployqt instead.
if(NOT ANDROID)
  install(TARGETS mymoney_app
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    BUNDLE DESTINATION .)

  qt_generate_deploy_qml_app_script(
    TARGET mymoney_app
    OUTPUT_SCRIPT mymoney_deploy_script)
  install(SCRIPT "${mymoney_deploy_script}")
endif()
