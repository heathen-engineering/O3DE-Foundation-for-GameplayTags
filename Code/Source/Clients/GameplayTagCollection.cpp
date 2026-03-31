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

#include "GameplayTagCollection.h"
#include "GameplayTagRegistry.h"

namespace Heathen
{
    void GameplayTagCollection::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Enum<GameplayTagOperation>()
                ->Value("Set", GameplayTagOperation::Set)
                ->Value("Add", GameplayTagOperation::Add)
                ->Value("Sub", GameplayTagOperation::Sub)
                ->Value("Mul", GameplayTagOperation::Mul)
                ->Value("Div", GameplayTagOperation::Div)
                ->Value("Min", GameplayTagOperation::Min)
                ->Value("Max", GameplayTagOperation::Max);

            serialize->Class<GameplayTagCollection>()
                ->Version(1)
                ->Field("Tags", &GameplayTagCollection::m_tags);
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            GameplayTagOperationReflect(*behavior);

            behavior->Class<GameplayTagCollection>("Gameplay Tag Collection")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay Tags")
                ->Constructor<>()
                ->Method("Add Tag", &GameplayTagCollection::AddTag,
                    {{{ "Tag", "The tag to add with a value of 1" }}})
                ->Method("Remove Tag", &GameplayTagCollection::RemoveTag,
                    {{{ "Tag", "The tag to remove unconditionally" }}})
                ->Method("Clear", &GameplayTagCollection::Clear)
                ->Method("Apply", &GameplayTagCollection::Apply,
                    {{
                        { "Tag", "The tag to apply the operation to" },
                        { "Operation", "The arithmetic operation to apply",
                            behavior->MakeDefaultValue(static_cast<int>(GameplayTagOperation::Set)) },
                        { "Value", "The value to use in the operation",
                            behavior->MakeDefaultValue(static_cast<AZ::u64>(1)) }
                    }},
                        "Applies an arithmetic operation to a tag's value")
                ->Method("Contains", &GameplayTagCollection::Contains,
                    {{
                        { "Tag", "The tag to check for" },
                        { "Exact Match", "True = exact match only, False = include descendants",
                            behavior->MakeDefaultValue(true) }
                    }},
                        "Returns true if this collection contains the tag")
                ->Method("Contains All", &GameplayTagCollection::ContainsAll,
                    {{{ "Other", "The collection of tags that must all be present" },
                      { "Exact Match", "True = exact match only, False = include descendants" }}})
                ->Method("Contains Any", &GameplayTagCollection::ContainsAny,
                    {{{ "Other", "The collection to check at least one tag from" },
                      { "Exact Match", "True = exact match only, False = include descendants" }}})
                ->Method("Contains None", &GameplayTagCollection::ContainsNone,
                    {{{ "Other", "The collection to check no tags from" },
                      { "Exact Match", "True = exact match only, False = include descendants" }}})
                ->Method("Get Matching Tags", &GameplayTagCollection::GetMatchingTags,
                    {{{ "Tag", "The tag to match against" },
                      { "Exact Match", "True = exact match only, False = include descendants" }}})
                ->Method("Get Excluding Tags", &GameplayTagCollection::GetExcludingTags,
                    {{{ "Tag", "The tag to exclude" },
                      { "Exact Match", "True = exact match only, False = include descendants" }}})
                ->Method("Get Shared", &GameplayTagCollection::GetShared,
                    {{{ "Other", "The collection to intersect with" },
                      { "Exact Match", "True = exact match only, False = include descendants" }}})
                ->Method("Get Exclusive", &GameplayTagCollection::GetExclusive,
                    {{{ "Other", "The collection to find symmetric difference with" },
                      { "Exact Match", "True = exact match only, False = include descendants" }}})
                ->Method("Get Descendants", &GameplayTagCollection::GetDescendants,
                    {{{ "Ancestor", "The ancestor tag to get descendants of" }}})
                ->Method("Is Empty", &GameplayTagCollection::IsEmpty)
                ->Method("Count", &GameplayTagCollection::Count);
        }
    }

    void GameplayTagCollection::AddTag(const GameplayTag& tag)
    {

        Apply(tag, GameplayTagOperation::Set, 1);
    }

    void GameplayTagCollection::RemoveTag(const GameplayTag& tag)
    {
        Apply(tag, GameplayTagOperation::Set, 0);
    }

    void GameplayTagCollection::Clear()
    {
        m_tags.clear();
    }

    void GameplayTagCollection::Apply(const GameplayTag& tag, GameplayTagOperation operation, AZ::u64 value)
    {
        if (!tag.IsValid())
        {
            return;
        }

        const AZ::u64 id = tag.GetId();
        const auto it = m_tags.find(id);
        const AZ::u64 current = (it != m_tags.end()) ? it->second : 0;
        AZ::u64 result = 0;

        switch (operation)
        {
        case GameplayTagOperation::Set:
            result = value;
            break;
        case GameplayTagOperation::Add:
            result = current + value;
            break;
        case GameplayTagOperation::Sub:
            result = (current > value) ? current - value : 0;
            break;
        case GameplayTagOperation::Mul:
            result = current * value;
            break;
        case GameplayTagOperation::Div:
            result = (value != 0) ? current / value : current;
            break;
        case GameplayTagOperation::Min:
            result = (current < value) ? current : value;
            break;
        case GameplayTagOperation::Max:
            result = (current > value) ? current : value;
            break;
        default:
            result = current;
            break;
        }

        if (result == 0)
        {
            if (it != m_tags.end())
            {
                m_tags.erase(it);
            }
        }
        else
        {
            m_tags[id] = result;
        }
    }

    bool GameplayTagCollection::Contains(const GameplayTag& tag, bool exactMatch) const
    {
        if (!tag.IsValid())
        {
            return false;
        }

        const AZ::u64 id = tag.GetId();

        if (exactMatch)
        {
            return m_tags.find(id) != m_tags.end();
        }

        // Non-exact: check tag itself first before touching the registry
        if (m_tags.find(id) != m_tags.end())
        {
            return true;
        }

        // Delegate to registry - handles unregistered tags gracefully
        // uses smaller-set iteration internally for efficiency
        return GameplayTagRegistry::ContainsAnyDescendantOf(m_tags, id);
    }

    bool GameplayTagCollection::ContainsAll(const GameplayTagCollection& other, bool exactMatch) const
    {
        if (other.m_tags.empty())
        {
            return true;
        }

        for (const auto& pair : other.m_tags)
        {
            if (!Contains(GameplayTag(pair.first), exactMatch))
            {
                return false;
            }
        }

        return true;
    }

    bool GameplayTagCollection::ContainsAny(const GameplayTagCollection& other, bool exactMatch) const
    {
        if (other.m_tags.empty())
        {
            return false;
        }

        for (const auto& pair : other.m_tags)
        {
            if (Contains(GameplayTag(pair.first), exactMatch))
            {
                return true;
            }
        }

        return false;
    }

    bool GameplayTagCollection::ContainsNone(const GameplayTagCollection& other, bool exactMatch) const
    {
        if (other.m_tags.empty())
        {
            return true;
        }

        for (const auto& pair : other.m_tags)
        {
            if (Contains(GameplayTag(pair.first), exactMatch))
            {
                return false;
            }
        }

        return true;
    }

    AZStd::vector<GameplayTag> GameplayTagCollection::GetMatchingTags(const GameplayTag& tag, bool exactMatch) const
    {
        AZStd::vector<GameplayTag> result;

        if (!tag.IsValid())
        {
            return result;
        }

        const AZ::u64 id = tag.GetId();

        if (exactMatch)
        {
            if (m_tags.find(id) != m_tags.end())
            {
                result.push_back(tag);
            }
            return result;
        }

        // Non-exact: check tag itself first
        if (m_tags.find(id) != m_tags.end())
        {
            result.push_back(tag);
        }

        // Then collect any descendants present in this collection
        const AZStd::unordered_set<AZ::u64> descendants = GameplayTagRegistry::GetDescendants(id);
        for (const AZ::u64 descendantId : descendants)
        {
            if (m_tags.find(descendantId) != m_tags.end())
            {
                result.push_back(GameplayTag(descendantId));
            }
        }

        return result;
    }

    AZStd::vector<GameplayTag> GameplayTagCollection::GetExcludingTags(const GameplayTag& tag, bool exactMatch) const
    {
        AZStd::vector<GameplayTag> result;

        if (!tag.IsValid())
        {
            // Invalid tag - return everything
            for (const auto& pair : m_tags)
            {
                result.push_back(GameplayTag(pair.first));
            }
            return result;
        }

        const AZ::u64 id = tag.GetId();

        if (exactMatch)
        {
            for (const auto& pair : m_tags)
            {
                if (pair.first != id)
                {
                    result.push_back(GameplayTag(pair.first));
                }
            }
            return result;
        }

        // Non-exact: build exclusion set from tag and all its descendants
        AZStd::unordered_set<AZ::u64> exclusionSet = GameplayTagRegistry::GetDescendants(id);
        exclusionSet.insert(id);

        for (const auto& pair : m_tags)
        {
            if (exclusionSet.find(pair.first) == exclusionSet.end())
            {
                result.push_back(GameplayTag(pair.first));
            }
        }

        return result;
    }

    GameplayTagCollection GameplayTagCollection::GetShared(const GameplayTagCollection& other, bool exactMatch) const
    {
        GameplayTagCollection result;

        if (m_tags.empty() || other.m_tags.empty())
        {
            return result;
        }

        for (const auto& pair : m_tags)
        {
            const GameplayTag tag(pair.first);
            if (other.Contains(tag, exactMatch))
            {
                result.Apply(tag, GameplayTagOperation::Set, pair.second);
            }
        }

        return result;
    }

    GameplayTagCollection GameplayTagCollection::GetExclusive(const GameplayTagCollection& other, bool exactMatch) const
    {
        GameplayTagCollection result;

        // Tags in this but not in other - preserve this collection's values
        for (const auto& pair : m_tags)
        {
            const GameplayTag tag(pair.first);
            if (!other.Contains(tag, exactMatch))
            {
                result.Apply(tag, GameplayTagOperation::Set, pair.second);
            }
        }

        // Tags in other but not in this - preserve other collection's values
        for (const auto& pair : other.m_tags)
        {
            const GameplayTag tag(pair.first);
            if (!Contains(tag, exactMatch))
            {
                result.Apply(tag, GameplayTagOperation::Set, pair.second);
            }
        }

        return result;
    }

    AZStd::vector<GameplayTag> GameplayTagCollection::GetDescendants(const GameplayTag& ancestor)
    {
        AZStd::vector<GameplayTag> result;

        if (!ancestor.IsValid())
        {
            return result;
        }

        const AZStd::unordered_set<AZ::u64> descendants = GameplayTagRegistry::GetDescendants(ancestor.GetId());
        for (const AZ::u64 id : descendants)
        {
            result.push_back(GameplayTag(id));
        }

        return result;
    }

    bool GameplayTagCollection::IsEmpty() const
    {
        return m_tags.empty();
    }

    AZ::u32 GameplayTagCollection::Count() const
    {
        return static_cast<AZ::u32>(m_tags.size());
    }

    bool GameplayTagCollection::ContainsAllDescendants(const GameplayTag& ancestor) const
    {
        if (!ancestor.IsValid())
        {
            return false;
        }

        const AZStd::unordered_set<AZ::u64> descendants = GameplayTagRegistry::GetDescendants(ancestor.GetId());

        if (descendants.empty())
        {
            return false;
        }

        for (const AZ::u64 id : descendants)
        {
            if (m_tags.find(id) == m_tags.end())
            {
                return false;
            }
        }

        return true;
    }
}
