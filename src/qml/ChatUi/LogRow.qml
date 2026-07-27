import QtQuick
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// One line of the session log: when, how bad, who said it, and what it said.
// The severity is carried twice on purpose, as a rule down the left edge for
// scanning and as a word for reading. Set the properties; standalone.
Rectangle {
    id: root

    required property string time
    required property string levelName
    required property string domain
    required property string message
    // Whether this is an odd row, which is the only thing banding depends on.
    required property bool alternate
    // Long lines wrap instead of eliding.
    property bool wrap: false

    readonly property color accent: ChatTheme.logLevelColor(root.levelName)
    readonly property bool severe: root.levelName === "error" || root.levelName === "warning"

    implicitWidth: 600
    implicitHeight: line.implicitHeight + 2 * verticalPadding

    readonly property int verticalPadding: 3

    color: root.levelName === "error" ? Theme.colors.getColor(Theme.palette.error, 0.07) : root.alternate ? Theme.palette.backgroundMuted : "transparent"

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 2
        color: root.severe ? root.accent : "transparent"
    }

    RowLayout {
        id: line

        anchors.fill: parent
        anchors.leftMargin: Theme.spacing.medium
        anchors.rightMargin: Theme.spacing.medium
        anchors.topMargin: root.verticalPadding
        anchors.bottomMargin: root.verticalPadding
        spacing: Theme.spacing.small

        LogosText {
            objectName: "logTime"
            // Fixed, because a line that carries no time would otherwise pull
            // every column left and break the reading edge.
            Layout.preferredWidth: 96
            Layout.alignment: Qt.AlignTop
            text: root.time
            font.family: Theme.typography.mono
            font.pixelSize: Theme.typography.secondaryText
            color: Theme.palette.textMuted
            elide: Text.ElideRight
        }

        LogosText {
            objectName: "logLevel"
            text: root.levelName
            font.family: Theme.typography.mono
            font.pixelSize: Theme.typography.badgeText + 2
            font.capitalization: Font.AllUppercase
            font.letterSpacing: 0.6
            color: root.accent
            Layout.preferredWidth: 46
            Layout.alignment: Qt.AlignTop
        }

        RowLayout {
            spacing: 5
            Layout.preferredWidth: 96
            Layout.alignment: Qt.AlignTop

            Rectangle {
                Layout.preferredWidth: 6
                Layout.preferredHeight: 6
                radius: Theme.spacing.radiusPill
                color: ChatTheme.logDomainColor(root.domain)
            }

            LogosText {
                objectName: "logDomain"
                Layout.fillWidth: true
                text: root.domain
                font.family: Theme.typography.mono
                font.pixelSize: Theme.typography.secondaryText
                color: Theme.palette.textTertiary
                elide: Text.ElideRight
            }
        }

        LogosText {
            objectName: "logMessage"
            Layout.fillWidth: true
            text: root.message
            textFormat: Text.PlainText
            font.family: Theme.typography.mono
            font.pixelSize: Theme.typography.secondaryText
            color: root.levelName === "debug" ? Theme.palette.textSecondary : Theme.palette.text
            wrapMode: root.wrap ? Text.Wrap : Text.NoWrap
            elide: root.wrap ? Text.ElideNone : Text.ElideRight
        }
    }
}
