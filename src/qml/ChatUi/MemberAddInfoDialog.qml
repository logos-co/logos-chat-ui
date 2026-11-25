import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// Explainer shown before members are listed: the commit is asynchronous, so new
// members take about two minutes to appear. Open it, and connect
// confirmed() — it carries whether the user ticked "Don't show this again", so the
// caller can persist that and skip the dialog next time. Standalone: instantiate
// with no context, open(), and read the signal.
LogosDialog {
    id: root

    // Emitted when the user confirms the add; `dontShowAgain` is the checkbox state.
    signal confirmed(bool dontShowAgain)

    title: qsTr("Adding members")
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    anchors.centerIn: Overlay.overlay
    width: Math.min(480, (Overlay.overlay ? Overlay.overlay.width : 480) - 2 * Theme.spacing.large)

    leftActions: [
        LogosButton {
            implicitWidth: 96
            implicitHeight: 36
            text: qsTr("Cancel")
            onClicked: root.close()
        }
    ]
    rightActions: [
        LogosButton {
            implicitWidth: 130
            implicitHeight: 36
            //: Button that goes on to list the members to invite
            text: qsTr("Continue")
            onClicked: {
                root.confirmed(dontShowAgain.checked);
                root.close();
            }
        }
    ]

    contentItem: ColumnLayout {
        spacing: Theme.spacing.medium

        LogosText {
            //: Explains that a new member shows up only after the group updates
            text: qsTr("Adding members isn't instant. Membership updates apply on a timer, so new members take about two minutes to appear. Until then, adding more fails, so list everyone you want to invite before adding them.")
            color: Theme.palette.textSecondary
            font.pixelSize: Theme.typography.secondaryText
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        LogosCheckbox {
            id: dontShowAgain
            text: qsTr("Don't show this again")
        }
    }
}
