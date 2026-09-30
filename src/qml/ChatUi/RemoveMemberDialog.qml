import QtQuick
import QtQuick.Controls

import Logos.Theme
import Logos.Controls

// Asks before removing a member from a group, saying that the group votes on
// it first. Set the member it acts on, open it, and connect confirmed(); it
// closes on either answer. Standalone: instantiate with no context, open(), and
// read the signal.
LogosWarningDialog {
    id: root

    // What the removal acts on, set before opening: the group, and the
    // member's address and the label it is shown by.
    property string conversationId: ""
    property string address: ""
    property string label: ""
    // The group's members, each of whom votes on the removal.
    property int memberCount: 0

    signal confirmed(string conversationId, string address)

    //: Title of the dialog that confirms removing a member from a group
    title: qsTr("Remove %1?").arg(root.label)
    message: {
        //: How a removal goes, in the dialog that confirms it
        const vote = qsTr("The group votes on it first, which takes a minute or more. %1 stays a member until then.").arg(root.label);
        if (root.memberCount !== 2)
            return vote;
        //: Added to the removal dialog in a group of two, where the member removed votes too
        return vote + "\n\n" + qsTr("In a group of two it needs a vote from %1 as well, so it only goes through while %1 is online.").arg(root.label);
    }
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
            //: Button that asks the group to remove the member
            text: qsTr("Remove")
            onClicked: {
                root.confirmed(root.conversationId, root.address);
                root.close();
            }
        }
    ]
}
