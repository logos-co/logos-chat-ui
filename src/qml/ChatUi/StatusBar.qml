import QtQuick
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// The strip across the foot of the canvas, where the app says what it could not
// do. It holds the newest failure until someone deals with it and counts the
// ones queued behind it, so a burst is visible rather than collapsing to
// whichever arrived last. Activating the message is what clears the strip.
//
// With nothing to report it rests on the newest line of the session log, which
// is how it says the log is running without asking to be read.
// Set the properties; standalone.
Rectangle {
    id: root

    // The newest failure, in prose. Empty leaves the strip resting.
    required property string errorMessage
    // How many failures are waiting, the displayed one included.
    required property int errorCount
    // The newest line of the session log, shown while nothing has failed.
    property string lastLine: ""
    // Something that just happened and is worth one line: it outranks the
    // resting line for a few seconds, then gives way again.
    property string notice: ""
    property bool consoleOpen: false

    signal errorActivated
    signal consoleToggled

    readonly property bool alerting: root.errorCount > 0
    readonly property bool noticing: noticeTimer.running

    implicitWidth: 480
    implicitHeight: 26
    radius: Theme.spacing.radiusMedium
    color: Theme.palette.backgroundTertiary
    border.width: 1
    border.color: Theme.palette.borderSubtle

    onNoticeChanged: if (root.notice !== "")
        noticeTimer.restart()

    Timer {
        id: noticeTimer
        interval: 6000
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacing.medium
        anchors.rightMargin: Theme.spacing.tiny
        spacing: Theme.spacing.small

        Rectangle {
            visible: root.alerting && !root.noticing
            Layout.preferredWidth: 6
            Layout.preferredHeight: 6
            radius: Theme.spacing.radiusPill
            color: Theme.palette.error
        }

        LogosText {
            objectName: "statusMessage"
            Layout.fillWidth: true
            text: root.noticing ? root.notice : root.alerting ? root.errorMessage : root.lastLine
            textFormat: Text.PlainText
            color: root.noticing ? Theme.palette.textSecondary : root.alerting ? Theme.palette.error : Theme.palette.textMuted
            font.family: root.noticing || root.alerting ? Theme.typography.publicSans : Theme.typography.mono
            font.pixelSize: Theme.typography.secondaryText
            elide: Text.ElideRight
            Accessible.role: Accessible.StaticText
            Accessible.name: text

            TapHandler {
                enabled: root.alerting
                onTapped: root.errorActivated()
            }

            HoverHandler {
                enabled: root.alerting
                cursorShape: Qt.PointingHandCursor
            }
        }

        LogosText {
            objectName: "statusErrorCount"
            visible: root.errorCount > 1 && !root.noticing
            //: How many failures are waiting on the status bar, the one shown included.
            text: qsTr("%1 errors").arg(root.errorCount)
            color: Theme.palette.textTertiary
            font.pixelSize: Theme.typography.secondaryText
        }

        Rectangle {
            id: consoleButton
            objectName: "consoleButton"

            Layout.preferredWidth: consoleRow.implicitWidth + 2 * Theme.spacing.small
            Layout.preferredHeight: 20
            radius: Theme.spacing.radiusSmall
            color: root.consoleOpen ? Theme.palette.surfaceRaised : Theme.palette.overlayLight
            border.width: 1
            border.color: Theme.palette.borderDark
            Accessible.role: Accessible.Button
            //: Names the button that opens and closes the log console
            Accessible.name: qsTr("Developer console")
            Accessible.onPressAction: root.consoleToggled()

            RowLayout {
                id: consoleRow
                anchors.centerIn: parent
                spacing: 6

                LogosText {
                    //: Button at the foot of the window that opens the log console
                    text: qsTr(">_ Console")
                    font.family: Theme.typography.mono
                    font.pixelSize: Theme.typography.secondaryText
                    color: root.consoleOpen ? Theme.palette.primary : Theme.palette.textTertiary
                }

                Rectangle {
                    objectName: "consoleBadge"
                    visible: root.errorCount > 0
                    Layout.preferredWidth: Math.max(15, badgeCount.implicitWidth + 8)
                    Layout.preferredHeight: 15
                    radius: Theme.spacing.radiusPill
                    color: Theme.palette.error

                    LogosText {
                        id: badgeCount
                        anchors.centerIn: parent
                        text: root.errorCount
                        font.pixelSize: Theme.typography.badgeText + 1
                        font.weight: Theme.typography.weightBold
                        color: Theme.palette.backgroundBlack
                    }
                }
            }

            TapHandler {
                onTapped: root.consoleToggled()
            }

            HoverHandler {
                cursorShape: Qt.PointingHandCursor
            }
        }
    }
}
