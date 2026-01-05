# Quick Launcher Guide

## Overview

The Quick Launcher is a Raycast-style interface for rapid task capture and navigation in SamFocus. Press **Ctrl+Space** anywhere in the application to instantly bring up the launcher.

## Opening the Launcher

**Global Hotkey:** `Ctrl+Space`

The launcher appears as a centered, floating window that overlays your current view. Press `Ctrl+Space` again or `Escape` to close it.

## Quick Task Creation

The launcher's primary purpose is lightning-fast task creation with natural language parsing.

### Basic Task Creation

Simply type your task and press `Enter`:

```
Buy groceries
```

This creates a task titled "Buy groceries" in your inbox.

### Natural Language Date Parsing

The launcher understands common date keywords:

```
Call mom tomorrow
Review document today
Submit report next week
Meeting with team monday
Plan weekend friday
```

**Supported keywords:**
- `today` - Sets defer date to today
- `tomorrow` - Sets defer date to tomorrow  
- `next week` - Sets defer date to 7 days from now
- `monday`, `mon` - Sets defer date to next Monday
- `friday`, `fri` - Sets defer date to next Friday

### Due Dates vs Defer Dates

Use the `due` keyword for deadlines:

```
Submit proposal due friday
Complete review due tomorrow
```

Use `defer` or `start` for defer dates:

```
Start project defer next week
Begin planning defer monday
```

### Flag Important Tasks

Mark tasks as important/flagged using `!` or the word "important" or "urgent":

```
Fix critical bug !
Pay rent important
Call client urgent
```

### Combine Multiple Features

You can combine dates, flags, and contexts in one line:

```
Buy birthday gift tomorrow ! @errands
Write report due friday important
Plan meeting monday @work
```

The launcher automatically extracts these keywords and creates a clean task title.

## Quick Navigation

### Search Tasks

Start typing to search through existing tasks:

```
birthday
```

Shows all tasks containing "birthday" in their title.

### Filter by Context

Type `@` followed by context name to filter:

```
@errands
@work
@home
```

### Filter by Project

Type `#` followed by project name:

```
#work
#personal
#home-renovation
```

### Execute Commands

Type `/` to run commands (future feature):

```
/export
/backup
/help
```

## Keyboard Navigation

- **Arrow Up/Down** - Navigate through results
- **Enter** - Execute selected action
- **Escape** - Close launcher
- **Ctrl+Space** - Close launcher

## Usage Examples

### Morning Planning

```
Ctrl+Space
> Review emails @work
Enter

Ctrl+Space  
> Team standup today @work !
Enter

Ctrl+Space
> Lunch with Sarah tomorrow
Enter
```

### Quick Errands List

```
Ctrl+Space
> Buy milk @errands
Enter

Ctrl+Space
> Pick up dry cleaning @errands
Enter

Ctrl+Space
> Post office @errands important
Enter
```

### Project Task Creation

```
Ctrl+Space
> Design mockups defer monday #website-redesign
Enter

Ctrl+Space
> Code review due friday #website-redesign
Enter
```

## Tips & Tricks

1. **Keep It Simple** - The launcher works best with quick, natural language
2. **Use Shortcuts** - `!` for flags, `@` for contexts, `#` for projects
3. **Dates Are Smart** - Say "tomorrow" instead of typing full dates
4. **Muscle Memory** - Ctrl+Space becomes second nature after a few uses
5. **Don't Overthink** - Type naturally and let the parser extract details

## Natural Language Processing

The launcher intelligently extracts:

- **Flags** - Any `!` character or words like "important", "urgent"
- **Dates** - Keywords like "today", "tomorrow", "monday", "next week"
- **Due Dates** - Text containing "due [date]"
- **Defer Dates** - Text containing "defer [date]" or "start [date]"

After extraction, it creates a clean task title by removing these keywords.

### Example Parsing

**Input:**
```
Buy groceries tomorrow @errands important
```

**Result:**
- **Title:** "Buy groceries"
- **Defer Date:** Tomorrow's date
- **Context:** @errands
- **Flagged:** Yes

## Visual Indicators

The launcher shows different icons based on action type:

- ➕ - Add new task
- 🔍 - Search result (existing task)
- 📁 - Project filter
- 🏷️ - Context filter
- ⚡ - Quick command

Selected items are highlighted with a blue background.

## Advanced Features

### Multiple Contexts (Future)

```
Buy gift @shopping @errands
```

### Project Assignment (Future)

```
Write documentation #open-source due monday
```

### Recurrence Patterns (Future)

```
Daily standup every day @work
```

## Comparison with Command Palette

| Feature | Quick Launcher (Ctrl+Space) | Command Palette (Ctrl+K) |
|---------|----------------------------|--------------------------|
| Primary Use | Quick task creation | Search & navigation |
| Natural Language | Yes | No |
| Date Parsing | Yes | No |
| Task Search | Basic | Advanced |
| Commands | Limited | Full |
| Speed | Fastest | Fast |

**Use Quick Launcher when:** You want to capture a task instantly

**Use Command Palette when:** You want to search, navigate, or run commands

## Keyboard Shortcuts Quick Reference

```
Ctrl+Space    Toggle launcher
Ctrl+N        Focus new task input (traditional method)
Ctrl+K        Open command palette
?             Show all keyboard shortcuts
```

## Future Enhancements

Planned features for the launcher:

- [ ] Custom hotkey configuration
- [ ] Multi-context support
- [ ] Project assignment in input
- [ ] Recurrence pattern parsing
- [ ] Time parsing (3pm, 5:30pm)
- [ ] Relative dates (in 3 days, in 2 weeks)
- [ ] Smart suggestions based on history
- [ ] Quick edit for existing tasks
- [ ] Launcher themes and customization

---

**Pro Tip:** The launcher is designed for speed. Don't worry about perfect syntax—just type naturally and press Enter. Most common patterns will be understood automatically!
