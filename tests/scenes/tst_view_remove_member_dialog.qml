import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// The confirmation Remove opens from a member's row in a group of three.
Item {
    id: stage
    width: 1280
    height: 800

    readonly property MockLogos logos: MockLogos {
        backend.chatStatus: ChatBackend.Online
        backend.myAddress: "4be1c07a9d22f0e8b5a6c3d7e9f0a1b2c3d4e5f60718293a4b5c6d7e8f90a1b2"
        backend.myLabel: "4be1c07a"
        backend.myInitials: "4b"

        conversations: ListModel {
            ListElement {
                conversationId: "c1"
                displayName: "Saro"
                isGroup: false
                avatarInitials: "sa"
                avatarRamp: 0
                unreadCount: 3
                lastActivityDisplay: "12:44"
                preview: "Did you get a chance to look?"
                description: ""
                historyOnly: false
                removed: false
            }
            ListElement {
                conversationId: "c2"
                displayName: "Release crew"
                isGroup: true
                avatarInitials: "c2"
                avatarRamp: 1
                unreadCount: 0
                lastActivityDisplay: "12:41"
                preview: "Raya: notes are up for review, comments welcome before Friday"
                description: "Release planning for 0.3"
                historyOnly: false
                removed: false
            }
            ListElement {
                conversationId: "c3"
                displayName: "Design sync"
                isGroup: true
                avatarInitials: "c3"
                avatarRamp: 3
                unreadCount: 0
                lastActivityDisplay: "11:20"
                preview: "Pax: I'm out next week"
                description: ""
                historyOnly: false
                removed: false
            }
            ListElement {
                conversationId: "c4"
                displayName: "Design Team"
                isGroup: true
                avatarInitials: "c4"
                avatarRamp: 2
                unreadCount: 0
                lastActivityDisplay: "Yesterday"
                preview: "You: Merged, thanks all"
                description: "Design reviews and theme work"
                historyOnly: false
                removed: false
            }
            ListElement {
                conversationId: "c5"
                displayName: "Pax"
                isGroup: false
                avatarInitials: "pa"
                avatarRamp: 2
                unreadCount: 0
                lastActivityDisplay: "Mon"
                preview: ""
                description: ""
                historyOnly: false
                removed: false
            }
        }
        // Newest first: the thread is bottom-anchored.
        messages: ListModel {
            ListElement {
                sender: "Raya"
                avatarInitials: "ra"
                avatarRamp: 1
                content: "Notes are up for review, comments welcome before Friday"
                timeDisplay: "12:41"
                isMe: false
                sameSenderAsPrevious: true
                showDaySeparator: false
                dayLabel: "Today"
            }
            ListElement {
                sender: "Raya"
                avatarInitials: "ra"
                avatarRamp: 1
                content: "Tagged 0.3.0-rc1 on the release branch"
                timeDisplay: "12:38"
                isMe: false
                sameSenderAsPrevious: false
                showDaySeparator: false
                dayLabel: "Today"
            }
            ListElement {
                sender: "Me"
                avatarInitials: "4b"
                avatarRamp: 0
                content: "CI is green, going ahead with the tag"
                timeDisplay: "12:30"
                isMe: true
                sameSenderAsPrevious: false
                showDaySeparator: false
                dayLabel: "Today"
            }
            ListElement {
                sender: "Pax"
                avatarInitials: "pa"
                avatarRamp: 2
                content: "Can someone check the build before we tag?"
                timeDisplay: "11:02"
                isMe: false
                sameSenderAsPrevious: false
                showDaySeparator: true
                dayLabel: "Today"
            }
        }
        members: ListModel {
            ListElement {
                address: "4be1c07a9d22"
                label: "4be1c07a"
                avatarInitials: "4b"
                avatarRamp: 0
                isSelf: true
                pending: false
                removable: false
            }
            ListElement {
                address: "0b2c3d4e5f60"
                label: "Raya"
                avatarInitials: "ra"
                avatarRamp: 1
                isSelf: false
                pending: false
                removable: true
            }
            ListElement {
                address: "f6e5d4c3b2a1"
                label: "Pax"
                avatarInitials: "pa"
                avatarRamp: 2
                isSelf: false
                pending: false
                removable: true
            }
        }
    }

    ChatView {
        anchors.fill: parent
    }

    Shot {
        id: shot
        name: "view-remove-member-dialog"
        target: stage
        setup: () => {
            const b = stage.logos.backend;
            b.currentIsGroup = true;
            b.currentDisplayName = "Release crew";
            b.currentDescription = "Release planning for 0.3";
            b.currentAvatarInitials = "c2";
            b.currentAvatarRamp = 1;
            b.memberCount = 3;
            b.selectConversation("c2");
            // Remove, from the menu a right-click on Pax's row opens. A context
            // menu opens at the X pointer, which a test's click does not move,
            // so the menu is put where the click was before it is used.
            shot.waitForRendering(stage);
            const row = shot.findChild(stage, "memberList").itemAtIndex(2);
            const x = Math.round(row.width * 0.4), y = row.height - 6;
            shot.mouseClick(row, x, y, Qt.RightButton);
            const menu = shot.findChild(stage, "memberMenu");
            const at = row.mapToItem(menu.parent, x, y);
            menu.x = at.x;
            menu.y = at.y;
            shot.mouseMove(stage, -1, -1);
            shot.waitForRendering(stage);
            shot.mouseClick(shot.findChild(stage, "removeMemberMenuItem"));
            shot.mouseMove(stage, -1, -1);
        }
    }
}
