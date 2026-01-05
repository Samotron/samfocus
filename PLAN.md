Absolutely. Below is a **clear, realistic development plan** tailored exactly to your situation:

* Loved OmniFocus
* Windows user
* Personal use only
* Happy with C + Dear ImGui
* Wants something usable quickly, not a 2-year yak shave

This plan is designed so that **you start using the tool early**, which is the key to actually finishing it.

---

# Personal OmniFocus-Style App — Development Plan

## Guiding Principles (print this)

1. **Inbox first**
2. **Keyboard over mouse**
3. **Local-first, offline always**
4. **No features unless they reduce friction**
5. **Ship something usable every weekend**

If a feature violates these, don’t build it.

---

## Phase 0 — Ground rules (½ day)

### Decisions (lock these in)

* Language: **C**
* UI: **Dear ImGui**
* Storage: **SQLite**
* Platform: **Windows only**
* Sync: **None (manual backup only)**

Write this at the top of the repo. Don’t revisit it.

---

## Phase 1 — Skeleton & persistence (Weekend 1)

### Goals

* App opens instantly
* You can add a task
* It survives a restart

### Tasks

* [ ] Create Win32 + GLFW + ImGui shell
* [ ] Create SQLite DB on first run
* [ ] `tasks` table (minimal)
* [ ] Insert + load tasks
* [ ] Hard-coded Inbox view

### Data model (minimal)

```sql
tasks (
  id INTEGER PRIMARY KEY,
  title TEXT NOT NULL,
  project_id INTEGER NULL,
  status INTEGER,       -- inbox, active, done
  created_at INTEGER
)
```

### Exit criteria

> You can type a task, close the app, reopen it, and it’s still there.

If this fails, stop and fix it.

---

## Phase 2 — Inbox + keyboard flow (Weekend 2)

### Goals

* Zero-friction capture
* Mouse optional

### Tasks

* [ ] Inline task editing
* [ ] `Enter` to add
* [ ] `Esc` to cancel
* [ ] `Ctrl+N` always creates a task
* [ ] Delete + complete shortcuts

### UX rule

> Inbox must always be one keystroke away.

---

## Phase 3 — Projects (Weekend 3)

### Goals

* Replace “flat list” thinking
* Enable real GTD structure

### Tasks

* [ ] `projects` table
* [ ] Assign task → project
* [ ] Project list in sidebar
* [ ] Sequential projects (only!)

### Data

```sql
projects (
  id INTEGER PRIMARY KEY,
  title TEXT NOT NULL,
  type INTEGER -- sequential
)
```

### Rule

Only **next available action** in a sequential project is “active”.

---

## Phase 4 — Defer & due dates (Weekend 4)

### Goals

* Hide future work
* See what matters today

### Tasks

* [ ] `defer_at`, `due_at`
* [ ] Availability logic
* [ ] “Today” perspective (hard-coded)

### Availability logic

```
available =
  now >= defer_at
  AND not completed
  AND (sequential rules)
```

---

## Phase 5 — Tags / contexts (Weekend 5)

### Goals

* Work / Home / Errands etc.
* Replace OmniFocus contexts

### Tasks

* [ ] `tags` table
* [ ] Many-to-many task ↔ tag
* [ ] Filter by tag
* [ ] Quick tag assignment

---

## Phase 6 — Review mode (Weekend 6)

### This is the hard part (and the payoff)

### Goals

* Trust the system
* Weekly review feels good

### Tasks

* [ ] `last_reviewed_at` on projects
* [ ] Review screen
* [ ] Keyboard-only flow:

  * Review → skip → complete → defer

### Rule

> If review feels annoying, you won’t use the app.

---

## Phase 7 — Perspectives (Weekend 7)

### Goals

* OmniFocus superpower
* Personal magic

### Tasks

* [ ] Perspective struct
* [ ] Load from config file
* [ ] Filters + grouping
* [ ] Quick switch (`Ctrl+1..9`)

Example:

```ini
[perspective.work_today]
filter = "tag:work AND available AND due<=today"
```

---

## Phase 8 — Polish that matters (Weekend 8)

### Only do things that save time

* [ ] Command palette (`Ctrl+K`)
* [ ] Plain-text capture parsing
* [ ] Faster navigation
* [ ] Manual backup/export

No animations. No theming rabbit holes.

---

## What to consciously *not* build

❌ Sync
❌ Mobile
❌ Collaboration
❌ Notifications
❌ Drag & drop
❌ AI

If you feel tempted — open OmniFocus and ask *why* you liked it.

---

## Success criteria (very important)

After ~2 months:

* You **actually use it daily**
* You don’t miss OmniFocus
* You trust the data
* You enjoy maintaining it

That’s a complete success.

---

## Final advice (from experience)

The danger isn’t technical difficulty — it’s **over-respecting OmniFocus**.

You don’t need to clone it.
You need to **capture what made it click for you**.

---

If you want next, I can:

* Turn this into a **GitHub checklist**
* Write the **initial SQLite schema + C structs**
* Design the **keyboard map**
* Sketch the **ImGui layout**

Which one do you want to do first?

