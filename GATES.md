# GATES — UX-03F: complete Delete instructions in compact native header

BASE: main@0d283e361d25c806efcbea9c93146ced06280cfd, VERSION.txt=0.9.66.
ISSUE: https://github.com/Askeladd22/The_Cuelist/issues/188
OWNS: GATES.md, src/CueViews.cpp, src/MainComponent.h,
      tests/UiRefinementTests.h, tests/Tests.cpp, docs/complete-user-guide.md,
      VERSION.txt, CMakeLists.txt, CHANGELOG.md.
SCOPE: Only visual copy selection in PinnedColumnHeaders when Delete is
armed, plus native tests/screenshots and docs/versioning. No changes to
keyboard dispatch, audio/MIDI/OSC/LTC, VIEW/LIVE, persistence JSON v10,
geometry, mouse input, transport or other controls.

## Evidence and hypothesis

- Baseline PR #187: Windows Release/CTest succeeded on
  ef8c5b1b1cb4d7eca3b744c55f822019471323f5,
  https://github.com/Askeladd22/The_Cuelist/actions/runs/43571468308
- A real JUCE 1180x820 screenshot from PR #186 showed the armed Delete
  row curtailed: "Press D to delete / repeating 5 does n…". The pinned
  action header is ~210 logical px wide at that viewport.
- H-UX03F: fit-checking short native copy with shaped JUCE glyph metrics
  will keep the complete "D DELETE / 5 IGNORED" instruction legible
  without reducing the 11px design size, while preserving Esc/PANIC and
  Space/GO hints. Long column names cannot take away row 2.
- Counterexample: merely setting drawText ellipses=false chops the final
  words, and deprecated Font::getStringWidth() fails with newer JUCE 8;
  measure with GlyphArrangement::getStringWidth() instead.

## Gates

- [x] F1 EVIDENCE: Issue #188, screenshot, main SHA, 0.9.66 baseline,
  #187 success and relevant JUCE painter read.
- [ ] F2 SOURCE: When armed, text is complete for measured available
  header width. Compact >=200px uses explicit D and 5 ignored; narrower
  fallback always begins with D. 72px/3-row layout and 11px label intact.
  EVIDENCE: pending source audit plus native Windows test on PR SHA.
- [ ] F3 REGRESSION: Native JUCE test checks actual 1180 and 1920
  header widths, GlyphArrangement width <= drawable width, long names,
  accessible description, repeat 5 does not delete, Esc still PANIC,
  D deletes only the captured stable ID, VIEW/LIVE unchanged.
  EVIDENCE: pending CTest on PR SHA.
- [ ] F4 VISUAL: CI captures real native armed screenshots at
  1180x820 and 1920x1080 plus 1180 with long column name; humans
  review no ellipsis on instruction, readable 10-11px text and contrast.
  EVIDENCE: pending screenshot artifacts and review.
- [ ] F5 CI: Windows Release, complete CTest and version/changelog
  guard green for exact PR head SHA.
  EVIDENCE: pending workflows.
- [ ] F6 LIVE / MANUAL: Windows 100/125/150% DPI, real keyboard/IME,
  Narrator, external audio/MIDI/OSC/LTC with PANIC/GO, operator metrics.
  EVIDENCE: ND; not proven by simulation or CI.

## Red team, tradeoff and rollback

Compact instruction is shorter than prose; full "5 cannot confirm"
meaning remains in setDescription and user guide. No new visual hierarchy
or controls; avoiding decreased text size is prioritized. Other JUCE
modal popup Esc/PANIC pathways remain a separate RQ. Gate F6 cannot be
declared passed by CI.

PR remains draft, no automatic merge. Rollback: revert UX-03F PR; no
project migration. GATES from prior merged PRs have unresolved manual
LIVE checks that remain independent of this narrow patch.
No inherited shell CHECK or hook is approved.
