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
            description: ""
        }
        ListElement {
            conversationId: "c2"
            displayName: "Team"
            isGroup: true
            unreadCount: 42
            lastActivityDisplay: "12:34"
            preview: "Alice: shipping the new theme"
            description: "Shipping the new theme"
        }
    }
    ListModel {
        id: messagesMock
        ListElement {
            sender: "Alice"
            content: "Hi there"
            timeDisplay: "12:34"
            isMe: false
            sameSenderAsPrevious: false
            showDaySeparator: true
            dayLabel: "Today"
        }
    }
    ListModel {
        id: emptyMessagesMock
    }
    ListModel {
        id: emptyMembersMock
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
            ready: true
        }
    }
    Component {
        id: emptyMembersPaneC
        MembersPane {
            memberModel: emptyMembersMock
            online: true
            ready: true
        }
    }
    Component {
        id: emptyThreadPaneC
        MessageThreadPane {
            messageModel: emptyMessagesMock
            currentIsGroup: false
            title: "Alice"
            conversationId: "c1"
            hasConversation: true
            online: true
            ready: true
        }
    }
    Component {
        id: membersPaneC
        MembersPane {
            memberModel: membersMock
            online: true
            ready: true
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
        id: dmInfoDialogC
        DmInfoDialog {
            conversationId: "0xconversationid"
        }
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
            description: "Ship it"
            currentConversationId: "c1"
        }
    }
    Component {
        id: messageBubbleC
        MessageBubble {
            content: "<b>not bold</b>"
            isMe: false
            timeDisplay: "12:34"
            sender: "Alice"
            groupContext: true
            sameSenderAsPrevious: false
            showDaySeparator: true
            dayLabel: "Today"
        }
    }
    Component {
        id: messageSkeletonC
        MessageSkeleton {}
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
        id: detailsSpy
        signalName: "detailsRequested"
    }
    SignalSpy {
        id: addMemberReqSpy
        signalName: "addMemberRequested"
    }
    SignalSpy {
        id: conversationSelectedSpy
        signalName: "conversationSelected"
    }
    SignalSpy {
        id: newConversationSpy
        signalName: "newConversationRequested"
    }
    SignalSpy {
        id: newGroupSpy
        signalName: "newGroupRequested"
    }
    SignalSpy {
        id: showAddressSpy
        signalName: "showAddressRequested"
    }
    SignalSpy {
        id: copyRequestedSpy
        signalName: "copyRequested"
    }

    TestCase {
        name: "ChatUiComponents"
        when: windowShown

        function instantiate(comp) {
            const obj = createTemporaryObject(comp, this);
            verify(obj, "component failed to instantiate standalone");
            return obj;
        }

        // Find a named field anywhere under an item, descending the visual
        // children, a Control/ScrollView contentItem, and the non-visual data, so
        // a field inside a LogosScrollView or an entry inside a menu popup is
        // still reachable.
        function findField(obj, objectName) {
            if (!obj)
                return null;
            if (obj.objectName === objectName)
                return obj;
            const pools = [obj.children, obj.contentItem ? [obj.contentItem] : [], obj.contentData || [], obj.data || []];
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
            instantiate(messageSkeletonC);
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
            instantiate(dmInfoDialogC);
        }

        // A direct conversation's details offer the peer's address once it is
        // known, and the conversation id either way.
        function test_dmInfoDialogAddressRow() {
            const dlg = instantiate(dmInfoDialogC);
            const copyAddress = findField(dlg, "copyPeerAddressButton");
            const copyId = findField(dlg, "copyDmIdButton");
            const copied = findField(dlg, "copiedLabel");
            verify(copyAddress && copyId && copied, "both copies and the confirmation must be reachable");

            dlg.open();
            verify(!copyAddress.visible, "no address row while the peer is unknown");
            dlg.peerAddress = "0xpeer";
            verify(copyAddress.visible, "the address row appears once known");

            copyAddress.clicked();
            compare(copied.opacity, 1, "copying confirms");
            dlg.close();
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

        // Details is offered for every conversation, a direct one included, and
        // requests the matching dialog. Parented into the shown root and sized so
        // effective visibility is meaningful.
        function test_threadPaneDetailsButton() {
            const pane = createTemporaryObject(messageThreadPaneC, testRoot);
            verify(pane, "the thread pane must instantiate");
            pane.width = 400;
            pane.height = 300;
            const details = findField(pane, "detailsButton");
            verify(details, "the details button must be reachable");
            verify(details.visible, "Details shows for a direct conversation");

            pane.currentIsGroup = true;
            verify(details.visible, "Details shows for a group too");

            detailsSpy.target = pane;
            detailsSpy.clear();
            details.clicked();
            compare(detailsSpy.count, 1, "clicking Details asks for the details");
        }

        // A conversation whose messages have not arrived yet shows neither the
        // rows left behind by the previous one nor a premature empty state; the
        // skeleton waits out the grace interval, and the row count keeps
        // reporting the model.
        function test_threadPaneGatesUnloadedConversation() {
            const pane = createTemporaryObject(messageThreadPaneC, testRoot);
            verify(pane, "the thread pane must instantiate");
            pane.width = 400;
            pane.height = 300;
            const list = findField(pane, "threadList");
            const skeleton = findField(pane, "threadSkeleton");
            const empty = findField(pane, "threadEmptyState");
            verify(list && skeleton && empty, "the thread parts must be reachable");
            verify(list.visible, "a loaded thread shows its rows");
            verify(!skeleton.visible, "a loaded thread shows no skeleton");

            pane.conversationId = "c2";
            pane.ready = false;
            verify(!pane.threadReady, "a conversation still loading is not ready");
            verify(!list.visible, "rows of the previous conversation stay hidden");
            verify(!empty.visible, "no empty state while the thread is loading");
            verify(!skeleton.visible, "no skeleton within the grace interval");
            compare(pane.messageCount, 1, "the row count still reports the model");
            tryCompare(skeleton, "visible", true, 3000, "a longer wait shows the skeleton");

            pane.ready = true;
            verify(list.visible, "the thread shows once its messages are loaded");
            verify(!skeleton.visible, "the skeleton goes with the wait");

            // A refetch under the same conversation is a wait like any other.
            pane.ready = false;
            verify(!skeleton.visible, "a reload gets the grace interval too");
            tryCompare(skeleton, "visible", true, 3000, "and the skeleton after it");
        }

        // An actually empty thread says so, but only after the model has had a
        // moment to fill: the messages arrive on their own replica, a beat after
        // the conversation counts as loaded.
        function test_threadPaneEmptyStateSettles() {
            const pane = createTemporaryObject(emptyThreadPaneC, testRoot);
            verify(pane, "the thread pane must instantiate");
            pane.width = 400;
            pane.height = 300;
            const empty = findField(pane, "threadEmptyState");
            verify(empty, "the empty state must be reachable");

            pane.conversationId = "c2";
            verify(!empty.visible, "an empty model right after a switch says nothing");
            tryCompare(empty, "visible", true, 3000, "a thread that stays empty says so");
        }

        // A roster that has just loaded does not claim to be empty until the
        // model has had a moment to fill, for the same reason the thread waits.
        function test_membersPaneEmptyStateSettles() {
            const pane = createTemporaryObject(emptyMembersPaneC, testRoot);
            verify(pane, "the members pane must instantiate");
            pane.width = 220;
            pane.height = 300;
            const empty = findField(pane, "memberEmptyState");
            verify(empty, "the empty state must be reachable");

            pane.ready = false;
            pane.ready = true;
            verify(!empty.visible, "an empty model right after a switch says nothing");
            tryCompare(empty, "visible", true, 3000, "a roster that stays empty says so");
        }

        // The roster is gated the same way: a switch never shows the previous
        // conversation's members, nor claims the new one has none.
        function test_membersPaneGatesUnloadedRoster() {
            const pane = createTemporaryObject(membersPaneC, testRoot);
            verify(pane, "the members pane must instantiate");
            pane.width = 220;
            pane.height = 300;
            const list = findField(pane, "memberList");
            const empty = findField(pane, "memberEmptyState");
            verify(list && empty, "the roster parts must be reachable");
            verify(list.visible, "a loaded roster shows its rows");

            pane.ready = false;
            verify(!list.visible, "rows of the previous conversation stay hidden");
            verify(!empty.visible, "no empty state while the roster is loading");
        }

        // Selecting a row carries the row's own data, which is what lets a view
        // render the selection before the backend has switched to it.
        function test_conversationsPaneSelectionCarriesTheRow() {
            const pane = createTemporaryObject(conversationsPaneC, testRoot);
            verify(pane, "the sidebar must instantiate");
            pane.width = 260;
            pane.height = 400;

            conversationSelectedSpy.target = pane;
            conversationSelectedSpy.clear();

            const list = findField(pane, "conversationList");
            verify(list, "the conversation list must be reachable");
            tryVerify(() => list.itemAtIndex(1) !== null, 2000, "the second row must be realised");
            list.itemAtIndex(1).clicked();

            compare(conversationSelectedSpy.count, 1, "clicking a row selects it once");
            const args = conversationSelectedSpy.signalArguments[0];
            compare(args[0], "c2", "the conversation id");
            compare(args[1], "Team", "its display name");
            compare(args[2], true, "whether it is a group");
            compare(args[3], "Shipping the new theme", "and its description");
        }

        // A closed composer names why it is closed, so a connected app with
        // nothing selected does not claim to be offline.
        function test_threadPaneDisabledPlaceholder() {
            const pane = createTemporaryObject(messageThreadPaneC, testRoot);
            verify(pane, "the thread pane must instantiate");
            const composer = findField(pane, "composer");
            verify(composer, "the composer must be reachable");

            pane.hasConversation = false;
            compare(composer.disabledPlaceholder, "Select a conversation to start chatting", "online with nothing selected");

            pane.online = false;
            compare(composer.disabledPlaceholder, "Chat not connected", "offline");
        }

        // A refused message comes back to the composer, and a composer the user
        // has already typed into is left alone.
        function test_threadPaneRestoresFailedSend() {
            const pane = createTemporaryObject(messageThreadPaneC, testRoot);
            verify(pane, "the thread pane must instantiate");
            const composer = findField(pane, "composer");
            verify(composer, "the composer must be reachable");

            pane.restoreFailedSend("c1", "lost message");
            compare(composer.text, "lost message", "the refused text comes back");

            composer.text = "already typing";
            pane.restoreFailedSend("c1", "lost message");
            compare(composer.text, "already typing", "a composer in use is left alone");

            composer.text = "";
            pane.restoreFailedSend("other", "not mine");
            compare(composer.text, "", "a refusal from another conversation stays out");
        }

        // A status message is an announcement: it shows, then goes quiet, while
        // the connectivity label stays.
        function test_statusBarMessageExpires() {
            const bar = instantiate(statusBarC);
            bar.messageTimeout = 200;
            const label = findField(bar, "statusMessageLabel");
            verify(label, "the status message label must be reachable");

            bar.statusMessage = "Address ready";
            compare(label.text, "Address ready", "a fresh message shows");
            tryCompare(label, "text", "", 3000, "the message goes quiet");
            compare(bar.statusLabel, "Online", "connectivity is state and stays");
        }

        // The roster line counts members in plain English and names invitations
        // that have not landed yet.
        function test_groupInfoDialogMemberSummary() {
            const dlg = instantiate(groupInfoDialogC);
            dlg.memberCount = 1;
            dlg.pendingMemberCount = 0;
            compare(dlg.memberSummary, "1 member", "a single member");
            dlg.memberCount = 3;
            compare(dlg.memberSummary, "3 members", "several members");
            dlg.pendingMemberCount = 1;
            compare(dlg.memberSummary, "3 members, 1 invited", "an outstanding invitation");
            dlg.pendingMemberCount = 2;
            compare(dlg.memberSummary, "3 members, 2 invited", "several outstanding invitations");
        }

        // The header's New menu offers both kinds of conversation, and picking
        // one requests it.
        function test_conversationsPaneNewMenu() {
            const pane = createTemporaryObject(conversationsPaneC, testRoot);
            verify(pane, "the sidebar must instantiate");
            pane.width = 260;
            pane.height = 400;

            const button = findField(pane, "newMenuButton");
            verify(button, "the New button must be reachable");
            const menu = findField(pane, "newMenu");
            verify(menu && !menu.opened, "the menu starts closed");
            button.clicked();
            tryVerify(() => menu.opened, 2000, "clicking New opens the menu");
            menu.close();

            const dm = findField(pane, "newDmMenuItem");
            const group = findField(pane, "newGroupMenuItem");
            verify(dm && group, "both menu entries must be reachable");

            newConversationSpy.target = pane;
            newConversationSpy.clear();
            newGroupSpy.target = pane;
            newGroupSpy.clear();

            dm.triggered();
            compare(newConversationSpy.count, 1, "the DM entry requests a direct message");
            group.triggered();
            compare(newGroupSpy.count, 1, "the group entry requests a group");
        }

        // The address action moved into the header and still asks for it.
        function test_conversationsPaneShowsAddress() {
            const pane = createTemporaryObject(conversationsPaneC, testRoot);
            verify(pane, "the sidebar must instantiate");
            const btn = findField(pane, "showAddressButton");
            verify(btn, "the address button must be reachable");
            verify(btn.enabled, "enabled when online");
            pane.online = false;
            verify(!btn.enabled, "disabled when offline");

            showAddressSpy.target = pane;
            showAddressSpy.clear();
            btn.clicked();
            compare(showAddressSpy.count, 1, "clicking asks for the address");
        }

        // The copy actions sit next to the values they copy, and confirm.
        function test_addressDialogCopyButton() {
            const dlg = instantiate(addressDialogC);
            dlg.addressText = "0xdeadbeef";
            const btn = findField(dlg, "copyMyAddressButton");
            const copied = findField(dlg, "copiedLabel");
            verify(btn && copied, "the copy button and its confirmation must be reachable");
            compare(dlg.rightActions.length, 1, "only Close is left in the action row");

            compare(copied.opacity, 0, "nothing confirmed yet");
            btn.clicked();
            compare(copied.opacity, 1, "the copy is confirmed at the value");
        }

        function test_groupInfoDialogCopyButton() {
            const dlg = instantiate(groupInfoDialogC);
            const btn = findField(dlg, "copyGroupIdButton");
            const copied = findField(dlg, "copiedLabel");
            verify(btn && copied, "the copy button and its confirmation must be reachable");
            compare(dlg.rightActions.length, 1, "only Close is left in the action row");

            btn.clicked();
            compare(copied.opacity, 1, "the copy is confirmed at the value");
        }

        // A roster row offers a copy button, hidden until the row is hovered so a
        // resting roster stays clean; using it flashes the same confirmation a
        // row click does.
        function test_memberDelegateCopyButton() {
            const del = instantiate(memberDelegateC);
            del.pending = false;
            const btn = findField(del, "copyMemberAddressButton");
            verify(btn, "the copy button must be reachable");

            tryCompare(btn, "opacity", 0, 2000, "the copy button is hidden until the row is hovered");

            copyRequestedSpy.target = del;
            copyRequestedSpy.clear();
            btn.clicked();
            compare(copyRequestedSpy.count, 1, "the button requests the copy");
            compare(copyRequestedSpy.signalArguments[0][0], "0xcarol", "with the row's address");
            verify(del.copiedFlashing, "and confirms it on the row");
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

        // The body is read-only selectable text, and a selection yields the raw
        // string, so markup in a message stays literal.
        function test_messageBubbleTextSelectable() {
            const bubble = instantiate(messageBubbleC);
            const body = findField(bubble, "messageText");
            verify(body, "the message text must be reachable");
            verify(body.readOnly, "the body must not be editable");
            verify(body.selectByMouse, "the body must be selectable with the mouse");

            body.selectAll();
            compare(body.selectedText, "<b>not bold</b>", "the selection is the literal plain text");
        }

        // The bubble hugs a short message and caps a long one at 70% of the row.
        // Guards the sizing formula, which reads the body's implicit size.
        function test_messageBubbleSizing() {
            const shortBubble = createTemporaryObject(messageBubbleC, testRoot);
            const longBubble = createTemporaryObject(messageBubbleC, testRoot);
            verify(shortBubble && longBubble, "both bubbles must instantiate");
            shortBubble.width = 400;
            shortBubble.content = "Hi";
            longBubble.width = 400;
            longBubble.content = "A message long enough to need wrapping. ".repeat(10);

            const shortBox = findField(shortBubble, "bubble");
            const longBox = findField(longBubble, "bubble");
            verify(shortBox && longBox, "both bubble backgrounds must be reachable");
            verify(shortBox.width > 0, "a short bubble must still have a width");
            verify(shortBox.width < longBox.width, "a short message makes a narrower bubble");
            compare(longBox.width, 280, "a long message caps at 70% of the row");
            verify(longBox.height > shortBox.height, "wrapped content makes a taller bubble");
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
