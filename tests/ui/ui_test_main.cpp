// main() of mymoney_ui_tests: Qt objects want a QCoreApplication, and the
// formatted amounts depend on the default locale - fixed here so the tests
// give the same result on every machine.

#include <QCoreApplication>
#include <QLocale>

#include <gtest/gtest.h>

int main(int argc, char* argv[]) {
  const QCoreApplication app(argc, argv);
  QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
