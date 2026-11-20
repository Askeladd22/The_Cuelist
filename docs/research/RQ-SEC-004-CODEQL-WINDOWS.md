# RQ-SEC-004 — CodeQL C++ security analysis for native Windows show control

Date 2026-11-20. Baseline `main@0799e74e90c73adc55fa9e02a478e213f12d50cf`; official `VERSION.txt` 0.9.83. Feature branch `ci/codeql-cpp-windows-v0984`, target app version 0.9.84; project format `.cuelist` JSON **v10 unchanged**.

## Scenario / source hierarchy

The operator relies on a C++17/JUCE 8 Windows show-control application for live Audio/MIDI/OSC and generated LTC. Unit/integration CTest and manual audits exist, but the canonical GitHub workflow inventory contains `windows-ci.yml` (native Release/CTest) and `version-changelog.yml`, **not** a dedicated CodeQL static security scan. Passing functional tests does not establish absence of use-after-free, unsafe conversions, injection risks or missing validation.

Authoritative documentation:
- GitHub CodeQL action: https://github.com/github/codeql-action — `init` and `analyze`, need `security-events: write` to upload, modes `none/autobuild/manual`.
- GitHub build-mode guidance: https://docs.github.com/en/code-security/how-tos/find-and-fix-code-vulnerabilities/manage-your-configuration/codeql-for-compiled-languages — for compiled C++, manual mode uses the actual build command and offers best extraction accuracy for dependencies and custom build flags.
- GitHub CodeQL Windows C++ requirements: https://docs.github.com/en/code-security/reference/code-scanning/codeql/build-options-for-compiled-languages — Windows MSVC run preserves platform-specific headers and compiler behavior.
- Existing repo source: `.github/workflows/windows-ci.yml`, `scripts/build.ps1`, `CMakeLists.txt` inspected on pinned baseline.

**Categories:** normative GitHub Actions permission contract and technical documentation above, versus actual observed CI and code. No claim that CodeQL, Trail of Bits original review scripts or sanitizers have already executed.

## Hypothesis, counterexamples and choice

**Hypothesis:** a separate least-privilege GitHub Actions workflow on pull requests to main, pushes to main and manual dispatch, using `github/codeql-action/init@v4` + **manual native MSVC Release/CTest** + `analyze@v4` with `security-extended` queries, can build a CodeQL C/C++ database and publish analysis to GitHub Code Scanning for the real project. It must not replace the existing Windows Release/CTest job or modify production runtime.

- **Chosen:** Windows 2022, Visual Studio 2022 x64, exactly `scripts/build.ps1` as an instrumented build after init; dedicated `contents: read`, `actions: read`, `security-events: write` permissions; cancel superseded runs of the same branch; timeout 90 minutes.
- **Rejected:** treating the plugin adaptation of Trail of Bits C++ Review as an executable automated scanner. It is manual review only.
- **Rejected:** Linux autobuild, which can miss Windows-specific JUCE/MSVC sources and does not discover the PowerShell build script reliably; also rejected `none` mode given generated configuration, MSVC definitions and external JUCE.
- **Counterexamples:** Code Scanning account-level settings might block upload; fork PR tokens may be read-only; downloads of JUCE/Google Fonts may fail; CMake/MSVC extraction might find no source code; an action build can succeed but CodeQL may not publish or cover all native sources. Existing CodeQL default setup could conflict and must be checked if CI fails.
- **Supply chain:** official GitHub-maintained actions are used at supported major v4, consistent with current checkout workflow. A later change may pin immutable commit SHAs by separate policy decision; this is not asserted as a complete provenance audit.

## Threat model and operator risk

Threat: LOCAL_UNPRIVILEGED malformed `.cuelist` files, network-exposed OSC input, C++ memory lifetime and concurrent callback paths, plus developer-introduced C++ defects. CodeQL can identify **some** bug patterns; it cannot prove realtime deadline safety, MIDI/OSC delivery, physical PANIC suppression or project integrity. A false-negative is possible, and reported alerts require triage: severity is not established merely by query result.

**No production C++ or operator UI changes:** GO/STOP/PANIC, VIEW/LIVE, Control Maps, editable cue matrix, Undo/Redo, Timeline, Mixer/Master, MIDI/OSC editors, audio callback and LTC are untouched. Versioning changes only app VERSION, CMake, CHANGELOG; JSON schema remains v10.

## PAUL PLAN → APPLY → UNIFY / CCteam QA / Unlazy gates

- **AC-1**: CodeQL workflow is present with correct triggers (PR + push main + manual), minimal permissions and supported v4 actions, Windows 2022 native C++ manual build and `security-extended`.
- **AC-2**: A real PR run of the **new CodeQL job** reaches the `analyze` step with `success` and reports nonzero analyzed compiled source files; no success claim without an observed job log.
- **AC-3**: Existing Windows Release/CTest and version/changelog guard both PASS on exact PR SHA, with no application-source or project-schema changes.
- **AC-4**: Any CodeQL findings are recorded and triaged separately; `CodeQL job green` is **not** equivalent to `no security findings`, `main protected` or `ready for LIVE`.
- **AC-5**: PR remains draft and unmerged until verified; rollback by reverting this workflow/versioning PR, no file migration.

| Gate | Evidence | Status |
| --- | --- | --- |
| Baseline | `main@0799e74`, v0.9.83; no dedicated CodeQL workflow in inspected .github/workflows | VERIFIED |
| CodeQL job | Native Windows C/C++ manual extraction and security-extended analysis, exact SHA | NOT RUN |
| CodeQL alert review | GitHub Code Scanning alerts and changed-source findings | ND |
| Existing Windows Release/CTest | Real Windows job on exact PR SHA | NOT RUN |
| Version/changelog guard | Real GitHub Actions job on exact PR SHA | NOT RUN |
| Hardware bench | Audio/MIDI/OSC/LTC external and Windows DPI | ND — unrelated to this CI-only PR |
| Branch protection | Configure required checks with owner permission after job names stabilize | UNPROTECTED (not changed) |
| Rollback | Revert single scoped PR; do not merge automatically | AVAILABLE |

## Red team checklist and rollback

Check that no GitHub secret is printed, no deployment token or elevated scope besides Code Scanning upload is requested, that workflow triggers on PR changes, that the manual build does not run unreviewed external scripts other than the already-in-use `scripts/build.ps1`, and that scan results are not mistaken for verification of LIVE equipment. CodeQL on PR builds compiles repository content with GitHub runner permissions, so third-party forks and permissions need an explicit governance review.

**Simplify Codebase:** no extra wrapper/build abstraction; reuse existing build script. **Matt Pocock:** testable failure signal is the CodeQL `analyze` step, not a synthetic "red" test of runtime code. **Trail of Bits:** manual source and workflow threat model only. **SEED Application/Deep:** isolated hardening proof of concept; gate WARN until actual CodeQL run succeeds. **PAUL:** PLAN approved by user request to continue; APPLY and UNIFY statuses will be updated from real GitHub Actions logs.
