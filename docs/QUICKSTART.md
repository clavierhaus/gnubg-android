# GNU Backgammon for Android — Quick Start

This is the real engine of GNU Backgammon in your hand: the same neural-net
player and analyser that runs on the desktop, judging every move to a
thousandth of a point. The board is a touch surface, and a few of its gestures
are not obvious until someone points them out. This guide points them out.

One idea underlies the whole app: **gnubg is the authority.** Every number,
every "best move", every verdict is the engine's own — the app only shows it to
you. Nothing here is the app's opinion.

The board is **landscape only**. The gear at the top-left of every screen opens
Settings; it is always the same gear in the same place.

---

## The four modes

From the hub you choose one of four things to do. The verb leads each one:

- **Play** a tournament match against gnubg.
- **Train** with the Coach — play, and have each move judged.
- **Analyse** any position you set up or paste in.
- **Review** a saved match, move by move.

---

## Moving the checkers (Play and Train)

You roll, then move. There are two ways to move a checker, and the second is
the one people miss.

**Tap the source, then let the dice place it.** Tap a point that has your
checker; the app plays one die from there (it tries your dice in order). Tap
again to play the second die. The engine decides at each tap whether that
sub-move is legal — if nothing happens, that die can't be played from there.

**Tap the destination to land two checkers at once — the shortcut worth
knowing.** When both of your dice can reach the same empty point — each from a
point where you already have a checker — tap that point, and both checkers
slide in together in a single move. It's the fast way to **make a point** when
the roll allows it: no need to move each checker separately. This shortcut is
specifically for that two-checkers-onto-one-point case; for any other move,
place checkers the normal way, one die at a time.

**Long-press a checker to see where it can go — and mind the doubles.** Press
and hold a point that holds your checkers; every point that checker can reach
lights up. This is more than one die: if a checker can travel several hops on
one turn, every resting point along the way lights. On a **double**, a single
back checker on the 24-point rolling 2-2 lights **22, 20, 18, and 16** — one
hop, two, three, four. The highlight is gnubg's own legal-move list, so a point
lights only if the engine truly offers a move landing there, and once you have
played a die only the moves that remain are shown. Release without moving —
it's a preview, not a commitment.

**Drag, if you prefer.** You can also drag a checker from its point to a
destination; the same legality rules apply.

**Coming off the bar.** If you have a checker on the bar, tap the bar (the
centre strip) as your source — you must enter before any other move, exactly as
the rules require.

**Undo and confirm.** While you have a move in progress, the two buttons by the
dice **undo** the last sub-move or **confirm** the whole move. gnubg enforces
the real rule that you must use both dice and make the largest legal play — if
your move can't complete to a legal maximum, confirm won't accept it, and you
undo and try another way. That is the engine refusing, not the app.

**When you can't move.** If the roll leaves you no legal move, a single
**pass** affordance appears where the buttons were; tap it to hand the turn on.

---

## The dice and the cube

**Roll** by tapping the **Roll** button on the right half of the board, where
your dice will appear. gnubg's dice for the turn sit greyed on the left half
until you roll.

**Swap the dice order.** Tap your two dice to swap which one is "first". This
only matters for how a tap-to-move sequence tries them — the move is the same
either way.

**Double** by tapping the **doubling cube**. The app offers the cube only when
doubling is actually legal — the engine decides that, so if a tap does nothing,
a double isn't available to you yet. When gnubg doubles you, buttons appear to
**take** or **drop**.

---

## Train with the Coach

Coach is Play with a teacher. It opens on a **setup screen**: choose the
opponent's **strength** (the same seven levels as tournament play, from
Beginner to Grandmaster) and the **match length**.

- **One point** keeps the doubling cube dead — you get **chequer-play coaching
  only**.
- **Longer matches** put the cube in play, and the Coach judges your **cube
  decisions** too.

Then you play. After each of your moves, gnubg judges it and the game pauses so
you can study the verdict — **GNU waits for you**. You'll see one of three
honest verdicts:

## The right-hand panel — reading the Coach's verdict

Everything the Coach tells you appears on the **right-hand side of the board**,
and it rewards a moment's explanation because its most useful feature is not
obvious at first glance.

At the top is the **verdict** on the move you just played — one of three honest
outcomes:

- **The best move** — you found gnubg's top choice.
- **Fine, Nth of M** — a reasonable move, with what would have been better.
- **A flagged move** — gnubg's severity (doubtful, bad, very bad) and the exact
  equity it cost.

Below the verdict is a **list of five rows**: gnubg's top five moves for this
position, ranked best-first, with your own move marked **P** in red **in its
place in the ranking** — so if you played the fourth-best move, P sits at the
fourth row, not the top. Each row shows how much that move gains or loses
against yours.

**Here is the part people miss: those five rows are buttons.** They look like a
read-out, but each one is tappable, and tapping is how you *see* the difference
on the board rather than just read the notation. This is the Coach's real
teaching tool, and it's worth learning the two-tap rhythm:

- **First tap on a row** shows that move's **decision point** — the position
  *before* the move, with the dice, no arrows. Crucially this is the *same*
  starting picture for every row, so you're always comparing like with like.
- **Second tap on the same row** shows the **result** — where those checkers
  end up, with **green arrows** pointing to each destination.
- **Tap once more** to flip back to the decision point.

So the way to actually compare is: tap **P** twice to watch your own move play
out in green arrows, then tap **row 1** twice to watch gnubg's best move play
out from the identical start. Arrow by arrow, you see exactly how the best play
differs from yours. Until you tap, nothing on the board moves — the rows are an
invitation, and most of the Coach's value is behind them.

**Continue** by tapping **GNU's turn** — the button on the board's left half,
the mirror of your Roll. Only then does gnubg receive your move and reply. The
Coach never rolls for GNU while alternatives are still on the table, so the
position you're studying never shifts under you.

**Cube coaching** (longer matches). When you double, take, or drop, the Coach
judges that decision against gnubg's — correct, reasonable, or flagged with the
equity cost — using the engine's own cube evaluation.

**One thing about the cube that surprises people.** When you tap the cube to
double, the Coach shows its verdict on your decision *straight away* — but the
**cube face does not change yet**, and GNU has not answered yet. This is
deliberate. Just like a chequer move, your cube decision is *held*: the Coach
judges what you chose, and nothing is committed until you tap **GNU's turn**.
Only then does gnubg actually receive the double, roll, and either take or drop
— and only then does the cube move to **2** and take its place on the board. So
the sequence is: tap the cube → read the verdict → **GNU's turn** → GNU
responds and the cube updates. A doubling cube that doesn't move the instant
you tap it feels odd at first, but it is the only way to judge your decision
*before* the game commits to it — the same "decide, see the verdict, then
continue" rhythm the whole Coach uses.

---

## Analyse a position

Analyse lets you set up any position and ask gnubg what it thinks.

**Build a position by hand.** Tap a point to add your checkers; the edit
controls let you switch which colour you're placing (**You** / **GNU**), send
checkers to the **bar**, or **erase** a point by tapping the tray. **Start pos**
resets to the opening arrangement; **Swap** exchanges the two sides.

**Or paste an ID.** Paste a **GNU BG ID** (`PositionID:MatchID`) or an **XGID**
— the app recognises either dialect and installs it exactly as given.

**Set the context that changes the answer.** The score, the match length, the
cube's position and owner, whose roll it is, and the **Crawford** flag all
affect the right play — set them so gnubg evaluates the position you actually
mean.

**Dice or no dice — this is the key distinction.** If you set **dice**, gnubg
gives you the best **chequer play** for that roll. If you leave the dice
**unset**, gnubg gives you the **cube decision** — whether to double, take, or
drop — in its own words. Setting the dice or clearing them is how you ask the
two different questions.

**Reading the result.** Once you press **Analyse**, the entry controls step
aside and the pane shows only the answer: the match context on top, then
gnubg's verdict. **Back** (bottom right) returns you to the entry screen; the
board keeps showing the analysed position while you read.

With **dice set** you get gnubg's ranked plays: the best move carries its own
equity, and every line below shows how much *worse* it is than the best — so
`-0,318` means that play gives up 0,318 equity against the top choice, not
that the position is bad.

With **no dice** you get the full cube picture:

- the **verdict**, in gnubg's own words ("No redouble, take");
- a **chances table** — Win / Gammon / Backgammon for both sides;
- **Cubeless** and **Cubeful** equity;
- the three **actions** — No double, Double/take, Double/pass — each with its
  equity and its distance from gnubg's optimal, the best one marked;
- a **Rollout** button: gnubg plays the position out 144 times (cubeful,
  variance-reduced) and reports win% and cubeful equity, each with a ±
  confidence figure. On a strong phone this takes a few seconds;
- the **MET** in use, named bottom-right. Changing the match equity table in
  Settings while a result is on screen recomputes the result under the new
  table on the spot — the numbers you see always belong to the table named.

**"Cube not available" is an answer, not an error.** The player on roll can
only double if the cube is centred or they own it. If your setup gives the
cube to the opponent (or it is the Crawford game, or the cube is off), gnubg
says so, and the action rows disappear — there is no cube decision to rank.
The chances and equities remain, because they describe the position itself.
If you meant to ask a redouble question, set **Cube owner: You** in the
editor.

---

## Review a saved match

Review walks through a match you've saved to `.sgf`, one move at a time.

**Open a match** with **Open match** and pick the saved file. Then step through
it: **Move >** advances one move, **Game >** jumps to the next game of the
match. The header shows the running score and where you are (which game, whose
turn, end of game).

This is the way to go back over a finished game and see how it unfolded —
the same board, replayed.

---

## A few things worth remembering

- **The gear is always top-left.** Settings — themes, match rules, equity
  tables — live there, on every screen.
- **Nothing scrolls.** Every screen is sized to fit its device; if something
  looks like it should scroll, it doesn't — it's all there.
- **The engine is always right, by design.** If a tap seems to do nothing, the
  most common reason is that gnubg has judged the move illegal or the action
  unavailable. That's not a bug; it's the rules being enforced by the same
  engine that will, elsewhere, tell you the move was a blunder.
