# Gameplay Tags Foundation
![License](https://img.shields.io/badge/License-Apache_2.0-blue?style=flat-square)
![Maintained](https://img.shields.io/badge/Maintained%3F-yes-green?style=flat-square)
![O3DE](https://img.shields.io/badge/O3DE-25.10%20%2B-%2300AEEF?style=flat-square&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHBhdGggZmlsbD0id2hpdGUiIGQ9Ik0xMiAxTDEgNy40djkuMkwxMiAyM2wxMS02LjRWNy40TDEyIDF6bTkuMSAxNC45TDExLjUgMjEuM2wtOC42LTYuNFY4LjFsOC42LTYuNCA5LjEgNi40djYuOHpNMTEuNSA0LjZMMi45IDkuNnY0LjhsOC42IDUuMSA4LjYtNS4xVjkuNmwtOC42LTUuMHoiLz48L3N2Zz4=)

A lightweight, flexible tag system for [Open 3D Engine (O3DE)](https://o3de.org) projects. Tags are hierarchical, dot-separated names stored as hashed `AZ::u64` values — zero runtime string cost after registration.

- **License:** Apache 2.0
- **Origin:** Heathen Group
- **Platforms:** Windows, Linux, macOS, Android, iOS

---

## Requirements

- O3DE engine **25.10.2** or compatible
- [xxHash](https://github.com/heathen-engineering/O3DE-xxHash) Gem (provides `xxHash::xxHashFunctions`)

---

## Become a GitHub Sponsor
[![Discord](https://img.shields.io/badge/Discord--1877F2?style=social&logo=discord)](https://discord.gg/6X3xrRc)
[![GitHub followers](https://img.shields.io/github/followers/heathen-engineering?style=social)](https://github.com/heathen-engineering?tab=followers)
Support Heathen by becoming a [GitHub Sponsor](https://github.com/sponsors/heathen-engineering). Sponsorship directly funds the development and maintenance of free tools like this, as well as our game development [Knowledge Base](https://heathen.group/) and community on [Discord](https://discord.gg/6X3xrRc).

Sponsors also get access to our private SourceRepo, which includes developer tools for O3DE, Unreal, Unity, and Godot.
Learn more or explore other ways to support @ [heathen.group/kb](https://heathen.group/kb/do-more/)

---

## What it does

Foundation for Gameplay Tags gives you a structured, hierarchy-aware tag system built on these core types:

| Type | Purpose |
|------|---------|
| `GameplayTag` | A single named tag stored as a `u64` hash |
| `GameplayTagCollection` | A container of tags with optional numeric values and set operations |
| `GameplayTagRegistry` | Static registry that tracks parent-child relationships between tags |
| `GameplayTagCondition` | A single condition that tests a tag's presence or value in a collection |
| `GameplayTagOperation` | A state mutation — applies arithmetic to a tag's value in a collection |

Tags follow a dot-separated hierarchy. Registering `"Effects.Buff.Strength"` automatically makes it a descendant of both `"Effects.Buff"` and `"Effects"`. All types are fully reflected for serialization and exposed to the BehaviorContext for use in Script Canvas and Lua.

---

## Setup

### 1. Add the gem to your O3DE project

Register the gem with the O3DE Project Manager, or add it directly to your project's `project.json`:

```json
"gem_names": [
    "FoundationGameplayTags"
]
```

Then re-run CMake configuration so the build system picks up the new gem.

---

## Gameplay Tags Editor
<img width="379" height="469" alt="image" src="https://github.com/user-attachments/assets/bee7f044-2cb0-47de-bc35-d574758362b2" />

The **Gameplay Tags** panel (under **Heathen Tools** in the O3DE Editor menu bar) provides a visual editor for all `.gptags` files in the project:

- Tree view of the full tag hierarchy across all files
- Inline add, move, and remove controls per tag and per file
- Duplicate tag detection with visual highlighting
- Virtual parent nodes shown where a structural ancestor is implied but not explicitly listed
- File system watcher with debounced reload on external changes
- Per-file `registered` toggle to control whether the Asset Processor builds a `.tagbin` for it

---

## .gptags File Format

Tags are defined in `.gptags` files — UTF-8 JSON documents placed anywhere in the project source tree.

```json
{
  "registered": true,
  "tags": [
    "Effects",
    "Effects.Buff",
    "Effects.Buff.Strength",
    "Effects.Buff.Speed",
    "Effects.Debuff",
    "Status",
    "Status.Burning"
  ]
}
```

| Field | Description |
|-------|-------------|
| `registered` | When `true` the Asset Processor compiles this file into a `.tagbin` binary product that is loaded at runtime. When `false` the file is ignored by the builder (useful for drafts). |
| `tags` | Flat array of fully-qualified dot-path tag names. All intermediate parents are inferred automatically — you do not need to list `"Effects"` separately if `"Effects.Buff"` is already present, though it is good practice to be explicit. |

**Tag naming rules:** ASCII only · alphanumeric plus `_` and `.` · no leading, trailing, or consecutive dots.

### Asset pipeline

The **Gameplay Tag Binary Builder** watches `*.gptags` files. When `registered` is `true` it computes the full descendant map for every tag and outputs a `.tagbin` binary product. At runtime the system component scans the project for all `.tagbin` files and loads them into `GameplayTagRegistry` via `MergeDefaultTags()` — making all project tags available for hierarchy queries before any game logic runs.

`UnregisterAll()` resets the working registry back to these project defaults; it does **not** wipe the registry entirely.

---

## Usage overview

### Registering tags at runtime

Project tags are loaded automatically from `.tagbin` products. You can also register tags programmatically at runtime (e.g. for dynamically generated tags):

```cpp
#include <GameplayTagRegistry.h>

Heathen::GameplayTagRegistry::RegisterFromStringData(
    "Runtime.SpawnedEnemy\n"
    "Runtime.SpawnedEnemy.Elite\n"
    "Runtime.SpawnedEnemy.Minion"
);
```

Use `GameplayTagRegistry::ValidateTag` to check a name before registering it.

### Creating tags

```cpp
#include <GameplayTag.h>

// From a registered name
Heathen::GameplayTag buffStrength = Heathen::GameplayTag::Make("Effects.Buff.Strength");

// From a stored hash (e.g. deserialized from an asset)
AZ::u64 id = Heathen::GameplayTagRegistry::Hash("Effects.Buff.Strength");
Heathen::GameplayTag tag = Heathen::GameplayTag::Cast(id);
```

### Hierarchy queries

```cpp
Heathen::GameplayTag buff    = Heathen::GameplayTag::Make("Effects.Buff");
Heathen::GameplayTag effects = Heathen::GameplayTag::Make("Effects");

buffStrength.IsDescendantOf(buff);     // true
buffStrength.IsDescendantOf(effects);  // true
buff.IsDescendantOf(buffStrength);     // false
```

### Working with collections

```cpp
#include <GameplayTagCollection.h>

Heathen::GameplayTagCollection active;

// Add tags (initial value 1)
active.AddTag(Heathen::GameplayTag::Make("Status.Burning"));
active.AddTag(Heathen::GameplayTag::Make("Effects.Buff.Speed"));

// Presence check — exactMatch=false matches tag or any descendant
bool onFire  = active.Contains(Heathen::GameplayTag::Make("Status.Burning"), true);
bool anyBuff = active.Contains(Heathen::GameplayTag::Make("Effects.Buff"), false);

// Numeric values — useful for stacking effects
active.Apply(Heathen::GameplayTag::Make("Effects.Buff.Speed"),
             Heathen::GameplayTagArithmetic::Add, 2);

AZ::u64 speedStacks = active.GetValue(Heathen::GameplayTag::Make("Effects.Buff.Speed")); // 3

// Tags reaching 0 are removed automatically
active.Apply(Heathen::GameplayTag::Make("Status.Burning"),
             Heathen::GameplayTagArithmetic::Sub, 1); // value was 1, now 0 → removed

// Set operations
Heathen::GameplayTagCollection required;
required.AddTag(Heathen::GameplayTag::Make("Effects.Buff.Speed"));
required.AddTag(Heathen::GameplayTag::Make("Status.Frozen"));

bool hasAll = active.ContainsAll(required, true);  // false — Frozen absent
bool hasAny = active.ContainsAny(required, true);  // true  — Speed present
```

### Conditions and operations

`GameplayTagCondition` and `GameplayTagOperation` are used by systems like FoundationOgham to drive data-driven state machines, but are available for any purpose.

```cpp
#include <GameplayTagCondition.h>
#include <GameplayTagOperation.h>

// Condition: "World.PlayerReputation" must be >= 10
Heathen::GameplayTagCondition repCheck;
repCheck.tag          = Heathen::GameplayTag::Make("World.PlayerReputation");
repCheck.comparison   = Heathen::GameplayTagComparisonOp::GreaterEqual;
repCheck.compareValue = 10;

bool passed = repCheck.Evaluate(myCollection);

// Operation: add 5 to "World.PlayerReputation" if the condition passes
Heathen::GameplayTagOperation giveRep;
giveRep.tag        = Heathen::GameplayTag::Make("World.PlayerReputation");
giveRep.arithmetic = Heathen::GameplayTagArithmetic::Add;
giveRep.value      = 5;
giveRep.conditions = { repCheck };   // optional guard conditions

giveRep.Apply(myCollection);  // applies only if repCheck passes

// Evaluate a list of conditions with AND/OR/Xor logic
AZStd::vector<Heathen::GameplayTagCondition> list = { condA, condB, condC };
bool result = Heathen::EvaluateConditions(list, myCollection);
```

**Script Canvas / Lua** — all types are available in the behaviour context under the `Heathen` category. `GameplayTag`, `GameplayTagCollection`, `GameplayTagRegistry`, `GameplayTagCondition`, and `GameplayTagOperation` are fully usable in Script Canvas graphs and Lua scripts.

---

## Public API reference

All public headers live under `Code/Include/`.

### `GameplayTag`

| Method | Description |
|--------|-------------|
| `GameplayTag::Make(name)` | Create a tag by name (hashes the string) |
| `GameplayTag::Cast(id)` | Wrap a previously stored `u64` hash |
| `GetId()` | Return the underlying `u64` hash |
| `IsValid()` | True if the hash is non-zero |
| `IsDescendantOf(ancestor)` | True if this tag is a descendant of `ancestor` in the registry |
| `GetDescendants()` | Return a collection of all registered descendants |

### `GameplayTagRegistry`

| Method | Description |
|--------|-------------|
| `Hash(text)` | Hash a string to a `u64` (xxHash3\_64bits, seed 0) |
| `RegisterFromStringData(tags)` | Register newline-delimited tags and build hierarchy |
| `UnregisterFromStringData(tags)` | Remove tags from the working set |
| `UnregisterAll()` | Reset the working set to project defaults (tags from `.tagbin` files); does not clear those defaults |
| `IsRegistered(tag)` | Check whether a tag name has been registered |
| `ValidateTag(tag)` | Validate format without registering |
| `IsDescendantOf(tag, ancestor)` | Hierarchy check by `u64` IDs |
| `ContainsAnyDescendantOf(collection, ancestor)` | True if any entry in `collection` is a descendant of `ancestor` |
| `ContainsOnlyDescendantOf(collection, ancestor)` | True if every entry in `collection` is a descendant of `ancestor` |
| `ContainsAllDescendantOf(collection, ancestor)` | True if every registered descendant of `ancestor` is in `collection` |
| `GetDescendants(tag)` | Return all registered descendants as a `u64` set |

### `GameplayTagCollection`

| Method | Description |
|--------|-------------|
| `AddTag(tag)` | Add a tag with value 1 |
| `RemoveTag(tag)` | Remove a tag |
| `Clear()` | Remove all tags |
| `Apply(tag, arithmetic, value)` | Apply `GameplayTagArithmetic` to a tag's value; tags reaching 0 are removed |
| `GetValue(tag)` | Return the tag's current numeric value (0 if absent) |
| `Contains(tag, exactMatch)` | Check presence; `exactMatch=false` matches tag or any descendant |
| `ContainsAll(other, exactMatch)` | All tags in `other` must be present |
| `ContainsAny(other, exactMatch)` | At least one tag from `other` must be present |
| `ContainsNone(other, exactMatch)` | No tags from `other` may be present |
| `GetMatchingTags(tag, exactMatch)` | Tags that match or are descendants of `tag` |
| `GetExcludingTags(tag, exactMatch)` | Tags that do not match `tag` |
| `GetShared(other, exactMatch)` | Tags present in both collections |
| `GetExclusive(other, exactMatch)` | Tags present in one collection but not the other |
| `GetDescendants(ancestor)` *(static)* | Delegates to `GameplayTagRegistry::GetDescendants` |
| `IsEmpty()` | True if the collection has no tags |
| `Count()` | Number of tags |

### `GameplayTagArithmetic`

The arithmetic operator used with `GameplayTagCollection::Apply` and `GameplayTagOperation`.

| Value | Behaviour |
|-------|-----------|
| `Set` | `result = value` |
| `Add` | `result = current + value` |
| `Sub` | `result = max(0, current − value)` |
| `Mul` | `result = current × value` |
| `Div` | `result = current ÷ value` (no-op if value is 0) |
| `Min` | `result = min(current, value)` |
| `Max` | `result = max(current, value)` |

A result of 0 removes the tag from the collection.

### `GameplayTagCondition`

Tests the state of a single tag within a `GameplayTagCollection`.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `tag` | `GameplayTag` | — | The tag to test |
| `comparison` | `GameplayTagComparisonOp` | `Exists` | How to test the tag |
| `compareValue` | `AZ::u64` | `1` | Value to compare against (numeric comparisons only) |
| `exactMatch` | `bool` | `true` | For `Exists`/`NotExists`: `false` also matches descendants |
| `logicOp` | `GameplayTagLogicOp` | `And` | How this condition's result chains with the next |

| Method | Description |
|--------|-------------|
| `Evaluate(collection)` | Returns the bool result of this single condition |

**`GameplayTagComparisonOp` values:**

| Value | Behaviour |
|-------|-----------|
| `Exists` | Tag is present (value ≠ 0) |
| `NotExists` | Tag is absent (value = 0) |
| `Equal` | Stored value == `compareValue` |
| `NotEqual` | Stored value != `compareValue` |
| `Less` | Stored value < `compareValue` |
| `LessEqual` | Stored value <= `compareValue` |
| `Greater` | Stored value > `compareValue` |
| `GreaterEqual` | Stored value >= `compareValue` |

**`GameplayTagLogicOp` values:** `And` · `Or` · `Xor` — evaluated with C-style precedence (AND tightest, then OR, then XOR).

### `GameplayTagOperation`

Applies arithmetic to a tag's value in a collection, with optional guard conditions.

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `tag` | `GameplayTag` | — | The tag to mutate |
| `arithmetic` | `GameplayTagArithmetic` | `Set` | The arithmetic operator |
| `value` | `AZ::u64` | `1` | The operand |
| `conditions` | `vector<GameplayTagCondition>` | empty | Guard conditions; empty = always apply |

| Method | Description |
|--------|-------------|
| `ShouldApply(collection)` | Returns true if all guard conditions pass |
| `Apply(collection)` | Applies the operation if `ShouldApply` is true; returns whether it was applied |

### `EvaluateConditions` (free function)

```cpp
bool Heathen::EvaluateConditions(
    const AZStd::vector<GameplayTagCondition>& conditions,
    const GameplayTagCollection& collection);
```

Reduces a list of conditions to a single bool. The last condition's `logicOp` is ignored. Returns `true` for an empty list (unconditional).

---

## Public headers

| Header | Contents |
|--------|----------|
| `GameplayTag.h` | Single tag type and factory methods |
| `GameplayTagRegistry.h` | Static registry and hierarchy queries |
| `GameplayTagCollection.h` | Tag container, set operations, and `GameplayTagArithmetic` enum |
| `GameplayTagCondition.h` | `GameplayTagCondition`, `GameplayTagComparisonOp`, `GameplayTagLogicOp`, and `EvaluateConditions` |
| `GameplayTagOperation.h` | `GameplayTagOperation` struct |
| `FoundationGameplayTags/FoundationGameplayTagsTypeIds.h` | AZ type ID constants |
