# arc

A lightweight CLI to orchestrate tmux sessions for AI coding agents.

Spin up a multiplexed terminal environment — multiple panes, each running
its own shell — and drive them all from a single command. Designed for
workflows where several AI agents work in parallel on the same codebase.

---

## How it works

```
arc start -p 4 -n agents
```

If you are **already inside tmux**, arc creates the session directly in your
existing tmux server:

```
  existing tmux server
  ┌──────────────────────────────────────────┐
  │  session: agents                         │
  │                                          │
  │  ┌───────────────┬──────────────────┐    │
  │  │  pane 0       │  pane 1          │    │
  │  │               │                  │    │
  │  │  $ fish -il   │  $ fish -il      │    │
  │  │               │                  │    │
  │  ├───────────────┼──────────────────┤    │
  │  │  pane 2       │  pane 3          │    │
  │  │               │                  │    │
  │  │  $ fish -il   │  $ fish -il      │    │
  │  │               │                  │    │
  │  └───────────────┴──────────────────┘    │
  └──────────────────────────────────────────┘
```

If you are **outside tmux**, arc opens a new kitty window with tmux inside:

```
  kitty window
  ┌──────────────────────────────────────────┐
  │  tmux session: agents                    │
  │                                          │
  │  ┌───────────────┬──────────────────┐    │
  │  │  pane 0       │  pane 1          │    │
  │  │  $ fish -il   │  $ fish -il      │    │
  │  ├───────────────┼──────────────────┤    │
  │  │  pane 2       │  pane 3          │    │
  │  │  $ fish -il   │  $ fish -il      │    │
  │  └───────────────┴──────────────────┘    │
  └──────────────────────────────────────────┘
```

Panes are distributed equitably using tmux's `tiled` layout.

`arc` stays alive after launch — press **Ctrl-C** to close the session
automatically. If the session was already closed manually, arc detects this
and exits cleanly without error.

---

Then send commands to any pane without touching the keyboard:

```
arc send -n agents 0 claude
arc send -n agents 1 opencode
arc send -n agents 2 cursor-cli
arc send -n agents 3 git log --oneline
```

Each command is sent as text and executed immediately (Enter included).

Or broadcast the same command to every pane at once:

```
arc send -n agents all "git pull"
```

```
  session: agents
  ┌───────────────┬──────────────────┐
  │  pane 0       │  pane 1          │
  │  $ git pull ◀─┤─ arc send        │
  ├───────────────┤   all "git pull" │
  │  pane 2       │  pane 3          │
  │  $ git pull ◀─┤◀─────────────────┘
  └───────────────┴──────────────────┘
```

---

## Session file

Pre-configure panes with commands using a TOML session file:

```
arc session
arc session set -n agents
arc session add -p "0:claude" -p "1:opencode" -p "2:cursor-cli"
arc session set --all_panes "cd /project"   # optional: broadcast on start
```

This writes `.arc.toml`:

```toml
version = "0.1.0"
name    = "agents"
all_panes = "cd /project"

[panes]
0 = "claude"
1 = "opencode"
2 = "cursor-cli"
```

`all_panes` is broadcast to every pane after pane-specific commands; `#N` is
replaced with each pane's index.

Then start with the session file — agents launch automatically in each pane:

```
arc -s .arc.toml start -p 3 -n agents
```

```
  arc start
      │
      ├─ pane 0 ──► $ claude
      ├─ pane 1 ──► $ opencode
      └─ pane 2 ──► $ cursor-cli
```

---

## Commands

### `arc start`

Launch a named tmux session.

```
arc start [options]

  -p, --panes  <N>       Number of panes (default: 2, minimum: 1)
      --shell  <shell>   Shell in each pane (default: fish -il)
  -n, --name   <name>    tmux session name (default: arc-0)
  -h, --help             Show help
```

> Note: use `--shell` (long form) — `-s` is reserved for the global
> `--session` option.

Fails if a session with the same name already exists.

**Examples:**

```sh
# Two panes, default shell, session arc-0
arc start

# Four panes with zsh
arc start -p 4 --shell zsh

# Single pane, custom name
arc start -p 1 -n editor

# Three panes with bash
arc start -p 3 --shell bash -n agents
```

---

### `arc send`

Send a command to a pane — or broadcast to all panes.

```
arc send [options] <pane|all> <text...>

  -n, --name   <name>      tmux session name (default: arc-0)
  -d, --delay  <seconds>   Delay before sending to a single pane,
                           or between panes when using all (default: 0)
  -h, --help               Show help

  <pane|all>               Pane index (0-based) or 'all'
  <text...>                Command or text to send (multi-word supported);
                           #N is replaced with the pane index at send time
```

**Examples:**

```sh
# Send "git status" to pane 0 of the default session
arc send 0 git status

# Send to a specific session and pane
arc send -n agents 1 claude

# Broadcast to every pane
arc send all "git pull"
arc send -n agents all "echo ready"

# Broadcast with a 2-second delay between panes
arc send -d 2 all "claude"

# Use #N to inject pane index (e.g. "echo pane #N" → "echo pane 0", "echo pane 1", ...)
arc send -n agents all "echo I am pane #N"
```

---

### `arc close`

Kill a tmux session and all its panes.

```
arc close [options]

  -n, --name  <name>     tmux session name (default: arc-0)
  -h, --help             Show help
```

**Examples:**

```sh
# Close the default session
arc close

# Close a named session
arc close -n agents
```

---

### `arc session`

Initialize or update the TOML session file (`.arc.toml` by default).

```sh
# Create or refresh the file (writes version)
arc session

# Set the session name
arc session set -n agents

# Set a command broadcast to all panes on start (#N = pane index)
arc session set --all_panes "cd /project"

# Add pane commands (repeatable)
arc session add -p "0:claude" -p "1:opencode"
```

---

### `arc worktrees`

Manage git worktrees of the repository in the current directory.

```
arc worktrees [options] <subcommand>

  -r, --regex  <pattern>   Filter worktrees by name (default: wt-\d)
  -h, --help               Show help

Subcommands:
  list     Print matching worktrees and their current branch
  prune    Remove matching worktrees and delete their local branches
  cp       Copy a file into each matching worktree
  restore  Restore each matching worktree to a clean state
```

The `--regex` pattern is matched against the worktree name using
`std::regex_search`. All operations are independent per worktree — if one
fails it is skipped without stopping the rest.

**`list`**

```sh
# List worktrees matching the default pattern (wt-0, wt-1, ...)
arc worktrees list

# List with a custom pattern
arc worktrees --regex "feature-.*" list
arc worktrees -r "wt-[0-3]" list
```

**`prune`**

Removes each matching worktree and deletes its local branch. Each operation
(remove worktree / delete branch) is independent — if one is missing or fails
it is skipped without error.

```sh
arc worktrees prune
arc worktrees --regex "wt-[0-3]" prune
arc worktrees -r "feature-.*" prune
```

**`cp <file>`**

Copies a file from the current directory into each matching worktree.
`#N` in the filename is replaced with the worktree's index suffix (e.g.
`worker-#N.md` → `worker-0.md` in `wt-0`, `worker-1.md` in `wt-1`).
The number of iterations is determined automatically by counting matching
worktrees.

```sh
arc worktrees cp worker-#N.md
arc worktrees -r "wt-[0-1]" cp worker-#N.md
```

**`restore`**

Restores each matching worktree to a clean state:
1. Checks out HEAD with force — discards all local modifications to tracked files (`git checkout .` equivalent).
2. Removes all untracked files and directories (`git clean -fd` equivalent).

```sh
arc worktrees restore
arc worktrees -r "wt-[0-1]" restore
```

---

### `arc help`

List all available commands and global options.

```
arc help
arc --help
arc
```

All three forms show the same full command reference.

---

## Global options

```
arc [global options] <command> [command options]

  -s, --session <file>   Session TOML file (default: .arc.toml)
```

---

## Typical workflow

```sh
# 1. Configure the session file
arc session
arc session set -n project
arc session add -p "0:claude" -p "1:opencode" -p "2:git log --oneline"

# 2. Start — agents launch automatically in their panes
#    (arc stays alive; Ctrl-C closes everything)
arc -s .arc.toml start -p 3 -n project

# 3. Send instructions to agents at any time
arc send -n project 0 "implement the auth module"
arc send -n project 1 "review the changes in src/auth"

# 4. Broadcast a command to all agents at once
arc send -n project all "git status"

# 5. Copy an updated config file to every worktree
arc worktrees cp worker-#N.md

# 6. Reset all worktrees to a clean state before a new task
arc worktrees restore

# 7. Clean up worktrees when an agent's branch is merged
arc worktrees -r "wt-[0-2]" prune

# 8. Tear it all down when done
arc close -n project
```

---

## License

GNU Affero General Public License v3.0 — see [LICENSE](LICENSE).
