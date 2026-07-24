# Claim audit -- falsifying the coach's sentences

`claim_audit.c` walks self-play contact positions with gnubg's own
`FindnSaveBestMoves`, and for every (variant, played) candidate pair dumps both
post-move boards' full input vectors -- in the same frame
`gnubg_mobile_position_features` uses -- plus pip counts, position class, and
the board facts the authored sentences talk about (made home points, bar,
blots, anchors, rearmost point, most advanced anchor in 18..23).

`audit.py` replays `InsightMatcher.match` term for term, and for every firing
asserts the sentence's literal claim against those facts.

    ./tools/narrator/audit/build_audit.sh
    ./tools/narrator/audit/claim_audit gnubg-app/app/src/main/assets/gnubg.weights 12000 7 > tmp/pairs.tsv
    python3 tools/narrator/audit/audit.py gnubg-app/app/src/main/assets/insights_v0.json tmp/pairs.tsv

Exit status is non-zero when any sentence fired where its words were false.

Two honest limits. Entries whose sentence merely restates their own signature
term (fewer shots, contained, timing) have no independent claim to test and are
reported as such, never counted as passes. And the predicates are one reading
of each sentence: when a predicate and a phrase disagree, the predicate can be
the wrong one -- `anchor.advance.mid` failed 21 times against a predicate that
looked at the rearmost anchor, and passed 209/209 once the predicate scanned
what `I_FORWARD_ANCHOR` itself scans (eval.c:844-861).
