import QtQuick

import Logos.Controls

// Says that delivery was already running when Chat opened, so Chat is on a node
// whose network it does not know, and what that can mean for the user. Standing
// until closed, never timed; standalone.
LogosNotice {
    severity: LogosNotice.Warning
    closable: true
    message: qsTr("Delivery network unknown. You and contacts on Chat's network may not reach each other.")
}
