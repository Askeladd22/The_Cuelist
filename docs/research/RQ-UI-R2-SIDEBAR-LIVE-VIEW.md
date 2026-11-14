# RQ-UI-R2: sidebar LIVE / NEXT GLOBAL GO / VIEW

Date: 2026-11-14. Issue: https://github.com/Askeladd22/The_Cuelist/issues/196
Verified baseline: main@0a2b0b997f0e889ecd6ab2cf0f12e264b6aec0aa, VERSION.txt 0.9.71, proposed 0.9.72; schema JSON v10.

## Operator scenario and evidence

Nine Cuelists: the operator browses L09 while L01 owns LIVE and the next global GO is 02. The operator screenshot and MainComponent.cpp showed a 51px sidebar status with only 94px for the LIVE name. The pinned Cue Grid header had a separate LIVE indicator; VIEW was not paired with global GO/PANIC. This is code/visual evidence, not an operator-error measurement.

MainComponent::liveCuelistNameForUi and nextLiveGoCueForUi are the existing LIVE source of truth; Show.activeCuelist is VIEW. Painting never modifies either state.

## Canonical sources and comparison

- docs/UI_V2_VISUAL_DESIGN_PRINCIPLES.md: one quiet surface per region, semantic colours, no loss of controls, minimum 10pt font.
- docs/UI_V2_FEATURE_PARITY.md: retain all JUCE callbacks, global transport and original Master, cue grid and routing.
- src/WorkspaceLayout.h/.cpp, src/MainComponent.cpp, src/CueViews.cpp, tests/CuelistGuiMockupTests.h and tests/OperatorNavigationTests.h.
- Baseline: 51px ON DECK card; 94px LIVE name and 45px GO locator; VIEW visible elsewhere. New R2: 114px status card; 140px LIVE name; separate NEXT GLOBAL GO and VIEW locator/name.
- Baseline Recent Projects inline extends to height−349. New boundary is ten pixels above status card. Fewer inline projects at compact heights; the real All action retains access to every item.
- GO/PANIC remain exact native buttons with original geometry and callbacks. No audio/MIDI/OSC/LTC, Show v10, Cue Grid, mixer, Master, dock or Timeline changes.

## Hypotheses, operator risk and red-team

H-R2-A: at 1180x820, 1647x895, 1920x1080 and 3840x1080 no label/card overlap; status ends at least 6px before GO, Recent Projects at least 10px before status; actual GO is (14,height−282,152,48), PANIC (14,height−228,152,48).
H-R2-B: a viewed L09 cannot transfer LIVE L01 or its GO cue. Both locators remain distinct when 64-character names share a prefix. VIEW=LIVE must be explicit; END must not look like active playback.
Measured operator goal (ND): shorter time to identify which Cuelist global GO will trigger; equal or fewer wrong-list activations. Run balanced blinded A/B rehearsal exercises, log timing and errors before declaring improvement.

Counterexamples: one vs twelve Cuelists; LIVE hidden behind tab overflow; END; two names indistinguishable under ellipsis; long Unicode/RTL names; Inspector collapsed; 1180x820 and ultrawide; compact Recent Open/All; theme contrast and screen-reader focus. A LIVE list can be silent; do not invent a playing lamp.

## Implementation and test plan

Shared SidebarOperatorLayout is calculated in calculateWorkspaceLayout and consumed by both MainComponent::paint and resized. It is a read-only status panel, never a new command target.
- Pure geometry and real JUCE component tests in WorkspaceLayoutTests.h: added, execution ND until CI.
- Existing RQ001 T03/T03B/T04 source/hitbox tests: snapshot rectangles updated to shared geometry; execution ND.
- Offscreen native JUCE screenshots of LIVE=L01, VIEW=L09, GO=02 at 1180x820, 1647x895 and 1920x1080: capture added, inspect after CI.
- Windows Release/full CTest and VERSION/CMake/changelog guard: ND until exact PR SHA succeeds.
- Physical Windows DPI 100/125/150, keyboard, Narrator/IME, Audio/MIDI/OSC/LTC hardware timing, operator A/B: NOT RUN. CI green is not certification for LIVE.
- Modal Esc/PANIC issue #191 remains a separate open operational risk.

## Recommendation and rollback

Ship only as a small draft PR after exact-SHA Windows CTest and actual JUCE image review. Revert the PR if status copy/contrast, Recent All or transport targets regress; no persisted project migration. No automatic merge.
