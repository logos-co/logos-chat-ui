import QtQuick
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// A text field plus a submit button with the trim/guard/clear behaviour the
// composer and member-add row share. Emits submitted() with the trimmed text.
Rectangle {
    id: root

    property string placeholder: ""
    // Placeholder shown while submitEnabled is false (e.g. "offline").
    property string disabledPlaceholder: ""
    // External gate (connectivity, a selected conversation, ...). The button is
    // additionally disabled while the trimmed text is empty.
    property bool submitEnabled: true
    property string buttonText: qsTr("Send")
    property alias text: field.text

    signal submitted(string text)

    implicitWidth: 200
    implicitHeight: 52
    color: Theme.palette.backgroundTertiary

    // Emit the trimmed text and clear. No-op when gated off or empty, so neither
    // Enter nor the button can submit blank input.
    function _submit() {
        const value = field.text.trim();
        if (value === "" || !root.submitEnabled)
            return;
        root.submitted(value);
        field.text = "";
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacing.small
        spacing: Theme.spacing.small

        LogosTextField {
            id: field
            Layout.fillWidth: true
            Layout.fillHeight: true
            enabled: root.submitEnabled
            placeholderText: root.submitEnabled ? root.placeholder : root.disabledPlaceholder

            Connections {
                target: field.textInput
                function onAccepted() {
                    root._submit();
                }
            }
        }

        LogosButton {
            Layout.preferredWidth: 84
            Layout.fillHeight: true
            text: root.buttonText
            enabled: root.submitEnabled && field.text.trim() !== ""
            onClicked: root._submit()
        }
    }
}
