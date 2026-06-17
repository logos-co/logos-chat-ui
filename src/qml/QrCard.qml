import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

// ─────────────────────────────────────────────────────────────────────────────
// QrCard — drop-in QR card component for ui_qml modules.
//
// Copy this file into your plugin's qml/ directory and instantiate:
//
//     QrCard {
//         title:       "My Chat ID"
//         description: "Scan to start a private chat"
//         payload:     someStringToEncode      // auto-generates when it changes
//     }
//
// Requires the `qr` core module installed. Declare it in your metadata.json:
//     "dependencies": ["qr"]
//
// Override the theme.* colors to match your module's palette (see #8). Defaults = dark.
// The card calls qr.generateCard(title, description, data) and renders title + description
// + the QR, with a "Save as image" button (qr.savePng → ~/Pictures/qr/).
// ─────────────────────────────────────────────────────────────────────────────
Item {
    id: card

    // ── Public API ────────────────────────────────────────────────────────
    property string title:       ""
    property string description:  ""
    property string payload:     ""    // data to encode; auto-generates on change
                                       // (named `payload`, not `data` — `data` is a
                                       //  reserved default property on QML Item)
    property bool   showSaveButton: true

    // ── Theme (override to match the host module) ─────────────────────────
    property color cardBg:      "#171717"
    property color titleColor:  "#FFFFFF"
    property color descColor:   "#A4A4A4"
    property color accent:      "#FF5000"
    property color borderColor: "#383838"
    property color qrBg:        "#FFFFFF"
    property color qrFg:        "#000000"
    property color errorColor:  "#FB3748"
    property color okColor:     "#22C55E"

    // ── Internal state ────────────────────────────────────────────────────
    property int    _n: 0
    property var    _cells: []
    property string _err: ""
    property string _saveMsg: ""
    property bool   _saveOk: false

    implicitWidth: 360
    implicitHeight: bg.height

    onPayloadChanged:     regenerate()
    onTitleChanged:       regenerate()
    onDescriptionChanged: regenerate()
    Component.onCompleted: if (payload.length > 0) regenerate()

    // logos.callModule returns a double-JSON-encoded string; unwrap to an object.
    function callModuleParse(raw) {
        try { var v = JSON.parse(raw); if (typeof v === "string") v = JSON.parse(v); return v }
        catch (e) { return null }
    }

    function regenerate() {
        _err = ""; _saveMsg = ""; _n = 0; _cells = []
        if (!payload || payload.length === 0) return
        if (typeof logos === "undefined") { _err = "Module bridge unavailable."; return }
        var res = callModuleParse(logos.callModule("qr", "generateCard", [title, description, payload]))
        if (!res || !res.ok) {
            _err = (res && res.error) ? res.error : "QR service unavailable — is the 'qr' module installed?"
            return
        }
        _n = res.n; _cells = res.cells
    }

    // Grab the whole card (title + description + QR) to a PNG and save it via the qr core.
    function saveImage() {
        _saveMsg = ""
        if (_n <= 0 || typeof logos === "undefined") return
        var name = "qr-" + Qt.formatDateTime(new Date(), "yyyyMMdd-hhmmss")
        var tmp  = "/tmp/" + name + ".png"
        bg.grabToImage(function(result) {
            if (!result.saveToFile(tmp)) { card._saveOk = false; card._saveMsg = "Could not capture image."; return }
            var res = card.callModuleParse(logos.callModule("qr", "savePng", [tmp, name]))
            if (res && res.ok) { card._saveOk = true;  card._saveMsg = "Saved: " + res.path }
            else               { card._saveOk = false; card._saveMsg = "Save failed: " + (res && res.error ? res.error : "?") }
        })
    }

    Rectangle {
        id: bg
        width: parent.width
        height: col.implicitHeight + 32
        radius: 12
        color: card.cardBg

        ColumnLayout {
            id: col
            x: 16; y: 16
            width: parent.width - 32
            spacing: 12

            Label {
                text: card.title
                visible: card.title.length > 0
                color: card.titleColor
                font.pixelSize: 18; font.bold: true
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }
            Label {
                text: card.description
                visible: card.description.length > 0
                color: card.descColor
                font.pixelSize: 13
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
            }

            // QR frame — fixed square, matrix centred (symmetric quiet zone).
            Rectangle {
                id: frame
                visible: card._n > 0
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 260
                Layout.preferredHeight: 260
                radius: 8
                color: card.qrBg
                readonly property int cell: card._n > 0 ? Math.max(1, Math.floor((width - 32) / card._n)) : 1
                Grid {
                    anchors.centerIn: parent
                    columns: card._n; rows: card._n
                    Repeater {
                        model: card._cells
                        delegate: Rectangle {
                            width:  frame.cell; height: frame.cell
                            color: modelData ? card.qrFg : card.qrBg
                        }
                    }
                }
            }

            Label {
                text: card._err
                visible: card._err.length > 0
                color: card.errorColor; font.pixelSize: 12
                Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
            }

            Button {
                id: saveBtn
                text: "Save as image"
                visible: card.showSaveButton && card._n > 0
                Layout.fillWidth: true; Layout.preferredHeight: 38
                onClicked: card.saveImage()
                contentItem: Text {
                    text: saveBtn.text; color: card.titleColor; font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    radius: 8; color: "transparent"
                    border.color: saveBtn.hovered ? card.accent : card.borderColor; border.width: 1
                }
            }
            Label {
                text: card._saveMsg
                visible: card._saveMsg.length > 0
                color: card._saveOk ? card.okColor : card.errorColor
                font.pixelSize: 11
                Layout.fillWidth: true; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WrapAnywhere
            }
        }
    }
}
