#ifndef MYMONEY_UI_MONEY_FORMAT_H_
#define MYMONEY_UI_MONEY_FORMAT_H_

#include <QLocale>
#include <QString>

#include "core/money.h"

namespace mymoney::ui {

// An amount as the user reads it, with the conventions of `locale`: its
// digits, group separator, decimal point and minus sign, then the currency
// code - "1 234,50 EUR" in Russian, "1,234.50 EUR" in English (ADR-0016).
// Works on the integer minor units only, never through a floating-point
// number. `minor_digits` is the currency's number of decimal places (0..4).
[[nodiscard]] QString FormatMoney(const core::Money& money, int minor_digits,
                                  const QLocale& locale = QLocale());

}  // namespace mymoney::ui

#endif  // MYMONEY_UI_MONEY_FORMAT_H_
