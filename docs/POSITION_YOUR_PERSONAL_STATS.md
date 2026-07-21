# Goodbye "PR", Welcome "Your Personal Stats"

*On the death of a product, the survival of a vocabulary, and what a standard
actually is. A position paper of the CBG project. Companion pieces: "The
Boundary to Hallucination" (docs/PAPER_HONEST_VERBOSITY.md) on why our coach
does not invent explanations; this paper is about numbers rather than words,
and about who owns them.*

## The king is dead

For a decade and a half, serious backgammon study meant one program.
eXtreme Gammon earned its place: the strongest widely available analysis,
the study tool of nearly every strong player, the program whose rating
number -- PR, the Performance Rating -- became the way the community talks
about skill. When players say "she's a sub-5 player," they are speaking
XG. That is a real achievement and it deserves a eulogy, not an autopsy.

But on Android, the king is dead. The mobile app is gone and will not
return; players looking for it today find a tombstone and a community
recommending workarounds. What remains on that platform is grief with
nowhere to go -- and a habit of speech. Players still ask for their PR.
They ask new tools whether their numbers are "real PR." They treat the
dead program as a standard that living programs must conform to.

That last sentence contains a category error, and this paper exists to
name it gently and then to offer something better.

## What a standard is, and what XG was

A standard, in the sense the word deserves, has three load-bearing parts.
There is a *published specification* that exists independently of any
implementation. There is a *conformity test* that anyone can run against
that specification. And there is a *steward* -- a body that answers for
the specification, maintains it, and can say what conforms.

Hold the Performance Rating against those three parts. The specification
was never published apart from the product: which decisions count as
"non-obvious," at which thresholds, under which analysis settings -- the
filter that defines the number lives inside proprietary code. The
conformity test therefore cannot exist: **you cannot conform to a
secret.** No one can implement "XG PR" from documentation and verify they
got it right, because the reference is not a document but a binary. And
the steward is a product vendor, which is a different thing from a
standards body -- as the community itself discovered when serious rating
of players required an external organisation to bolt a fixed settings
protocol onto XG before its numbers became comparable at all. Even inside
the product, a PR of 5 at one analysis level is not a PR of 5 at another.
A number that needs an outside body to pin its parameters before it
commensurates with itself was never a standard. It was a *brand whose
vocabulary escaped it*.

None of this diminishes what XG was as an engine or a study tool. It
means only that the thing the community treats as certifiable was never
the kind of thing that certifies.

## The vocabulary survived the product

What actually happened is older than software: the product died and its
language stayed behind. Horsepower outlived horses. Elo outlived every
implementation Arpad Elo touched. "PR," "World Class," "sub-5" are now
folk metrology -- a shared way of talking about skill that belongs to the
community, not to the program that coined it. And unstewarded language
drifts. Players today compare PRs produced at different analysis depths,
by different tools, under different decision filters, and draw
conclusions from differences that are artifacts of convention. There is
no body left to tell them the numbers do not commensurate.

The opportunity, for anyone building in this space, is therefore not to
replace XG. XG cannot be replaced, because the thing people miss is not
the binary -- it is the confidence that a number means something. The
opportunity is to *steward the vocabulary the dead product abandoned*:
to keep the words the community speaks, and to put published definitions
under them.

## What actually meets the definition

Here is the quiet irony of this story. In this domain there has been,
all along, exactly one thing that satisfies the definition of a standard
-- and it is the one nobody crowned. GNU Backgammon's statistics are
specified in a published manual, formula by formula: what counts as an
unforced move, what a close cube decision is, how the error rate divides,
how the luck-adjusted result is constructed. Its source code is public,
which means the specification and the implementation are the same
inspectable text. And the conformity test is one anyone can run: analyse
the same match file in the reference implementation and compare, number
for number.

That last property inverts certification itself, and the inversion is
the philosophical heart of this paper. A standards body certifies by
*authority*: a stamp you must trust. Reproducibility certifies by
*verification*: a check you can run yourself. A program whose numbers are
computed by published formulas over open source does not need to issue
certificates of conformity -- it hands every user the means to certify
it, personally, as often as they like. Where XG could only ever say
"trust the brand," an open engine says "here is the formula, here is the
source, here is your match file -- check."

Our own project holds itself to exactly that: before our match statistics
ever reached a screen, they were validated against desktop GNU Backgammon
on identical input, and any user can repeat that comparison forever. We
do not ask to be believed. We ask to be checked.

## The farewell

So what, precisely, are we saying goodbye to? Not to XG -- the eulogy
stands, and the vocabulary it gave the community is kept with respect.
We are saying goodbye to the *altar*: the rating-first culture in which a
single number, produced by an unpublishable filter, is treated as the
point of analysis -- chased match by match, compared across incomparable
settings, worshipped at a precision it does not possess. It deserves
saying plainly: over a single match, any error rate is mostly noise. The
number that humbles you tonight and flatters you tomorrow has not
measured a change in you; it has measured variance. A serious tool owes
its users that sentence, printed where the number is shown -- not buried
in a forum answer.

And we are saying goodbye to the unlabeled number. A statistic that does
not carry its convention on its face -- which denominator, which scale,
which analysis depth -- is not information; it is an invitation to a
wrong comparison. Every number we show is labeled with the convention
that produced it, and where two legitimate conventions exist, both are
shown as what they are: two dialects for one fact, computed by the same
published formulas. The fact is one; the renderings are many; none of
them is smuggled.

## The welcome: Your Personal Stats

What replaces the altar is a change of grammatical person. Not
"Performance Rating" -- a noun that belongs to a dead product and a
leaderboard culture -- but **Your Personal Stats**: a possessive, and a
deliberate one.

*Yours* because the data is yours in the plainest sense: match records as
ordinary files in a folder you chose, readable by any gnubg-compatible
software on earth, never locked to an app, an account, or a server --
including, for those arriving from the old kingdom, whatever archives
they can carry with them. A tool that respects its users does not hold
their history hostage; it makes leaving easy, and trusts that staying
will be earned.

*Personal* because the point of the numbers is you, not your rank. The
first thing a match report should show is not a rating but the receipts:
the moves that cost you, ranked, each one a doorway back into the
position to replay it right. The rating is context -- one line, honestly
labeled, with its noise stated -- because the purpose of analysis is not
to crown you but to teach you. That is the inversion the old culture
never made: the number served the leaderboard; these numbers serve the
player.

And *stats*, plural and plain, because that is what they are: published,
reproducible calculations over facts you collected by playing --
displayed in whichever dialect you prefer, verifiable by anyone,
certified by no one because they need no one's certificate.

The king is dead. The language lives, and it finally has definitions.
Your matches, your folder, your numbers -- your personal stats.

*-- The CBG project. The statistics described here are computed by GNU
Backgammon's published formulas (GPL); the source, the validation
tooling, and this paper are part of the project's public repository.*
