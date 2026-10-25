#include "ui/date_convert.h"

#include <chrono>

#include <QDate>

#include "core/check.h"

namespace mymoney::ui {

QDate ToQDate(std::chrono::year_month_day date) {
  core::Check(date.ok(), "ToQDate needs a valid date");
  return {static_cast<int>(date.year()),
          static_cast<int>(static_cast<unsigned>(date.month())),
          static_cast<int>(static_cast<unsigned>(date.day()))};
}

std::chrono::year_month_day FromQDate(QDate date) {
  core::Check(date.isValid(), "FromQDate needs a valid date");
  return {std::chrono::year{date.year()},
          std::chrono::month{static_cast<unsigned>(date.month())},
          std::chrono::day{static_cast<unsigned>(date.day())}};
}

std::chrono::year_month_day LocalToday() {
  return FromQDate(QDate::currentDate());
}

}  // namespace mymoney::ui
