#ifndef MYMONEY_UI_DATE_CONVERT_H_
#define MYMONEY_UI_DATE_CONVERT_H_

#include <chrono>

#include <QDate>

namespace mymoney::ui {

// Business dates cross the C++/Qt border only here (PROJECT_PLAN 4.5): the
// core uses std::chrono::year_month_day, Qt uses QDate. Both are calendar
// dates without a time zone. Precondition for both: a valid date.
[[nodiscard]] QDate ToQDate(std::chrono::year_month_day date);
[[nodiscard]] std::chrono::year_month_day FromQDate(QDate date);

// Today's date in the device's time zone.
[[nodiscard]] std::chrono::year_month_day LocalToday();

}  // namespace mymoney::ui

#endif  // MYMONEY_UI_DATE_CONVERT_H_
