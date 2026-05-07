import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

// Shows the local identity name, the delivery_module connection state, and
// a "Copy intro bundle" button.
Item {
    id: root

    property string installationName: ""
    property string deliveryState: "starting"   // "starting"|"online"|"error"|"stopped"|other
    property string deliveryDetail: ""

    signal introBundleRequested()

    function _stateColor(s) {
        if (s === "online") return "#2ecc71"
        if (s === "starting") return "#f1c40f"
        if (s === "error") return "#e74c3c"
        if (s === "stopped") return "#95a5a6"
        return "#95a5a6"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            spacing: 6
            Layout.fillWidth: true

            Label {
                text: "Identity"
                font.bold: true
                font.pixelSize: 13
                color: "#888"
                Layout.fillWidth: true
            }

            Rectangle {
                width: 10
                height: 10
                radius: 5
                color: root._stateColor(root.deliveryState)
                ToolTip.visible: hover.containsMouse
                ToolTip.text: root.deliveryState
                    + (root.deliveryDetail.length > 0
                        ? (" — " + root.deliveryDetail)
                        : "")
                MouseArea {
                    id: hover
                    anchors.fill: parent
                    hoverEnabled: true
                }
            }
        }

        Label {
            id: nameLabel
            text: root.installationName || "…"
            font.pixelSize: 15
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            Layout.fillWidth: true
        }

        Button {
            text: "Copy intro bundle"
            Layout.fillWidth: true
            enabled: root.deliveryState === "online"
            onClicked: root.introBundleRequested()
        }

        Item { Layout.fillHeight: true }
    }
}
