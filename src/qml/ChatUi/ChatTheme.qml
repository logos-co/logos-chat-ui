pragma Singleton

import QtQuick

import Logos.Theme

// Chat-local design tokens layered on Logos.Theme: only what the design system
// does not provide yet.
QtObject {
    id: root

    // Message-bubble pair. Peer bubbles reuse the recessed surface; own bubbles
    // use the primary accent. Logos.Theme has no on-primary text token, so the
    // dark foreground for own bubbles is defined here (6.8:1 on the salmon accent).
    // TODO(upstream: logos-design-system): bubble roles + an on-primary text token.
    readonly property color bubblePeer: Theme.palette.surfaceRecessed
    readonly property color bubblePeerText: Theme.palette.text
    readonly property color bubbleOwn: Theme.palette.primary
    readonly property color bubbleOwnText: "#000000"

    // Selection highlight for message text, inverted per side so the marked run
    // stays legible on either bubble background.
    // TODO(upstream: logos-design-system): selection roles.
    readonly property color bubblePeerSelection: Theme.palette.primary
    readonly property color bubblePeerSelectedText: "#000000"
    readonly property color bubbleOwnSelection: "#000000"
    readonly property color bubbleOwnSelectedText: Theme.palette.primary

    // Avatar gradients, indexed by a model's avatarRamp role. There are exactly
    // as many as Identity::kAvatarRampCount in the backend, which is what
    // spreads identities across them.
    readonly property list<Gradient> avatarRamps: [
        Gradient {
            GradientStop {
                position: 0
                color: Theme.palette.accentYellowSoft
            }
            GradientStop {
                position: 1
                color: Theme.palette.accentOrangeMid
            }
        },
        Gradient {
            GradientStop {
                position: 0
                color: Theme.palette.info
            }
            GradientStop {
                position: 1
                color: Theme.palette.accentOrangeDeep
            }
        },
        Gradient {
            GradientStop {
                position: 0
                color: Theme.palette.successHover
            }
            GradientStop {
                position: 1
                color: Theme.palette.info
            }
        },
        Gradient {
            GradientStop {
                position: 0
                color: Theme.palette.warning
            }
            GradientStop {
                position: 1
                color: Theme.palette.accentBurntOrange
            }
        },
        Gradient {
            GradientStop {
                position: 0
                color: Theme.palette.primarySoft
            }
            GradientStop {
                position: 1
                color: Theme.palette.warning
            }
        }
    ]

    // This account's own avatar, on the brand ramp rather than a hashed one, so
    // your own entry is recognisable in any list.
    readonly property Gradient selfAvatarRamp: Gradient {
        GradientStop {
            position: 0
            color: Theme.palette.primarySoft
        }
        GradientStop {
            position: 1
            color: Theme.palette.primary
        }
    }

    // Ink for what sits on an avatar's light gradient, where the theme's text
    // colours have no contrast.
    // TODO(upstream: logos-design-system): an on-accent text token.
    readonly property color avatarInk: Qt.rgba(0, 0, 0, 0.74)

    // One colour per session-log domain, so a chip and the dot on every row it
    // filters read as the same thing. `domain` is a name the backend publishes.
    function logDomainColor(domain) {
        switch (domain) {
        case "libchat":
            return Theme.palette.info;
        case "chat-module":
            return Theme.palette.primary;
        case "chat-ui":
            return Theme.palette.successHover;
        case "delivery":
            return Theme.palette.accentYellowSoft;
        default:
            return Theme.palette.textMuted;
        }
    }

    // One colour per severity, on the same scale the rest of the app uses for
    // status. `level` is a severity name as the backend spells it.
    function logLevelColor(level) {
        switch (level) {
        case "error":
            return Theme.palette.error;
        case "warning":
            return Theme.palette.warning;
        case "info":
            return Theme.palette.info;
        default:
            return Theme.palette.textMuted;
        }
    }
}
