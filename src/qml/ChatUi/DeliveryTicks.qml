import QtQuick

import Logos.Theme
import Logos.Controls
import Logos.Icons

// A sent message's ticks: one once it is sent, two once another member has
// received it. Standalone.
Item {
    id: root

    // Whether another member received the message.
    property bool delivered: false
    property color color: Theme.palette.textTertiary

    implicitWidth: root.delivered ? 17 : 12
    implicitHeight: 12

    Accessible.role: Accessible.Indicator
    Accessible.name: root.delivered ? tip.text : qsTr("Sent")

    // MultiEffect mixes its tint in by the tint's alpha, so the ticks take the
    // colour opaque and its alpha as opacity.
    Item {
        anchors.fill: parent
        opacity: root.color.a

        LogosIcon {
            width: 12
            height: 12
            source: LogosIcons.check
            color: Qt.rgba(root.color.r, root.color.g, root.color.b, 1)
        }

        LogosIcon {
            objectName: "secondTick"
            x: 5
            width: 12
            height: 12
            visible: root.delivered
            source: LogosIcons.check
            color: Qt.rgba(root.color.r, root.color.g, root.color.b, 1)
        }
    }

    HoverHandler {
        id: hover
    }

    LogosToolTip {
        id: tip
        objectName: "deliveredTip"
        //: Shown over a sent message's two ticks
        text: qsTr("At least one member has received it")
        visible: root.delivered && hover.hovered
    }
}
