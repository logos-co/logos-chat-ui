import QtQuick
import QtQuick.Controls

import Logos.Theme
import Logos.Controls

// Transient error notification: a top-center banner in the window overlay,
// above modal dialogs and clear of the input affordances along the window's
// bottom edge. Call show(message); it auto-dismisses after a few seconds, a
// tap dismisses it sooner. A new show() replaces the current message.
// TODO(upstream: logos-design-system): a shared toast/notification component.
Popup {
    id: root

    property string message: ""

    function show(text) {
        root.message = text;
        root.open();
        dismissTimer.restart();
    }

    parent: Overlay.overlay
    modal: false
    focus: false
    closePolicy: Popup.NoAutoClose
    leftPadding: Theme.spacing.large
    rightPadding: Theme.spacing.large
    topPadding: Theme.spacing.small
    bottomPadding: Theme.spacing.small
    width: Math.min(implicitWidth, (parent ? parent.width : 480) - 2 * Theme.spacing.xxlarge)
    x: parent ? (parent.width - width) / 2 : 0
    y: Theme.spacing.xxlarge

    background: Rectangle {
        color: Theme.colors.getColor(Theme.palette.error, 0.15)
        border.color: Theme.palette.error
        border.width: 1
        radius: Theme.spacing.radiusMedium
    }

    contentItem: LogosText {
        text: root.message
        textFormat: Text.PlainText
        color: Theme.palette.error
        font.pixelSize: Theme.typography.secondaryText
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter

        TapHandler {
            onTapped: root.close()
        }
    }

    Timer {
        id: dismissTimer
        interval: 4000
        onTriggered: root.close()
    }
}
