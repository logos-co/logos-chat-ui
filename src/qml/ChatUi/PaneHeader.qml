import QtQuick
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// A 48px pane header: a title on the left that fills and elides, plus a trailing
// slot on the right for actions or status. Declare trailing items as children.
// Standalone: set `title` and optionally add trailing children.
Rectangle {
    id: root

    property string title
    // Optional secondary line under the title; empty hides it and the header
    // keeps its title-only height.
    property string subtitle
    // Trailing items (buttons, status dot) laid out right-aligned.
    default property alias trailing: trailingRow.data

    implicitWidth: 200
    implicitHeight: Math.max(48, titleColumn.implicitHeight + 2 * Theme.spacing.small)
    color: Theme.palette.backgroundTertiary

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacing.medium
        anchors.rightMargin: Theme.spacing.medium
        spacing: Theme.spacing.small

        ColumnLayout {
            id: titleColumn
            spacing: 0
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter

            LogosText {
                text: root.title
                textFormat: Text.PlainText
                color: Theme.palette.text
                font.pixelSize: Theme.typography.primaryText
                font.weight: Theme.typography.weightBold
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            LogosText {
                text: root.subtitle
                visible: root.subtitle !== ""
                textFormat: Text.PlainText
                color: Theme.palette.textSecondary
                font.pixelSize: Theme.typography.secondaryText
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        RowLayout {
            id: trailingRow
            spacing: Theme.spacing.small
            Layout.alignment: Qt.AlignVCenter
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.palette.borderSubtle
    }
}
