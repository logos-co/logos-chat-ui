# Two-instance message exchange

The [chat UI tutorial](../doctests/chat-ui.test.yaml) drives a single chat UI
window: it connects to the network and shares its address. A message
exchange, though, is inherently two-party, and the doc-test `ui_test` harness
cannot express it on its own: it drives one app instance and can only *set
literal values* on fields, so it can never read one window's address out and
paste it into another. The
[automated exchange tutorial](../doctests/chat-ui-exchange.test.yaml) runs that
round-trip headless via the `exchange` app and captures the result in one
screenshot.

This page covers the other half end-to-end: two real chat UI windows exchanging
encrypted messages over the live network. It is driven by
[`doctests/exchange/run-exchange.mjs`](../doctests/exchange/run-exchange.mjs),
which speaks the [logos-qt-mcp](https://github.com/logos-co/logos-qt-mcp)
inspector protocol to **both** instances at once, so it can read Alice's address
out of her window and hand it to Bob, then capture each side of the
conversation. The screenshots below are produced by that script.

Two instances coexist on one host because each picks a random QtRO socket name
and each delivery node picks its own listening ports; the script gives each its
own session dir (`--user-dir`) and QML inspector port (`QML_INSPECTOR_PORT`).

## Run two instances interactively

To exchange a message by hand, launch two standalone apps; a distinct session
dir keeps the two module instances cleanly apart:

```bash
# window A
nix run . -- --user-dir ~/.local/share/chat_a
# window B
nix run . -- --user-dir ~/.local/share/chat_b
```

Both nodes join the `logos.test` Waku fleet (and publish their key packages to
the key-package registry during `init`), so this needs internet and ~5-20s to
reach **Online**. Each window makes its account on its first start and keeps it
in its session dir; name it with **Add your name** on the account card, so the
other side sees a name rather than the first eight characters of an address.
Then, in window A copy the address off the account card; in window B
pick **New chat > Direct message** and paste A's address — the conversation opens on B's
side and the invite goes out. Once it appears in A's list (she has joined),
send the first message from B; reply from either side.

## Regenerating the screenshots

```bash
doctests/exchange/run-exchange.sh docs/images/exchange
```

The wrapper builds the standalone app from the flake, launches Alice and Bob
offscreen, names each account, runs the exchange, writes the PNGs below, and tears both instances
down. It exits non-zero if the round-trip does not complete, so it doubles as an
end-to-end integration check. CI's doc-test run on Linux uploads a fresh set as
the `chat-ui-flow-screenshots` artifact.

The icons are absent from these captures: `LogosIcon` tints its asset through a
`MultiEffect`, and the offscreen platform plugin has no GPU to rasterize a
shader effect with, so the New chat button, the copy buttons and the send arrow
render as their bare backgrounds. The assets themselves load; a windowed run
shows them.

## The exchange

### 1. Alice names her account and shares her address

Alice waits for the network to come up (**Online**), at which point the backend
has called `get_address()` and her account card carries her address. Her key
package was published to the registry during `init`, so the address alone lets a
peer open a conversation with her. She and Bob each name their account on the
card, which calls `set_display_name(name)`: the name goes into the account's log
on the registry, and the card shows it once it is there.

![Alice's account card](images/exchange/01-alice-address.png)

### 2. Bob opens a conversation and sends the first message

Bob pastes Alice's address into **New chat > Direct message**. The backend calls
`create_conversation(address)`, which fetches her key package from the registry
and sends her the cryptographic invite; the new thread opens empty. Once Alice
has joined (the conversation shows up in her list), Bob sends the first
message. Bob's window shows the message sent, in a conversation titled by
Alice's name, which his module read from her account's log.

![Bob's first message](images/exchange/02-bob-sent.png)

### 3. Alice receives it

Bob's message lands, the `message_received` event fires, and the thread of the
conversation she joined shows it. The message also tells her whose conversation
it is, so her list titles it by Bob's name from then on.

![Alice receives Bob's message](images/exchange/03-alice-received.png)

### 4. Alice replies

Alice replies with `send_message(convo_id, content)`; the reply travels back
over the same conversation.

![Alice replies](images/exchange/04-alice-replied.png)

### 5. Bob receives the reply

The same path in reverse: Bob's thread now shows both messages. A real
bidirectional, end-to-end-encrypted round-trip between two independent UI
instances.

![Bob's full thread](images/exchange/05-bob-roundtrip.png)
