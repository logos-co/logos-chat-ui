pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// The developer console: the session log, filtered. It takes the whole canvas
// because reading a log is the activity, not an aside to one.
//
// Every filter is applied by the backend, so this only reports what the reader
// asked for and renders what comes back. Set the properties; standalone.
Rectangle {
    id: root

    required property var logModel
    // One entry per domain: `name`, `count`, `enabled`.
    required property var domains
    // One entry per severity: `label`, `value` (a level bit), `name`.
    required property var levelOptions
    // Which level bits the filter currently passes.
    required property int levels
    required property string filterText
    // Of the whole session, and of what the filters leave on screen.
    required property int lineCount
    required property int shownCount
    required property int errorCount
    required property string logPath
    required property string logSizeLabel

    signal domainToggled(string domain, bool enabled)
    signal levelToggled(int level, bool enabled)
    signal filterTextEdited(string text)
    signal fullExportRequested
    signal viewExportRequested

    // Whether new lines pull the view down with them. Scrolling away turns it
    // off, which is what makes reading back possible while the log is running.
    property bool following: true
    property bool wrap: false

    implicitWidth: 900
    implicitHeight: 600
    radius: Theme.spacing.radiusXlarge
    color: Theme.palette.backgroundTertiary
    border.width: 1
    border.color: Theme.palette.borderSubtle
    clip: true

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ── head ────────────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            Layout.leftMargin: Theme.spacing.medium
            Layout.rightMargin: Theme.spacing.medium
            spacing: Theme.spacing.small

            LogosText {
                //: Title of the developer log console
                text: qsTr("Developer console")
                font.pixelSize: Theme.typography.subtitleText
                font.weight: Theme.typography.weightMedium
            }

            Item {
                Layout.fillWidth: true
            }

            LogosText {
                objectName: "consoleErrorCount"
                visible: root.errorCount > 0
                //: How many error lines the session log holds
                text: root.errorCount === 1 ? qsTr("1 error") : qsTr("%1 errors").arg(root.errorCount)
                font.pixelSize: Theme.typography.secondaryText
                color: Theme.palette.error
            }

            LogosText {
                objectName: "consoleFollowState"
                text: root.following ? qsTr("following") : qsTr("paused")
                font.pixelSize: Theme.typography.secondaryText
                color: Theme.palette.textSecondary
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.palette.borderSubtle
        }

        // ── filters ─────────────────────────────────────────────────────────
        // Two fixed rows rather than one that reflows: a wrapping toolbar has to
        // report a height that depends on the width it is being given, and the
        // column cannot resolve that without leaving the rows on top of the log.
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacing.medium
            Layout.rightMargin: Theme.spacing.medium
            Layout.topMargin: Theme.spacing.small
            Layout.bottomMargin: Theme.spacing.small
            spacing: Theme.spacing.small

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacing.medium

                Row {
                    spacing: Theme.spacing.tiny

                    Repeater {
                        model: root.domains

                        LogFilterChip {
                            required property var modelData

                            label: modelData.name
                            count: modelData.count
                            checked: modelData.enabled
                            accent: ChatTheme.logDomainColor(modelData.name)
                            onToggled: root.domainToggled(modelData.name, !modelData.enabled)
                        }
                    }
                }

                Row {
                    spacing: Theme.spacing.tiny

                    Repeater {
                        model: root.levelOptions

                        LogFilterChip {
                            required property var modelData

                            readonly property bool on: (root.levels & modelData.value) !== 0

                            objectName: "levelChip_" + modelData.name
                            label: modelData.label
                            checked: on
                            accent: ChatTheme.logLevelColor(modelData.name)
                            onToggled: root.levelToggled(modelData.value, !on)
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacing.medium

                LogosTextField {
                    id: filterField
                    objectName: "logFilterField"
                    Layout.preferredWidth: 260
                    Layout.preferredHeight: 28
                    //: Placeholder of the box that narrows the log to matching text
                    placeholderText: qsTr("Filter text")
                    text: root.filterText

                    // The editor's own signal, not the property's: reporting every
                    // change would echo the value the backend pushes back.
                    Connections {
                        target: filterField.textInput
                        function onTextEdited() {
                            root.filterTextEdited(filterField.text);
                        }
                    }
                }

                Row {
                    spacing: Theme.spacing.tiny

                    LogFilterChip {
                        objectName: "followChip"
                        //: Toggle that keeps the newest log line on screen
                        label: qsTr("follow")
                        checked: root.following
                        accent: Theme.palette.primary
                        onToggled: root.following = !root.following
                    }

                    LogFilterChip {
                        objectName: "wrapChip"
                        //: Toggle that wraps long log lines instead of cutting them
                        label: qsTr("wrap")
                        checked: root.wrap
                        accent: Theme.palette.primary
                        onToggled: root.wrap = !root.wrap
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                LogosButton {
                    id: exportButton
                    objectName: "exportButton"
                    //: Button that writes the log out to a file
                    text: qsTr("Export")
                    variant: LogosButton.Variant.Secondary
                    Layout.preferredHeight: 28
                    font.pixelSize: Theme.typography.secondaryText
                    onClicked: exportMenu.popup(0, exportButton.height + Theme.spacing.tiny)

                    LogosMenu {
                        id: exportMenu
                        objectName: "exportMenu"

                        ChatMenuItem {
                            objectName: "exportFullMenuItem"
                            //: Menu entry that exports the whole log file, filters ignored
                            text: qsTr("Export the whole file")
                            // The session's line count would overstate this once the
                            // host has rotated a full file aside, so the size of what
                            // will actually be written is what the entry promises.
                            //: Says how much the unfiltered export covers, under its entry
                            description: root.logSizeLabel
                            onTriggered: root.fullExportRequested()
                        }
                        ChatMenuItem {
                            objectName: "exportViewMenuItem"
                            //: Menu entry that exports only what the filters show
                            text: qsTr("Export this view")
                            // Counting it would mean scanning the file the export is
                            // about to read, and the count on screen is of the window,
                            // not of the file.
                            //: Says what the filtered export covers, under its entry
                            description: qsTr("only the lines these filters pass")
                            onTriggered: root.viewExportRequested()
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.palette.borderSubtle
        }

        // ── lines ───────────────────────────────────────────────────────────
        ListView {
            id: lines
            objectName: "logList"

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            reuseItems: true
            model: root.logModel

            // Following means the newest line is the one on screen. A reader who
            // scrolls up stops it, and returning to the end resumes it.
            onCountChanged: if (root.following)
                positionViewAtEnd()
            onMovementEnded: root.following = atYEnd

            delegate: LogRow {
                // The rest of the row's inputs are its own required properties,
                // which the view fills from the model's roles by name.
                required property int index

                width: lines.width
                alternate: index % 2 === 1
                wrap: root.wrap
            }

            ScrollBar.vertical: LogosScrollBar {}

            EmptyState {
                anchors.centerIn: parent
                width: parent.width - 2 * Theme.spacing.xxlarge
                visible: lines.count === 0
                text: root.logPath === "" ?
                //: Shown when the host keeps no log for the console to read
                qsTr("This host keeps no session log.") : root.lineCount === 0 ?
                //: Shown while the session has written nothing yet
                qsTr("Nothing written yet.") :
                //: Shown when filters hide every line
                qsTr("No line matches these filters.")
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.palette.borderSubtle
        }

        // ── foot ────────────────────────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            Layout.leftMargin: Theme.spacing.medium
            Layout.rightMargin: Theme.spacing.medium
            spacing: Theme.spacing.small

            LogosText {
                objectName: "consoleTally"
                //: Reports how much of the session log the filters leave on screen
                text: root.shownCount !== root.lineCount ? qsTr("%1 of %2 lines").arg(root.shownCount).arg(root.lineCount) : root.lineCount === 1 ? qsTr("1 line, none hidden") : qsTr("%1 lines, none hidden").arg(root.lineCount)
                font.pixelSize: Theme.typography.secondaryText
                color: Theme.palette.textTertiary
            }

            Item {
                Layout.fillWidth: true
            }

            LogosText {
                objectName: "consolePath"
                Layout.maximumWidth: root.width / 2
                text: root.logPath === "" ? "" : root.logPath + " · " + root.logSizeLabel
                font.family: Theme.typography.mono
                font.pixelSize: Theme.typography.secondaryText
                color: Theme.palette.textMuted
                elide: Text.ElideMiddle
            }
        }
    }
}
