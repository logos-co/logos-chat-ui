#ifndef IDENTITY_H
#define IDENTITY_H

#include <QString>

namespace Identity {

// Short form of an account address, the identity string every surface shows in
// place of the full address. Empty for an empty address; a caller that must
// name an unknown account substitutes its own wording.
QString shortLabel(const QString& address);

} // namespace Identity

#endif
