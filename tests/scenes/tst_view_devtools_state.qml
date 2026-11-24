import QtQuick

import Logos.ChatBackend
import ChatUiScenes

import "../../src/qml"

// A GroupV3 group with the developer tools on State: one member behind this
// account's epoch, and one not heard from since joining.
Item {
    id: stage
    width: 1280
    height: 800

    readonly property MockLogos logos: MockLogos {
        backend.chatStatus: ChatBackend.Online
        backend.myAddress: "4be1c07a9d22f0e8b5a6c3d7e9f0a1b2c3d4e5f60718293a4b5c6d7e8f90a1b2"
        backend.myLabel: "4be1c07a"
        backend.myInitials: "4b"
        backend.debugData: ({
                epoch: 4,
                epochFingerprint: "a2434d2747f3f5c1",
                members: [
                    {
                        label: "4be1c07a",
                        signer: "1233c4d1f0e2a9b8c7d6e5f4a3b2c1d0e9f8a7b6c5d4e3f2a1b0c9d8e7f6a5b4",
                        sync: "synced",
                        epoch: 4,
                        epochFingerprint: "a2434d2747f3f5c1",
                        isSelf: true
                    },
                    {
                        label: "0b2c3d4e",
                        signer: "52c9e1f0d2c3b4a5968778695a4b3c2d1e0f9a8b7c6d5e4f3a2b1c0d9e8f7a6b",
                        sync: "behind",
                        epoch: 3,
                        epochFingerprint: "5e81c09b2a7d44e0",
                        isSelf: false
                    },
                    {
                        label: "a1b2c3d4",
                        signer: "34e8a1b2c3d4e5f60718293a4b5c6d7e8f90a1b2c3d4e5f60718293a4b5c6d7e",
                        sync: "unconfirmed",
                        epoch: -1,
                        epochFingerprint: "",
                        isSelf: false
                    }
                ],
                counts: [
                    {
                        sync: "synced",
                        count: 1
                    },
                    {
                        sync: "behind",
                        count: 1
                    },
                    {
                        sync: "unconfirmed",
                        count: 1
                    }
                ],
                json: "{}",
                readAt: "12:41:07"
            })

        conversations: ListModel {
            ListElement {
                conversationId: "c2"
                displayName: "Design Team"
                isGroup: true
                avatarInitials: "c2"
                avatarRamp: 3
                unreadCount: 0
                lastActivityDisplay: "12:41"
                preview: "You: Merged, thanks all"
                description: ""
                historyOnly: false
            }
        }
        messages: ListModel {
            ListElement {
                sender: "Me"
                avatarInitials: "4b"
                avatarRamp: 0
                content: "Merged, thanks all"
                timeDisplay: "12:41"
                isMe: true
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
            }
            ListElement {
                address: "0b2c3d4e5f60"
                label: "0b2c3d4e"
                avatarInitials: "0b"
                avatarRamp: 1
                isSelf: false
                pending: false
            }
            ListElement {
                address: "a1b2c3d4e5f6"
                label: "a1b2c3d4"
                avatarInitials: "a1"
                avatarRamp: 2
                isSelf: false
                pending: false
            }
        }
    }

    ChatView {
        id: view
        anchors.fill: parent
    }

    Shot {
        id: shot
        name: "view-devtools-state"
        target: stage
        setup: () => {
            const b = stage.logos.backend;
            b.currentIsGroup = true;
            b.currentDisplayName = "Design Team";
            b.currentAvatarInitials = "c2";
            b.currentAvatarRamp = 3;
            b.memberCount = 3;
            b.selectConversation("c2");
            shot.findChild(view, "devtoolsTabBar").currentIndex = 2;
            view.toggleDevtools();
        }
    }
}
