# Phrase research routine

The phrase set is **finite and closed**: 11 narrator sentences and 14 corpus
entries. Every one can therefore be researched exhaustively rather than
corrected one field screenshot at a time. This is the routine for doing that,
and for keeping it done.

Written 2026-07-22, after four distinct false-statement classes reached the
screen in a single day — every one found by playing, none by any check we had.


## Why phrases go wrong

The phrase layer was authored as backgammon writing and wired to gnubg
signatures separately. Nobody checked the join. Four failure classes have been
observed, and they are independent of each other:

1. **Hard-coded referent.** A sentence naming "the best play" becomes false the
   moment the compared variant is not the best one. Fixed by the `{REF}` token.
2. **Direction inversion.** A rule fires on the opposite sign of what its
   sentence asserts. `board.structure.connected` fired when the other variant
   was *more* scattered and then praised it for being connected, because
   `I_BACKBONE` measures disconnection.
3. **Unlicensed count.** "Both blots were in reach" from a signature that counts
   no blots.
4. **Unlicensed claim generally.** "The board crunched onto dead points while a
   spare was free to carry the roll" — crunch, dead points and spare are all
   unmeasured.

Classes 2, 3 and 4 are all the same underlying fault: **the sentence asserts
more than the signature establishes.**


## Sources, in order of authority

1. **Tom Keith's Backgammon Galore glossary** — <https://bkgm.com/glossary.html>
   The dictionary. A term that is not an entry there is not a backgammon word,
   whatever it sounds like. This is what retired "rear point" (the word is
   *anchor*), "back checkers" (*runners*), "re-enter" (*enter*), and corrected
   *blockade* to *contain* once the measured input was known.
2. **Expert annotation in context** — the rec.games.backgammon archive at
   <https://bkgm.com/rgb/>, the annotated matches at <https://bkgm.com/matches/>,
   and contemporary match analysis such as <https://backgammon.substack.com>.
   Use these for *register* — how a strong player actually explains a move —
   not for terminology, which comes from the glossary.
3. **Forums** — bgonline.org, r/backgammon. Useful for what confuses people and
   for contested usage.
4. **General web search** — last resort. It returns rules pages and shopping
   results; it does not find idiom.


## The register finding: backgammon counts in rolls

Strong annotation does not speak in abstractions. It counts: *all ones are good
for Red now*, *nothing outside 66, 55 and 44 is a decent roll*. The glossary
makes it the native unit — a direct shot is a hit from six points or less, and
"combinations of the dice" is defined as the number of rolls out of 36 that
achieve something.

Four of our inputs are literally that number over 36, so a phrase can state the
count itself. This is simultaneously **more licensed** (it is the raw
measurement, not an interpretation) and **more idiomatic** (it is how players
talk):

| Input            | eval.c form         | Speakable as                          |
|------------------|---------------------|---------------------------------------|
| `I_P1`           | `n1/36`             | numbers that hit at least one blot    |
| `I_P2`           | `n2/36`             | numbers that hit two or more          |
| `I_BACKESCAPES`  | `Escapes/36`        | numbers that escape                   |
| `I_CONTAIN`      | `(36 - n)/36`       | 36 minus the minimum escape count     |

A delta multiplied by 36 is a whole number of rolls. `threat.shots.given` fires
at `notable = 0.08`, which is 2.9 rolls — so it fires when roughly three more
numbers hit you, and could say exactly that.

**Do not extend this to inputs that are not roll counts.** `I_ENTER` is
equity-weighted (`loss / (36 * 49/6)`) and `I_ENTER2` is quadratic in the number
of made points (`(36 - (n-6)^2)/36`). Multiplying either by 36 yields a number
that is not a count of anything, and saying "numbers" of it would be a fifth
false-statement class.


## The four gates

Every phrase must pass all four. They are independent; passing three is failing.

**G-LICENSE.** For each board-fact the sentence asserts, name the term in its
own signature that measures that fact. No term, no claim. This is the gate that
retired nine corpus entries. Note the architectural limit: `InsightMatcher`
compares two **post-move** boards, so no phrase may assert anything about the
position *before* the move ("both blots **were** in reach") — the pre-move board
exists in `CoachScreen` as `glance.preBoard` but is never passed to `match()`.

**G-DIRECTION.** Read the input's computation in `engine-core/eval.c` and
confirm its sign matches the rule's `direction`. Remember that `d = vb - vp`, so
`up` means the *compared* variant scores higher. Watch for sentinels:
`I_FORWARD_ANCHOR` is `n == 0 ? 2.0 : n/6`, so **2.0 means no advanced anchor**
and the rule correctly uses `direction: down`.

**G-DICTIONARY.** Every noun and verb is a glossary entry.

**G-CONVENTION.** Player is *you / your / yours*. Opponent is *the opponent* —
never *their*, *them*, *it*, or *his*. *Variants*, never *plays*. Referent
neutrality: the compared variant is `{REF}`, never "the best".

**G-GRAMMAR.** LanguageTool `en-GB`, zero findings, checked in **every** `{REF}`
fill, on the rendered string rather than the template.


## Running it

```sh
pip install language-tool-python --break-system-packages   # needs Java, present
```

For each phrase: extract it with its signature terms; walk the four gates;
where a gate fails, research the concept in the sources above and find the
term the game actually uses; then rewrite — or retire.

**Retire rather than reduce.** If stripping a phrase back to what its signature
supports leaves only what a narrator rule already says, the entry has nothing of
its own left and should go. What made those nine entries worth having was
precisely the part that was never measured.


## Standing work

- Convert these gates into a **script that fails the build**, so a bad phrase
  cannot reach a commit, let alone a screen. Hand-running them is what let four
  classes ship.
- Re-check `G-DIRECTION` for every corpus entry, as was done for all eleven
  narrator rules. Only the narrator has had that pass.
- Consider passing `glance.preBoard` into `match()`, which would legitimise a
  whole class of pre-move claims currently impossible to license.
