import QtQuick
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// A filter that is on or off, carrying how many lines it stands for. Switched
// off it fades rather than disappearing, so a domain that wrote nothing stays
// distinguishable from one that is being hidden. Set the properties; standalone.
Rectangle {
    id: root

    required property string label
    required property bool checked
    // How many lines the filter stands for. Negative carries no count, which is
    // what a switch that counts nothing wants.
    property int count: -1
    // The dot that ties the chip to the rows it filters.
    property color accent: Theme.palette.textMuted

    signal toggled

    implicitWidth: row.implicitWidth + 2 * Theme.spacing.small
    implicitHeight: 24
    radius: Theme.spacing.radiusPill
    color: root.checked ? Theme.palette.overlayLight : "transparent"
    border.width: 1
    border.color: root.checked ? Theme.palette.borderDark : "transparent"

    RowLayout {
        id: row

        anchors.centerIn: parent
        spacing: 6

        Rectangle {
            Layout.preferredWidth: 7
            Layout.preferredHeight: 7
            radius: Theme.spacing.radiusPill
            color: root.accent
            opacity: root.checked ? 1 : 0.35
        }

        LogosText {
            objectName: "chipLabel"
            text: root.label
            font.pixelSize: Theme.typography.secondaryText
            color: root.checked ? Theme.palette.text : Theme.palette.textMuted
        }

        LogosText {
            objectName: "chipCount"
            visible: root.count >= 0
            text: root.count.toLocaleString(Qt.locale(), "f", 0)
            font.family: Theme.typography.mono
            font.pixelSize: Theme.typography.badgeText + 2
            color: root.checked ? Theme.palette.textTertiary : Theme.palette.textMuted
        }
    }

    TapHandler {
        onTapped: root.toggled()
    }

    HoverHandler {
        cursorShape: Qt.PointingHandCursor
    }
}
