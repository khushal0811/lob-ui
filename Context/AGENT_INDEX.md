# Agent index — lob-qt

Read this file at the start of every session before doing anything else.
Then read only the sections you need from the files listed below.

---

## Files in this folder

### AGENT_INDEX.md (this file)
- Read first, every session, no exceptions
- Tells you what to read and where to find it
- Update "Current project state" after every completed part

### scope.md
- Read when: you need to know what is in scope vs out of scope
- Contains: UI features, Qt modules used, threading model, excluded features

### architecture.md
- Read when: you need to understand how the UI, bridge, and engine connect
- Contains: full component diagram, threading model, signal/slot map, directory structure, key design decisions

### build-plan.md
- Read when: you want a quick overview of the phases and what each delivers
- Contains: one-paragraph summary per phase, summary table

### implementation-plan.md
- Read when: you are about to implement any part of any phase
- Contains: exact file paths, full code, commit messages, checklists
- How to use: find the current phase and part number, read only that section

---

## Project context

This project builds a Qt 6 desktop UI that wraps the existing `lob-engine` C++ matching engine.
The engine is a compiled static library (`liblob_core.a`) — it must not be modified.
The UI is a separate application that links against the engine.

**Engine location:** `../lob-engine/` (sibling directory)
**This project location:** `/Users/apple/Documents/Career/Project_1/lob-ui/`

The engine does NOT know Qt exists. Qt does NOT run matching logic. The bridge decouples them.

---

## Current project state

- Current phase: 5
- Current part: 5.1
- Last completed part: 4.3 (Phase 4 complete — MetricsPanel, SpreadChart, full observability layer)
- Last commit: none yet (commits pending)

---

## Rules for every session

1. Read this file first
2. Check "Current project state" to know where you are
3. Read only the relevant section of `implementation-plan.md` for the current part
4. Read `architecture.md` if you are unsure how components connect
5. Implement exactly as specified — no deviations
6. Write files to: `/Users/apple/Documents/Career/Project_1/lob-ui/`
7. Never modify anything inside `../lob-engine/src/` — engine is read-only
8. After completing each part, tell the user to run cmake and the app before moving on
9. After completing each part, tell the user to update "Current project state" in this file
10. Never read the full implementation-plan.md in one go — read only the current part

---

## Critical constraints — read before every coding session

- The matching engine runs on a **dedicated worker thread** — never on the Qt UI thread
- All communication between UI and engine goes through the **EngineWorker bridge**
- Qt widgets are **never** touched from the worker thread — only from the UI thread
- Use **Qt signals/slots** for all cross-thread communication
- The engine's `liblob_core.a` and headers are included via CMake target — do not copy source files
- Every UI update must be driven by engine output — never poll engine state directly from UI
