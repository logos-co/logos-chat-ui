import QtQuick
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// A group-roster row: the member label with a self marker, revealing the full
// address on hover and requesting a copy on click. Delegate roles bind by name.
LogosItemDelegate {
    id: root

    required property string label
    required property string address
    required property bool isSelf

    signal copyRequested(string address)

    width: 200
    implicitHeight: 40
    radius: 0

    Accessible.role: Accessible.ListItem
    Accessible.name: label

    onClicked: root.copyRequested(root.address)

    contentItem: RowLayout {
        spacing: Theme.spacing.small

        LogosText {
            text: root.label
            textFormat: Text.PlainText
            color: Theme.palette.text
            font.pixelSize: Theme.typography.secondaryText
            elide: Text.ElideRight
            Layout.fillWidth: true
        }

        LogosText {
            visible: root.isSelf
            //: Badge marking the current user in the member list
            text: qsTr("you")
            color: Theme.palette.primary
            font.pixelSize: Theme.typography.badgeText
            Layout.alignment: Qt.AlignVCenter
        }
    }

    LogosToolTip {
        text: root.address
        placement: LogosToolTip.Left
        visible: root.hovered && root.address !== ""
    }
}
