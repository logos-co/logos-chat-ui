import QtQuick
import QtQuick.Layouts

import Logos.Theme
import Logos.Controls

// The thread composer: a multi-line text area plus a Send button, sharing the
// trim/guard/clear contract the thread relies on. Enter sends; Shift+Enter
// inserts a newline. The area grows with its content up to maxLines, then
// scrolls internally. Emits submitted() with the trimmed text.
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

    // The area grows from one line up to this many, then scrolls.
    readonly property int maxLines: 4

    implicitWidth: 200
    implicitHeight: layout.implicitHeight + 2 * Theme.spacing.small
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

    FontMetrics {
        id: fontMetrics
        font: field.font
    }

    RowLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: Theme.spacing.small
        spacing: Theme.spacing.small

        LogosScrollView {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignBottom
            readonly property real chromeHeight: field.topPadding + field.bottomPadding
            readonly property real oneLine: fontMetrics.height + chromeHeight
            readonly property real maxHeight: root.maxLines * fontMetrics.height + chromeHeight
            Layout.preferredHeight: Math.max(oneLine, Math.min(field.implicitHeight, maxHeight))

            LogosTextArea {
                id: field
                enabled: root.submitEnabled
                placeholderText: root.submitEnabled ? root.placeholder : root.disabledPlaceholder

                // Enter sends; Shift+Enter falls through to insert a newline.
                Keys.onReturnPressed: function (event) {
                    if (event.modifiers & Qt.ShiftModifier) {
                        event.accepted = false;
                    } else {
                        root._submit();
                        event.accepted = true;
                    }
                }
                Keys.onEnterPressed: function (event) {
                    if (event.modifiers & Qt.ShiftModifier) {
                        event.accepted = false;
                    } else {
                        root._submit();
                        event.accepted = true;
                    }
                }
            }
        }

        LogosButton {
            Layout.preferredWidth: 84
            Layout.preferredHeight: 36
            Layout.alignment: Qt.AlignBottom
            text: root.buttonText
            enabled: root.submitEnabled && field.text.trim() !== ""
            onClicked: root._submit()
        }
    }
}
