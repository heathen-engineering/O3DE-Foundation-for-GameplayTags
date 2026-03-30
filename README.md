# Foundation for GameplayTags (O3DE Gem)

A lightweight gameplay tagging system based on 64-bit xxHash identifiers.

GameplayTags are deterministic u64 values generated from validated ASCII strings. When identical hashing rules and inputs are used, values are stable across platforms and projects.

---

## Core Concepts

### GameplayTag
- 64-bit identifier (`u64`)
- Derived from validated string input
- Backed by xxHash64
- Used as the fundamental gameplay classification token

String rules:
- ASCII alphanumeric and `_`
- `.` allowed as a hierarchical separator with strict validation
- Invalid patterns such as empty segments are rejected (eg `Effect..Buff`)

---

### GameplayTagRegistry (Optional)
An opt-in structural layer used for hierarchical awareness.

- Not a source of truth
- Builds derived hierarchy from tag strings
- Enables descendant-based queries when enabled
- Used purely for optimisation and set expansion

---

### GameplayTagCollection
A runtime set of GameplayTags supporting basic set operations and optional hierarchical queries via the registry.

---

## Design Notes

- Deterministic hashing via xxHash64
- Cross-platform stability depends on identical hashing parameters and string rules
- No required global tag database
- Registry is optional and purely performance/structure oriented
- Designed for gameplay systems requiring fast classification and filtering
