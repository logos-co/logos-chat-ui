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
    id: testRoot
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
            lastActivityDisplay: "12:34"
            preview: "See you tomorrow"
        }
        ListElement {
            conversationId: "c2"
            displayName: "Team"
            isGroup: true
            unreadCount: 42
            lastActivityDisplay: "12:34"
            preview: "Alice: shipping the new theme"
        }
    }
    ListModel {
        id: messagesMock
        ListElement {
            sender: "Alice"
            content: "Hi there"
            timestamp: 0
            isMe: false
            sameSenderAsPrevious: false
            showDaySeparator: true
            dayLabel: "Today"
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
        id: messageComposerC
        MessageComposer {
            placeholder: "Type a message..."
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
        id: groupInfoDialogC
        GroupInfoDialog {
            groupName: "Book Club"
            memberCount: 3
            conversationId: "0xconversationid"
        }
    }
    Component {
        id: addMemberDialogC
        AddMemberDialog {}
    }
    Component {
        id: conversationDelegateC
        ConversationDelegate {
            conversationId: "c1"
            displayName: "Alice"
            isGroup: true
            unreadCount: 3
            lastActivityDisplay: "12:34"
            preview: "See you tomorrow"
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
            sameSenderAsPrevious: false
            showDaySeparator: true
            dayLabel: "Today"
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
    SignalSpy {
        id: groupInfoSpy
        signalName: "groupInfoRequested"
    }
    SignalSpy {
        id: addMemberReqSpy
        signalName: "addMemberRequested"
    }

    TestCase {
        name: "ChatUiComponents"
        when: windowShown

        function instantiate(comp) {
            const obj = createTemporaryObject(comp, this);
            verify(obj, "component failed to instantiate standalone");
            return obj;
        }

        // Find a named field anywhere under a dialog's content, descending both
        // the visual children and a Control/ScrollView contentItem, so a field
        // nested inside a LogosScrollView is still reachable.
        function findField(obj, objectName) {
            if (!obj)
                return null;
            if (obj.objectName === objectName)
                return obj;
            const pools = [obj.children, obj.contentItem ? [obj.contentItem] : []];
            for (let p = 0; p < pools.length; ++p) {
                const pool = pools[p];
                for (let i = 0; pool && i < pool.length; ++i) {
                    const found = findField(pool[i], objectName);
                    if (found)
                        return found;
                }
            }
            return null;
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

        // The group info dialog renders with both an empty description (the "No
        // description" fallback) and a long one.
        function test_groupInfoDialog() {
            const dlg = instantiate(groupInfoDialogC);
            compare(dlg.description, "", "starts with no description");
            dlg.open();
            dlg.description = "A fairly long group description ".repeat(10);
            dlg.close();
        }

        // The Details button shows only for a group and requests the info dialog.
        // Parented into the shown root and sized so effective visibility tracks
        // the currentIsGroup binding.
        function test_threadPaneDetailsButton() {
            const pane = createTemporaryObject(messageThreadPaneC, testRoot);
            verify(pane, "the thread pane must instantiate");
            pane.width = 400;
            pane.height = 300;
            const details = findField(pane, "detailsButton");
            verify(details, "the details button must be reachable");
            verify(!details.visible, "Details is hidden for a direct conversation");

            pane.currentIsGroup = true;
            verify(details.visible, "Details shows for a group");

            groupInfoSpy.target = pane;
            groupInfoSpy.clear();
            details.clicked();
            compare(groupInfoSpy.count, 1, "clicking Details requests the group info");
        }

        // Every pane header is a fixed 48px, subtitle or not, so panes stay
        // aligned across the window.
        function test_paneHeaderSubtitle() {
            const plain = instantiate(paneHeaderC);
            compare(plain.implicitHeight, 48, "a title-only header is 48px");
            const withSubtitle = instantiate(paneHeaderSubtitleC);
            compare(withSubtitle.implicitHeight, 48, "a subtitled header is 48px too");
        }

        function test_delegatesInstantiate() {
            instantiate(conversationDelegateC);
            instantiate(messageBubbleC);
            instantiate(memberDelegateC);
        }

        // The sender label shows on the first message of a run in a group, is
        // suppressed on continuations, and never shows on own messages.
        function test_messageBubbleGrouping() {
            const bubble = instantiate(messageBubbleC);
            verify(bubble.showSender, "first message of a run in a group shows the sender");
            bubble.sameSenderAsPrevious = true;
            verify(!bubble.showSender, "a continuation hides the sender label");
            bubble.sameSenderAsPrevious = false;
            bubble.isMe = true;
            verify(!bubble.showSender, "own messages never show a sender label");
        }

        // The current conversation highlights; a different one does not.
        function test_conversationDelegateHighlight() {
            const del = instantiate(conversationDelegateC);
            verify(del.highlighted, "the current conversation should be highlighted");
            del.currentConversationId = "other";
            verify(!del.highlighted, "a non-current conversation should not be highlighted");
        }

        // The composer trims, emits, and clears; a blank entry and a gated-off
        // composer are both no-ops. The Enter/Shift+Enter handling is declarative
        // in the field's key handlers and not key-simulated here.
        function test_messageComposerSubmits() {
            const composer = instantiate(messageComposerC);
            submitSpy.target = composer;
            submitSpy.clear();

            composer.text = "   ";
            composer._submit();
            compare(submitSpy.count, 0, "a blank message must not submit");

            composer.text = "  hello  ";
            composer._submit();
            compare(submitSpy.count, 1, "a non-blank message submits once");
            compare(submitSpy.signalArguments[0][0], "hello", "the submitted text is trimmed");
            compare(composer.text, "", "the field clears after submit");

            composer.submitEnabled = false;
            composer.text = "blocked";
            composer._submit();
            compare(submitSpy.count, 1, "a gated-off composer does not submit");
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

        // A dismissed group dialog keeps its draft; only a successful accept
        // clears it. Create is gated on both length limits, and Enter cannot
        // bypass them.
        function test_newGroupDialogDraftAndLimits() {
            const dlg = createTemporaryObject(newGroupDialogC, this);
            verify(dlg);
            groupDetailsSpy.target = dlg;
            groupDetailsSpy.clear();

            const nameField = findField(dlg.contentItem, "groupNameField");
            const descField = findField(dlg.contentItem, "groupDescriptionField");
            verify(nameField && descField, "both fields must be reachable");
            const create = dlg.rightActions[0];

            dlg.open();
            nameField.text = "Draft name";
            descField.text = "Draft description";
            dlg.close();
            compare(nameField.text, "Draft name", "closing keeps the name draft");
            compare(descField.text, "Draft description", "closing keeps the description draft");

            dlg.open();
            nameField.text = "x".repeat(dlg.nameLimit + 1);
            verify(!create.enabled, "Create is disabled over the name limit");
            dlg._accept();
            compare(groupDetailsSpy.count, 0, "Enter must not accept over the name limit");

            nameField.text = "Valid";
            descField.text = "y".repeat(dlg.descriptionLimit + 1);
            verify(!create.enabled, "Create is disabled over the description limit");

            descField.text = "ok";
            dlg._accept();
            compare(groupDetailsSpy.count, 1, "valid input accepts once");
            compare(nameField.text, "", "accept clears the name");
            compare(descField.text, "", "accept clears the description");
        }

        // A dismissed conversation dialog keeps its draft; a successful accept
        // clears it.
        function test_newConversationDialogDraft() {
            const dlg = createTemporaryObject(newConvDialogC, this);
            verify(dlg);
            addressSpy.target = dlg;
            addressSpy.clear();
            const field = findField(dlg.contentItem, "convAddressField");
            verify(field, "the address field must be reachable");

            dlg.open();
            field.text = "0xdraft";
            dlg.close();
            compare(field.text, "0xdraft", "closing keeps the address draft");

            dlg.open();
            dlg._accept();
            compare(addressSpy.count, 1, "a non-blank draft accepts");
            compare(field.text, "", "accept clears the address");
        }

        // The add-member dialog gates Add on non-blank input, keeps its draft
        // across a close, and clears on accept.
        function test_addMemberDialog() {
            const dlg = createTemporaryObject(addMemberDialogC, this);
            verify(dlg);
            addressSpy.target = dlg;
            addressSpy.clear();
            const field = findField(dlg.contentItem, "addMemberField");
            verify(field, "the address field must be reachable");

            dlg.open();
            dlg._accept();
            compare(addressSpy.count, 0, "an empty address must not be accepted");

            field.text = "0xpeer";
            dlg.close();
            compare(field.text, "0xpeer", "closing keeps the address draft");

            dlg.open();
            dlg._accept();
            compare(addressSpy.count, 1, "a non-blank draft accepts");
            compare(addressSpy.signalArguments[0][0], "0xpeer", "the address is emitted");
            compare(field.text, "", "accept clears the address");
        }

        // The Add member button reflects connectivity and requests a member add.
        function test_membersPaneAddButton() {
            const pane = instantiate(membersPaneC);
            const btn = findField(pane, "addMemberButton");
            verify(btn, "the add-member button must be reachable");
            verify(btn.enabled, "enabled when online");
            pane.online = false;
            verify(!btn.enabled, "disabled when offline");
            pane.online = true;

            addMemberReqSpy.target = pane;
            addMemberReqSpy.clear();
            btn.clicked();
            compare(addMemberReqSpy.count, 1, "clicking requests adding a member");
        }

        // A copy flashes a brief confirmation on the row that then clears.
        function test_memberDelegateCopiedFlash() {
            const del = instantiate(memberDelegateC);
            del.pending = false;
            verify(!del.copiedFlashing, "not flashing at rest");
            del.flashCopied();
            verify(del.copiedFlashing, "flashing right after a copy");
            tryCompare(del, "copiedFlashing", false, 3000, "the flash clears");
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
