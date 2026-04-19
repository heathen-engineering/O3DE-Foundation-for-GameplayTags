# Gameplay Tags Foundation
![License](https://img.shields.io/badge/License-Apache_2.0-blue?style=flat-square)
![Maintained](https://img.shields.io/badge/Maintained%3F-yes-green?style=flat-square)
![O3DE](https://img.shields.io/badge/O3DE-25.10%20%2B-%2300AEEF?style=flat-square&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAyNCAyNCI+PHBhdGggZmlsbD0id2hpdGUiIGQ9Ik0xMiAxTDEgNy40djkuMkwxMiAyM2wxMS02LjRWNy40TDEyIDF6bTkuMSAxNC45TDExLjUgMjEuM2wtOC42LTYuNFY4LjFsOC42LTYuNCA5LjEgNi40djYuOHpNMTEuNSA0LjZMMi45IDkuNnY0LjhsOC42IDUuMSA4LjYtNS4xVjkuNmwtOC42LTUuMHoiLz48L3N2Zz4=)

A lightweight, flexible tag system for [Open 3D Engine (O3DE)](https://o3de.org) projects. Tags are hierarchical, dot-separated names stored as hashed `AZ::u64` values — zero runtime string cost after registration.

- **License:** Apache 2.0
- **Origin:** Heathen Group
- **Platforms:** Windows, Linux, macOS, Android, iOS

## Become a GitHub Sponsor
[![Discord](https://img.shields.io/badge/Discord--1877F2?style=social&logo=discord)](https://discord.gg/6X3xrRc)
[![GitHub followers](https://img.shields.io/github/followers/heathen-engineering?style=social)](https://github.com/heathen-engineering?tab=followers)
Support Heathen by becoming a [GitHub Sponsor](https://github.com/sponsors/heathen-engineering). Sponsorship directly funds the development and maintenance of free tools like this, as well as our game development [Knowledge Base](https://heathen.group/) and community on [Discord](https://discord.gg/6X3xrRc).

Sponsors also get access to our private SourceRepo, which includes developer tools for O3DE, Unreal, Unity, and Godot.
Learn more or explore other ways to support @ [heathen.group/kb](https://heathen.group/kb/do-more/)

---

## What it does

Foundation for Gameplay Tags gives you a structured, hierarchy-aware tag system built on three types:

| Type | Purpose |
|------|---------|
| `GameplayTag` | A single named tag stored as a `u64` hash |
| `GameplayTagCollection` | A container of tags with optional numeric values and set operations |
| `GameplayTagRegistry` | Static registry that tracks parent-child relationships between tags |

Tags follow a dot-separated hierarchy. Registering `"Effects.Buff.Strength"` automatically makes `"Effects.Buff.Strength"` a descendant of both `"Effects.Buff"` and `"Effects"`. All three classes are fully reflected for serialization and exposed to the BehaviorContext for use in Script Canvas and Lua.

---

## Requirements

- O3DE engine **25.10.2** or compatible

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

## Building

Build the gem through the O3DE Project Manager:

---

## Usage overview

### Registering tags

Tags must be registered before hierarchy queries work. Pass a newline-delimited string of fully-qualified tag names to `GameplayTagRegistry::RegisterFromStringData`:

```cpp
#include <GameplayTagRegistry.h>

// Register at startup (e.g., from your system component's Activate())
Heathen::GameplayTagRegistry::RegisterFromStringData(
    "Effects\n"
    "Effects.Buff\n"
    "Effects.Buff.Strength\n"
    "Effects.Buff.Speed\n"
    "Effects.Debuff\n"
    "Effects.Debuff.Slow\n"
    "Status\n"
    "Status.Burning\n"
    "Status.Frozen"
);
```

Tag names must be alphanumeric (plus underscores), dot-separated, and ASCII only. Use `GameplayTagRegistry::ValidateTag` to check a name before registering it.

### Creating tags

```cpp
#include <GameplayTag.h>

// From a registered name
Heathen::GameplayTag buffStrength = Heathen::GameplayTag::Make("Effects.Buff.Strength");

// From a stored hash (e.g., deserialized from an asset)
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

// Add tags
active.AddTag(Heathen::GameplayTag::Make("Status.Burning"));
active.AddTag(Heathen::GameplayTag::Make("Effects.Buff.Speed"));

// Presence check — exactMatch=false includes descendants
bool onFire   = active.Contains(Heathen::GameplayTag::Make("Status.Burning"), true);
bool anyBuff  = active.Contains(Heathen::GameplayTag::Make("Effects.Buff"), false);

// Numeric values — useful for stacking effects
active.Apply(Heathen::GameplayTag::Make("Effects.Buff.Speed"),
             Heathen::GameplayTagOperation::Add, 2);

// Set operations
Heathen::GameplayTagCollection required;
required.AddTag(Heathen::GameplayTag::Make("Effects.Buff.Speed"));
required.AddTag(Heathen::GameplayTag::Make("Status.Burning"));

bool hasAll = active.ContainsAll(required, true);   // false — missing Burning
bool hasAny = active.ContainsAny(required, true);   // true  — has Speed
```

**Script Canvas / Lua** — all three classes are available in the behaviour context. `GameplayTag`, `GameplayTagCollection`, and `GameplayTagRegistry` methods appear under the `Heathen` category.

---

## Public API reference

All public headers live under `Code/Include/`.

### `GameplayTag`

| Method | Description |
|--------|-------------|
| `GameplayTag::Make(name)` | Create a tag by name (hashes the string) |
| `GameplayTag::Cast(id)` | Wrap a previously stored hash |
| `GetId()` | Return the underlying `u64` hash |
| `IsValid()` | True if the hash is non-zero |
| `IsDescendantOf(ancestor)` | True if this tag is a descendant of `ancestor` in the registry |
| `GetDescendants()` | Return a collection of all registered descendants |

### `GameplayTagRegistry`

| Method | Description |
|--------|-------------|
| `Hash(text)` | Hash a string to a `u64` (xxHash3) |
| `RegisterFromStringData(tags)` | Register newline-delimited tags and build hierarchy |
| `IsRegistered(tag)` | Check whether a tag name has been registered |
| `UnregisterFromStringData(tags)` | Remove tags (and their hierarchy entries) |
| `UnregisterAll()` | Clear the entire registry |
| `IsDescendantOf(tag, ancestor)` | Hierarchy check by `u64` IDs |
| `ContainsAnyDescendantOf(collection, ancestor)` | True if any entry in `collection` is a descendant |
| `ContainsOnlyDescendantOf(collection, ancestor)` | True if every entry in `collection` is a descendant |
| `ContainsAllDescendantOf(collection, ancestor)` | True if every registered descendant is present in `collection` |
| `GetDescendants(tag)` | Return all registered descendants as a `u64` set |
| `ValidateTag(tag)` | Validate format without registering |

### `GameplayTagCollection`

| Method | Description |
|--------|-------------|
| `AddTag(tag)` | Add a tag (initial value 1) |
| `RemoveTag(tag)` | Remove a tag |
| `Clear()` | Remove all tags |
| `Apply(tag, operation, value)` | Apply arithmetic to a tag's value; tags reaching 0 are removed |
| `Contains(tag, exactMatch)` | Check presence; `exactMatch=false` matches descendants |
| `ContainsAll(other, exactMatch)` | All tags in `other` must be present |
| `ContainsAny(other, exactMatch)` | At least one tag from `other` must be present |
| `ContainsNone(other, exactMatch)` | No tags from `other` may be present |
| `GetMatchingTags(tag, exactMatch)` | Tags that match or are descendants of `tag` |
| `GetExcludingTags(tag, exactMatch)` | Tags that do not match `tag` |
| `GetShared(other, exactMatch)` | Tags present in both collections |
| `GetExclusive(other, exactMatch)` | Tags present in one collection but not both |
| `GetDescendants(ancestor)` *(static)* | Delegates to `GameplayTagRegistry::GetDescendants` |
| `IsEmpty()` | True if the collection has no tags |
| `Count()` | Number of tags |

`GameplayTagOperation` values: `Set`, `Add`, `Sub`, `Mul`, `Div`, `Min`, `Max`

---

## Public headers

| Header | Contents |
|--------|----------|
| `GameplayTag.h` | Single tag type and factory methods |
| `GameplayTagRegistry.h` | Static registry and hierarchy queries |
| `GameplayTagCollection.h` | Tag container, set operations, and `GameplayTagOperation` enum |
| `FoundationGameplayTags/FoundationGameplayTagsTypeIds.h` | AZ type ID constants |