import QtQuick
import QtTest

// One screenshot of `target`, saved to `<name>.png` in the working directory
// once the scene has rendered and settled into a frame that is the same on
// every run. `setup` runs first, for a state reached through the UI (a dialog
// opened, a conversation selected).
TestCase {
    required property Item target
    property var setup: null
    // Upper bound on waiting out finite animations and single-shot timers.
    property int settleTimeout: 5000

    when: windowShown

    // Every object in the window, visual or not. The QML lists (children, data,
    // resources) miss value sources (`X on prop {}`), Behaviors and objects held
    // in plain properties; TestCase.findChild falls through to
    // QObject::findChild, which does not. It is asked per item, since a view's
    // delegates are not QObject children of the view. An empty name matches an
    // object whose objectName is empty, so tagging each hit moves the search on;
    // the names are put back after.
    function objectsIn(root) {
        const tag = "\u0001scene";
        const items = [];
        const walk = item => {
            items.push(item);
            for (let i = 0; i < item.children.length; ++i)
                walk(item.children[i]);
        };
        walk(root);
        const tagged = items.filter(i => i.objectName === "");
        for (const i of tagged)
            i.objectName = tag;
        const others = [];
        for (const i of items) {
            for (let o = findChild(i, ""); o; o = findChild(i, "")) {
                others.push(o);
                o.objectName = tag;
            }
        }
        for (const o of tagged.concat(others))
            o.objectName = "";
        return items.concat(others);
    }

    // What still changes the frame on its own: running finite animations and
    // single-shot timers.
    function busy(objects) {
        // Animation.Infinite reads back as -1 from `loops`, not as its own -2.
        return objects.filter(o => o.running && (o instanceof Animation ? o.loops >= 0 : o instanceof Timer && !o.repeat));
    }

    // Waits out finite animations and single-shot timers, then parks endless
    // animations at the start of a loop and stops them, and hides the caret.
    // An Animator runs on the render thread and stops where it is instead.
    function settle() {
        const root = target.Window.contentItem;
        let objects = [];
        // A timer firing can create objects (a list's delegates, a popup's
        // content), so the walk repeats until it finds nothing new.
        for (let round = 0; round < 5; ++round) {
            const before = objects.length;
            objects = objectsIn(root);
            for (let waited = 0; busy(objects).length > 0; waited += 50) {
                if (waited >= settleTimeout)
                    fail("still running after " + settleTimeout + " ms: " + busy(objects).join(", "));
                wait(50);
            }
            if (objects.length === before)
                break;
        }
        for (const o of objects) {
            if (o instanceof Animation && o.running) {
                o.complete();
                o.stop();
            } else if (o instanceof Timer && o.running) {
                console.warn("Shot: a repeating timer is still running:", o);
            }
        }
        // The caret blinks on the wall clock. Hiding it keeps the focus, and with
        // it the focused field's frame.
        const focused = target.Window.activeFocusItem;
        if (focused && focused.cursorVisible !== undefined)
            focused.cursorVisible = false;
    }

    function test_shot() {
        // The X pointer starts wherever the server put it (the screen's centre
        // under Xvfb), and whatever it rests on shows hover. Park it outside.
        mouseMove(target, -1, -1);
        if (setup)
            setup();
        waitForRendering(target);
        settle();
        // Throws when the file cannot be written, which fails the shot.
        grabImage(target).save(name + ".png");
    }
}
