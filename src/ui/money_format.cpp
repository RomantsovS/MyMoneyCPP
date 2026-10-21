#include "ui/money_format.h"

#include <cstdint>

#include <QLocale>
#include <QString>

#include "core/check.h"
#include "core/money.h"

namespace mymoney::ui {

QString FormatMoney(const core::Money& money, int minor_digits,
                    const QLocale& locale) {
  core::Check(minor_digits >= 0 && minor_digits <= 4,
              "a currency has 0 to 4 decimal places");
  // The magnitude as unsigned: the magnitude of INT64_MIN does not fit into
  // int64.
  const std::int64_t units = money.minor_units();
  const std::uint64_t magnitude = units < 0
                                      ? 0 - static_cast<std::uint64_t>(units)
                                      : static_cast<std::uint64_t>(units);
  std::uint64_t scale = 1;
  for (int i = 0; i < minor_digits; ++i) {
    scale *= 10;
  }

  // The integer part with the locale's group separators ("1 234").
  QString text = locale.toString(static_cast<qulonglong>(magnitude / scale));
  if (minor_digits > 0) {
    // The fraction without separators, padded with the locale's zeros.
    QLocale plain = locale;
    plain.setNumberOptions(QLocale::OmitGroupSeparator);
    QString fraction =
        plain.toString(static_cast<qulonglong>(magnitude % scale));
    while (fraction.size() < minor_digits) {
      fraction.prepend(locale.zeroDigit());
    }
    text += locale.decimalPoint() + fraction;
  }
  if (units < 0) {
    text.prepend(locale.negativeSign());
  }
  return text + u' ' + QString::fromUtf8(money.currency().code());
}

}  // namespace mymoney::ui
