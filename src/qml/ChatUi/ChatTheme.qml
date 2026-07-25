pragma Singleton

import QtQuick

import Logos.Theme

// Chat-local design tokens layered on Logos.Theme: only what the design system
// does not provide yet.
QtObject {
    id: root

    // Monospace family for addresses and installation IDs, where fixed-width hex
    // aids scanning and copy. Logos.Theme ships only Public Sans, so this uses
    // the platform's generic monospace family.
    // TODO(upstream: logos-design-system): a shared code/monospace token.
    readonly property string monoFont: "monospace"

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
}
