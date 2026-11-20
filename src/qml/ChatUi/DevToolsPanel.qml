pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// The developer tools: the run's failures and log files, beside what the open
// conversation reports about itself for debugging. State lays that report out
// and Raw shows it whole, so a key the chat core adds shows up with no change
// here. Set the properties and read the signals; standalone.
Rectangle {
    id: root

    // The run's logs, as SessionLogsView takes them.
    property var errors: []
    property var runs: []
    property string logDir: ""

    // The open conversation's name, empty when none is open.
    property string conversationName: ""
    // Whether it is from a previous session, which leaves no group to ask.
    property bool historyOnly: false
    // Its debug data as the backend publishes it: `epoch`, `epochFingerprint`,
    // `members`, `counts`, `json` and `readAt`. Empty until read.
    property var debugData: ({})

    // 0 Errors, 1 Logs, 2 State, 3 Raw.
    property alias currentTab: tabs.currentIndex
    // Whether the tab on show is about the open conversation rather than the run.
    readonly property bool conversationTab: tabs.currentIndex >= 2

    readonly property bool debugDataRead: (root.debugData.readAt ?? "") !== ""
    readonly property var members: root.debugData.members ?? []
    // What State says in place of a table, and Raw in place of the map.
    readonly property string unreadText: {
        if (root.conversationName === "")
            return qsTr("No conversation is open.");
        if (root.historyOnly)
            return qsTr("A conversation from a previous session has no state to read.");
        return qsTr("Nothing read yet. A read that failed is under Errors.");
    }

    signal refreshRequested
    signal closeRequested

    function syncColor(sync) {
        return ({
                forked: Theme.palette.error,
                behind: Theme.palette.warning,
                ahead: Theme.palette.info,
                synced: Theme.palette.success
            })[sync] ?? Theme.palette.textTertiary;
    }

    implicitWidth: 640
    implicitHeight: 320
    radius: Theme.spacing.radiusXlarge
    color: Theme.palette.backgroundTertiary
    border.width: 1
    border.color: Theme.palette.borderSubtle

    component Mono: LogosText {
        font.family: Theme.typography.mono
        font.pixelSize: Theme.typography.secondaryText
        color: Theme.palette.textSecondary
    }

    component SyncMark: RowLayout {
        id: syncMark
        property string sync
        spacing: 6

        Rectangle {
            Layout.preferredWidth: 7
            Layout.preferredHeight: 7
            Layout.alignment: Qt.AlignVCenter
            radius: Theme.spacing.radiusPill
            color: root.syncColor(syncMark.sync)
        }
        Mono {
            text: syncMark.sync
            color: root.syncColor(syncMark.sync)
        }
    }

    ClipboardProxy {
        id: clipboard
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: Theme.spacing.tiny
        anchors.bottomMargin: Theme.spacing.medium
        anchors.leftMargin: Theme.spacing.large
        anchors.rightMargin: Theme.spacing.medium
        spacing: Theme.spacing.small

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacing.medium

            LogosTabBar {
                id: tabs
                objectName: "devtoolsTabBar"
                Layout.preferredWidth: 340

                LogosTabButton {
                    text: qsTr("Errors")
                }
                LogosTabButton {
                    //: Tab holding the run's log files
                    text: qsTr("Logs")
                }
                LogosTabButton {
                    //: Tab laying out the open conversation's debug data
                    text: qsTr("State")
                }
                LogosTabButton {
                    //: Tab showing the open conversation's debug data as JSON
                    text: qsTr("Raw")
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Mono {
                objectName: "debugConversationName"
                visible: root.conversationTab && root.conversationName !== ""
                Layout.maximumWidth: 240
                text: root.conversationName
                textFormat: Text.PlainText
                elide: Text.ElideRight
            }

            LogosText {
                objectName: "debugReadAt"
                visible: root.conversationTab && root.debugDataRead
                //: When the debug data on show was read, as HH:mm:ss
                text: qsTr("read %1").arg(root.debugData.readAt ?? "")
                font.pixelSize: Theme.typography.secondaryText
                color: Theme.palette.textTertiary
            }

            ChatIconButton {
                id: refreshButton
                objectName: "debugRefreshButton"
                visible: root.conversationTab
                enabled: root.conversationName !== "" && !root.historyOnly
                size: 24
                iconSize: 14
                iconSource: Qt.resolvedUrl("icons/refresh.png")
                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Read again")
                onClicked: root.refreshRequested()

                LogosToolTip {
                    text: qsTr("Read again")
                    placement: LogosToolTip.Top
                    visible: refreshButton.hovered
                }
            }

            ChatIconButton {
                objectName: "devtoolsCloseButton"
                size: 24
                iconSize: 12
                iconSource: Qt.resolvedUrl("icons/close.png")
                Accessible.role: Accessible.Button
                Accessible.name: qsTr("Close the developer tools")
                onClicked: root.closeRequested()
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: Math.max(0, tabs.currentIndex - 1)

            SessionLogsView {
                errors: root.errors
                runs: root.runs
                logDir: root.logDir
                page: Math.min(tabs.currentIndex, 1)
            }

            // ── State ─────────────────────────────────────────────────
            Item {
                EmptyState {
                    objectName: "stateEmpty"
                    anchors.fill: parent
                    visible: root.members.length === 0
                    text: root.debugDataRead ? qsTr("This conversation reports no group state.") : root.unreadText
                    verticalAlignment: Text.AlignVCenter
                }

                ColumnLayout {
                    anchors.fill: parent
                    visible: root.members.length > 0
                    spacing: Theme.spacing.small

                    Flow {
                        Layout.fillWidth: true
                        spacing: Theme.spacing.large

                        RowLayout {
                            spacing: 6

                            Mono {
                                text: "epoch"
                                color: Theme.palette.textTertiary
                            }
                            Mono {
                                objectName: "debugEpoch"
                                text: (root.debugData.epoch ?? -1) >= 0 ? root.debugData.epoch : "—"
                                color: Theme.palette.text
                            }
                        }

                        RowLayout {
                            spacing: 6

                            Mono {
                                text: "epoch_fingerprint"
                                color: Theme.palette.textTertiary
                            }
                            Mono {
                                text: root.debugData.epochFingerprint || "—"
                                color: Theme.palette.text
                            }
                            ChatIconButton {
                                id: copyFingerprint
                                visible: (root.debugData.epochFingerprint ?? "") !== ""
                                size: 20
                                iconSize: 11
                                iconSource: Qt.resolvedUrl("icons/copy.png")
                                Accessible.role: Accessible.Button
                                Accessible.name: qsTr("Copy the fingerprint")
                                onClicked: clipboard.copy(root.debugData.epochFingerprint)

                                LogosToolTip {
                                    text: qsTr("Copy the fingerprint")
                                    placement: LogosToolTip.Top
                                    visible: copyFingerprint.hovered
                                }
                            }
                        }

                        Row {
                            spacing: Theme.spacing.medium

                            Repeater {
                                model: root.debugData.counts ?? []

                                RowLayout {
                                    id: countEntry
                                    objectName: "debugSyncCount"
                                    required property var modelData
                                    spacing: 4

                                    Mono {
                                        text: countEntry.modelData.count
                                        color: Theme.palette.text
                                    }
                                    SyncMark {
                                        sync: countEntry.modelData.sync
                                    }
                                }
                            }
                        }
                    }

                    LogosTable {
                        objectName: "debugMembers"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: root.members
                        rowHeight: 34
                        headerHeight: 30

                        columns: [
                            LogosTableColumn {
                                title: qsTr("member")
                                role: "label"
                                minWidth: 140
                                preferredWidth: 170
                                cellDelegate: memberCell
                            },
                            LogosTableColumn {
                                title: "signer"
                                role: "signer"
                                minWidth: 150
                                preferredWidth: 160
                                cellDelegate: signerCell
                            },
                            LogosTableColumn {
                                title: "sync"
                                role: "sync"
                                minWidth: 120
                                preferredWidth: 140
                                cellDelegate: syncCell
                            },
                            LogosTableColumn {
                                title: "epoch"
                                role: "epoch"
                                minWidth: 80
                                preferredWidth: 90
                                cellDelegate: epochCell
                            },
                            LogosTableColumn {
                                title: "epoch_fingerprint"
                                role: "epochFingerprint"
                                minWidth: 160
                                fillWidth: true
                                cellDelegate: fingerprintCell
                            }
                        ]
                    }
                }
            }

            // ── Raw ───────────────────────────────────────────────────
            Item {
                EmptyState {
                    anchors.fill: parent
                    visible: !root.debugDataRead
                    text: root.unreadText
                    verticalAlignment: Text.AlignVCenter
                }

                LogosScrollView {
                    anchors.fill: parent
                    visible: root.debugDataRead

                    Mono {
                        objectName: "debugJson"
                        text: root.debugData.json ?? ""
                        textFormat: Text.PlainText
                    }
                }

                LogosButton {
                    objectName: "copyJsonButton"
                    anchors.top: parent.top
                    anchors.right: parent.right
                    visible: root.debugDataRead
                    topPadding: Theme.spacing.small
                    bottomPadding: Theme.spacing.small
                    leftPadding: Theme.spacing.medium
                    rightPadding: Theme.spacing.medium
                    implicitWidth: implicitContentWidth + leftPadding + rightPadding
                    implicitHeight: implicitContentHeight + topPadding + bottomPadding
                    text: qsTr("Copy JSON")
                    onClicked: clipboard.copy(root.debugData.json)
                }
            }
        }
    }

    // LogosTable hands a cell its row as `rowItem`, a property of the Loader
    // that is the cell's parent, which qmllint cannot type. With a list for a
    // model, the row's `modelData` is the member's map.
    // qmllint disable missing-property
    Component {
        id: memberCell

        RowLayout {
            id: cell
            readonly property var member: parent ? parent.rowItem.modelData : ({})
            spacing: Theme.spacing.small

            Mono {
                text: cell.member.label ?? ""
                textFormat: Text.PlainText
                font.weight: Theme.typography.weightMedium
                color: Theme.palette.text
            }
            LogosText {
                objectName: "debugSelfMark"
                visible: cell.member.isSelf ?? false
                //: Marks this account's own row in a list of members
                text: qsTr("you")
                font.pixelSize: Theme.typography.secondaryText
                color: Theme.palette.textTertiary
            }
            Item {
                Layout.fillWidth: true
            }
        }
    }

    Component {
        id: signerCell

        Mono {
            readonly property var member: parent ? parent.rowItem.modelData : ({})
            verticalAlignment: Text.AlignVCenter
            text: member.signer ?? ""
            elide: Text.ElideMiddle
        }
    }

    Component {
        id: syncCell

        Item {
            id: cell
            readonly property var member: parent ? parent.rowItem.modelData : ({})

            SyncMark {
                anchors.verticalCenter: parent.verticalCenter
                sync: cell.member.sync ?? ""
            }
        }
    }

    Component {
        id: epochCell

        Mono {
            readonly property var member: parent ? parent.rowItem.modelData : ({})
            verticalAlignment: Text.AlignVCenter
            // -1 until this member has been heard from.
            text: (member.epoch ?? -1) >= 0 ? member.epoch : "—"
        }
    }

    Component {
        id: fingerprintCell

        Mono {
            readonly property var member: parent ? parent.rowItem.modelData : ({})
            verticalAlignment: Text.AlignVCenter
            text: member.epochFingerprint || "—"
            elide: Text.ElideRight
        }
    }
    // qmllint enable missing-property
}
