/*
 * Copyright (c) 2026 Heathen Engineering Limited
 * Irish Registered Company #556277
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h>
#include <AzCore/std/containers/vector.h>
#include "GameplayTag.h"


namespace Heathen
{
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(GameplayTagOperation, AZ::u8,
        Set,
        Add,
        Sub,
        Mul,
        Div,
        Min,
        Max
    );

    AZ_ENUM_DEFINE_REFLECT_UTILITIES(GameplayTagOperation)
    AZ_TYPE_INFO_SPECIALIZE(Heathen::GameplayTagOperation, "{F3A7B2C9-D4E1-4F8A-B6C3-2E9D5A7F1B4E}");

    /// <summary>
    /// A collection of GameplayTags each with an associated u64 value.
    /// Value invariant: a value of 0 is never stored. Tags are removed when their value reaches 0.
    /// Simple presence (AddTag) stores a value of 1.
    /// Registry-aware operations require tags to be registered in GameplayTagRegistry.
    /// Unregistered tags are treated as not found with no error or exception.
    /// </summary>
    class GameplayTagCollection
    {
    public:
        AZ_TYPE_INFO(GameplayTagCollection, "{A8C2F5B3-6E3A-4F2A-9C2D-9B7F7C4A1E11}");
        AZ_CLASS_ALLOCATOR(GameplayTagCollection, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        // -----------------------------------------------------------------------
        // Mutation
        // -----------------------------------------------------------------------

        /// <summary>Adds the tag with a value of 1. Shortcut for Apply(tag, Set, 1).</summary>
        void AddTag(const GameplayTag& tag);

        /// <summary>Removes the tag unconditionally. Shortcut for Apply(tag, Set, 0).</summary>
        void RemoveTag(const GameplayTag& tag);

        /// <summary>Removes all tags from the collection.</summary>
        void Clear();

        /// <summary>
        /// Applies an arithmetic operation to the tag's current value using the input value.
        /// If the result is 0 the tag is removed.
        /// Invalid tags (id == 0) are ignored.
        /// </summary>
        void Apply(const GameplayTag& tag, GameplayTagOperation operation, AZ::u64 value);

        // -----------------------------------------------------------------------
        // Presence checks - single tag
        // -----------------------------------------------------------------------

        /// <summary>
        /// Returns true if this collection contains the tag.
        /// exactMatch = true  : the tag must be present exactly.
        /// exactMatch = false : the tag OR any of its descendants must be present.
        ///                      Requires the tag to be registered. Returns false if not registered.
        /// </summary>
        bool Contains(const GameplayTag& tag, bool exactMatch) const;

        // -----------------------------------------------------------------------
        // Presence checks - collection vs collection
        // -----------------------------------------------------------------------

        /// <summary>
        /// Returns true if this collection contains every tag in other.
        /// exactMatch = true  : exact tag matches only.
        /// exactMatch = false : each tag in other may be matched by itself or any descendant.
        ///                      Requires tags to be registered. Unregistered tags return false.
        /// </summary>
        bool ContainsAll(const GameplayTagCollection& other, bool exactMatch) const;

        /// <summary>
        /// Returns true if this collection contains at least one tag from other.
        /// exactMatch = true  : exact tag matches only.
        /// exactMatch = false : a match may be the tag itself or any of its descendants.
        ///                      Requires tags to be registered. Unregistered tags return false.
        /// </summary>
        bool ContainsAny(const GameplayTagCollection& other, bool exactMatch) const;

        /// <summary>
        /// Returns true if this collection contains no tags from other.
        /// exactMatch = true  : exact tag matches only.
        /// exactMatch = false : also checks descendants of each tag in other.
        ///                      Requires tags to be registered. Unregistered tags return false.
        /// </summary>
        bool ContainsNone(const GameplayTagCollection& other, bool exactMatch) const;

        // -----------------------------------------------------------------------
        // Query
        // -----------------------------------------------------------------------

        /// <summary>
        /// Returns all tags in this collection that match the given tag.
        /// exactMatch = true  : returns only the tag itself if present.
        /// exactMatch = false : returns the tag and any of its descendants present in this collection.
        ///                      Requires the tag to be registered. Returns empty if not registered.
        /// </summary>
        AZStd::vector<GameplayTag> GetMatchingTags(const GameplayTag& tag, bool exactMatch) const;

        /// <summary>
        /// Returns all tags in this collection that do not match the given tag.
        /// exactMatch = true  : excludes only the exact tag.
        /// exactMatch = false : excludes the tag and any of its descendants.
        ///                      Requires the tag to be registered. Returns all tags if not registered.
        /// </summary>
        AZStd::vector<GameplayTag> GetExcludingTags(const GameplayTag& tag, bool exactMatch) const;

        /// <summary>
        /// Returns a new collection containing only tags present in both collections.
        /// exactMatch = true  : exact tag matches only.
        /// exactMatch = false : a tag from other matches if this collection contains it or any descendant.
        ///                      Requires tags to be registered. Unregistered tags are skipped.
        /// </summary>
        GameplayTagCollection GetShared(const GameplayTagCollection& other, bool exactMatch) const;

        /// <summary>
        /// Returns a new collection containing tags present in one collection but not both.
        /// exactMatch = true  : exact tag matches only.
        /// exactMatch = false : descendant matches are considered when determining exclusivity.
        ///                      Requires tags to be registered. Unregistered tags are skipped.
        /// </summary>
        GameplayTagCollection GetExclusive(const GameplayTagCollection& other, bool exactMatch) const;

        /// <summary>
        /// Static registry passthrough.
        /// Returns all tags registered as descendants of the given ancestor tag.
        /// Returns empty set if the tag is not registered.
        /// </summary>
        static AZStd::vector<GameplayTag> GetDescendants(const GameplayTag& ancestor);

        // -----------------------------------------------------------------------
        // Utility
        // -----------------------------------------------------------------------

        /// <summary>Returns true if the collection contains no tags.</summary>
        bool IsEmpty() const;

        /// <summary>Returns the number of tags currently in the collection.</summary>
        AZ::u32 Count() const;

        // -----------------------------------------------------------------------
        // C++ only - not registered in BehaviorContext
        // -----------------------------------------------------------------------

        // C++ only - not script exposed, semantically niche
        // Returns true if every registered descendant of ancestor is present in this collection
        bool ContainsAllDescendants(const GameplayTag& ancestor) const;

    private:
        // key   = tag hash (u64)
        // value = raw u64 value, treated as integer unless caller manages otherwise
        // invariant: value of 0 is never stored, key is erased when result reaches 0
        AZStd::unordered_map<AZ::u64, AZ::u64> m_tags;
    };
}