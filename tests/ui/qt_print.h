#ifndef MYMONEY_TESTS_UI_QT_PRINT_H_
#define MYMONEY_TESTS_UI_QT_PRINT_H_

// GoogleTest finds these PrintTo overloads by argument-dependent lookup - Qt's
// string types live in the global namespace - and prints the text in failure
// messages instead of the object's bytes.

#include <ostream>

#include <QString>
#include <QStringList>

inline void PrintTo(const QString& text, std::ostream* os) {
  *os << '"' << text.toStdString() << '"';
}

inline void PrintTo(const QStringList& list, std::ostream* os) {
  *os << '{';
  for (qsizetype i = 0; i < list.size(); ++i) {
    *os << (i == 0 ? "" : ", ");
    PrintTo(list.at(i), os);
  }
  *os << '}';
}

#endif  // MYMONEY_TESTS_UI_QT_PRINT_H_
