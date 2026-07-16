import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// Modal dialog that collects a name and an optional description for a new group
// and emits them. Open it and connect groupDetailsEntered(); it clears its
// inputs whenever it closes. Standalone: instantiate with no context, open(),
// and read the signal.
LogosDialog {
    id: root

    // Emitted with the trimmed name and description once the user confirms a
    // non-empty name.
    signal groupDetailsEntered(string name, string description)

    title: qsTr("New Group")
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
            implicitWidth: 96
            implicitHeight: 36
            //: Button that confirms creating the new group
            text: qsTr("Create")
            enabled: nameField.text.trim() !== ""
            onClicked: root._accept()
        }
    ]

    onOpened: nameField.forceActiveFocus()
    onClosed: {
        nameField.text = "";
        descriptionField.clear();
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacing.medium

        LogosText {
            text: qsTr("Group name")
            color: Theme.palette.textSecondary
            font.pixelSize: Theme.typography.secondaryText
            Layout.fillWidth: true
        }

        LogosTextField {
            id: nameField
            objectName: "groupNameField"
            Layout.fillWidth: true
            placeholderText: qsTr("e.g. Book Club")

            // Enter confirms rather than moving focus: a valid group needs only
            // the name, and the description is optional.
            Connections {
                target: nameField.textInput
                function onAccepted() {
                    root._accept();
                }
            }
        }

        LogosText {
            text: qsTr("Description (optional)")
            color: Theme.palette.textSecondary
            font.pixelSize: Theme.typography.secondaryText
            Layout.fillWidth: true
        }

        LogosScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: 80

            LogosTextArea {
                id: descriptionField
                placeholderText: qsTr("What's this group about?")
            }
        }
    }

    // Emit the trimmed name and description and close. No-op on an empty name, so
    // neither Enter nor Create can create a group without one.
    function _accept() {
        const name = nameField.text.trim();
        if (name === "")
            return;
        root.groupDetailsEntered(name, descriptionField.text.trim());
        root.close();
    }
}
