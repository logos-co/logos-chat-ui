pragma ComponentBehavior: Bound

import QtQuick
import QtTest

import ChatUi

// Standalone-instantiability + behaviour tests for the ChatUi components. Run
// headless with qmltestrunner, giving it src/qml on the import path (so `import
// ChatUi` resolves) and the design system's lib dir (so Logos.* resolves):
//   qmltestrunner -input tests/qml -import src/qml -import <app>/lib -platform offscreen
// Every component must instantiate with mock data and no host `logos` context —
// that is the house-rule-2 proof; a broken import or a misused Logos API surfaces
// here as a null object.
Item {
    width: 400
    height: 400

    // ── mock data ────────────────────────────────────────────────────────
    ListModel {
        id: conversationsMock
        ListElement {
            conversationId: "c1"
            displayName: "Alice"
            isGroup: false
            unreadCount: 0
            lastActivity: 0
        }
        ListElement {
            conversationId: "c2"
            displayName: "Team"
            isGroup: true
            unreadCount: 42
            lastActivity: 0
        }
    }
    ListModel {
        id: messagesMock
        ListElement {
            sender: "Alice"
            content: "Hi there"
            timestamp: 0
            isMe: false
        }
    }
    ListModel {
        id: membersMock
        ListElement {
            address: "0xabc"
            label: "Alice"
            isSelf: false
            pending: false
        }
    }

    // ── components under test ────────────────────────────────────────────
    Component {
        id: conversationsPaneC
        ConversationsPane {
            conversationModel: conversationsMock
            currentConversationId: "c1"
            online: true
        }
    }
    Component {
        id: messageThreadPaneC
        MessageThreadPane {
            messageModel: messagesMock
            currentIsGroup: false
            title: "Alice"
            conversationId: "c1"
            hasConversation: true
            online: true
        }
    }
    Component {
        id: membersPaneC
        MembersPane {
            memberModel: membersMock
            online: true
        }
    }
    Component {
        id: statusBarC
        StatusBar {
            statusMessage: "Ready"
            statusLabel: "Online"
            online: true
            identity: "0xdeadbeef"
        }
    }
    Component {
        id: paneHeaderC
        PaneHeader {
            title: "Header"
        }
    }
    Component {
        id: paneHeaderSubtitleC
        PaneHeader {
            title: "Header"
            subtitle: "A subtitle line"
        }
    }
    Component {
        id: emptyStateC
        EmptyState {
            text: "Nothing here"
        }
    }
    Component {
        id: submitRowC
        SubmitRow {
            placeholder: "type..."
            buttonText: "Send"
            submitEnabled: true
        }
    }
    Component {
        id: toastC
        Toast {}
    }
    Component {
        id: clipboardProxyC
        ClipboardProxy {}
    }
    Component {
        id: newConvDialogC
        NewConversationDialog {}
    }
    Component {
        id: newGroupDialogC
        NewGroupDialog {}
    }
    Component {
        id: addressDialogC
        AddressDialog {}
    }
    Component {
        id: memberAddInfoDialogC
        MemberAddInfoDialog {}
    }
    Component {
        id: conversationDelegateC
        ConversationDelegate {
            conversationId: "c1"
            displayName: "Alice"
            isGroup: true
            unreadCount: 3
            lastActivity: 0
            currentConversationId: "c1"
        }
    }
    Component {
        id: messageBubbleC
        MessageBubble {
            content: "<b>not bold</b>"
            isMe: false
            timestamp: 0
            sender: "Alice"
            groupContext: true
        }
    }
    Component {
        id: memberDelegateC
        MemberDelegate {
            label: "Carol"
            address: "0xcarol"
            isSelf: false
            pending: true
        }
    }

    SignalSpy {
        id: submitSpy
        signalName: "submitted"
    }
    SignalSpy {
        id: addressSpy
        signalName: "addressEntered"
    }
    SignalSpy {
        id: confirmedSpy
        signalName: "confirmed"
    }
    SignalSpy {
        id: groupDetailsSpy
        signalName: "groupDetailsEntered"
    }

    TestCase {
        name: "ChatUiComponents"
        when: windowShown

        function instantiate(comp) {
            const obj = createTemporaryObject(comp, this);
            verify(obj, "component failed to instantiate standalone");
            return obj;
        }

        function test_panesInstantiate() {
            instantiate(conversationsPaneC);
            instantiate(messageThreadPaneC);
            instantiate(membersPaneC);
            instantiate(statusBarC);
        }

        function test_leafComponentsInstantiate() {
            instantiate(paneHeaderC);
            instantiate(emptyStateC);
            instantiate(submitRowC);
            instantiate(toastC);
            instantiate(clipboardProxyC);
        }

        // copy() writes without throwing; an empty string is a no-op.
        function test_clipboardProxyCopies() {
            const clip = instantiate(clipboardProxyC);
            clip.copy("");
            clip.copy("0xdeadbeef");
        }

        function test_dialogsInstantiate() {
            instantiate(newConvDialogC);
            instantiate(newGroupDialogC);
            instantiate(addressDialogC);
            instantiate(memberAddInfoDialogC);
        }

        // A title-only header keeps its 48px height, so panes that set no
        // subtitle are unchanged; a subtitled header instantiates and is no
        // shorter.
        function test_paneHeaderSubtitle() {
            const plain = instantiate(paneHeaderC);
            compare(plain.implicitHeight, 48, "a title-only header stays 48px");
            const withSubtitle = instantiate(paneHeaderSubtitleC);
            verify(withSubtitle.implicitHeight >= 48, "a subtitled header is at least as tall");
        }

        function test_delegatesInstantiate() {
            instantiate(conversationDelegateC);
            instantiate(messageBubbleC);
            instantiate(memberDelegateC);
        }

        // The current conversation highlights; a different one does not.
        function test_conversationDelegateHighlight() {
            const del = instantiate(conversationDelegateC);
            verify(del.highlighted, "the current conversation should be highlighted");
            del.currentConversationId = "other";
            verify(!del.highlighted, "a non-current conversation should not be highlighted");
        }

        // SubmitRow trims, emits, and clears; a blank entry is a no-op. This is
        // the shared composer/member-add behaviour.
        function test_submitRowTrimsEmitsClears() {
            const row = instantiate(submitRowC);
            submitSpy.target = row;
            submitSpy.clear();

            row.text = "   ";
            row._submit();
            compare(submitSpy.count, 0, "a blank entry must not submit");

            row.text = "  hello  ";
            row._submit();
            compare(submitSpy.count, 1, "a non-blank entry must submit once");
            compare(submitSpy.signalArguments[0][0], "hello", "the submitted text must be trimmed");
            compare(row.text, "", "the field must clear after submit");
        }

        // The dialog only enables Create once there is non-blank input, and emits
        // the trimmed address.
        function test_newConversationDialogValidation() {
            const dlg = createTemporaryObject(newConvDialogC, this);
            verify(dlg);
            addressSpy.target = dlg;
            addressSpy.clear();
            dlg.open();
            dlg._accept();
            compare(addressSpy.count, 0, "an empty address must not be accepted");
            dlg.close();
        }

        // The group dialog gates on a non-blank name: an empty name is a no-op,
        // a filled name emits once with the trimmed name, and an unentered
        // description comes through empty.
        function test_newGroupDialogValidation() {
            const dlg = createTemporaryObject(newGroupDialogC, this);
            verify(dlg);
            groupDetailsSpy.target = dlg;
            groupDetailsSpy.clear();
            dlg.open();

            dlg._accept();
            compare(groupDetailsSpy.count, 0, "an empty name must not be accepted");

            let nameField = null;
            const kids = dlg.contentItem.children;
            for (let i = 0; i < kids.length; ++i) {
                if (kids[i].objectName === "groupNameField") {
                    nameField = kids[i];
                    break;
                }
            }
            verify(nameField, "the group name field must be reachable");
            nameField.text = "  Book Club  ";
            dlg._accept();
            compare(groupDetailsSpy.count, 1, "a non-blank name must be accepted once");
            compare(groupDetailsSpy.signalArguments[0][0], "Book Club", "the name must be trimmed");
            compare(groupDetailsSpy.signalArguments[0][1], "", "an unentered description is empty");
        }

        // Confirming the explainer emits confirmed() so the caller can proceed
        // with the invite and persist the "don't show again" choice.
        function test_memberAddInfoDialogConfirms() {
            const dlg = instantiate(memberAddInfoDialogC);
            confirmedSpy.target = dlg;
            confirmedSpy.clear();
            dlg.open();
            dlg.rightActions[0].clicked();
            compare(confirmedSpy.count, 1, "confirming must emit confirmed() once");
        }
    }
}
