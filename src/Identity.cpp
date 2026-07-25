#include "Identity.h"

namespace {

constexpr int kShortLabelChars = 8;

} // namespace

QString Identity::shortLabel(const QString& address)
{
    return address.left(kShortLabelChars);
}
