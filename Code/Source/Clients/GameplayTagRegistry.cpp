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

#include "GameplayTagRegistry.h"
#include <xxHash/xxhash.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace Heathen
{
    // Static lookup table - built once, valid chars return true
    // ASCII only, indices 0-127
    static const bool s_validTagChars[128] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 0-15
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 16-31
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0, // 32-47  (46='.')
        1,1,1,1,1,1,1,1,1,1,              // 48-57  '0'-'9'
        0,0,0,0,0,0,0,                    // 58-64
        1,1,1,1,1,1,1,1,1,1,1,1,1,        // 65-77  'A'-'M'
        1,1,1,1,1,1,1,1,1,1,1,1,1,        // 78-90  'N'-'Z'
        0,0,0,0,                          // 91-94
        1,                                // 95     '_'
        0,                                // 96
        1,1,1,1,1,1,1,1,1,1,1,1,1,        // 97-109  'a'-'m'
        1,1,1,1,1,1,1,1,1,1,1,1,1,        // 110-122 'n'-'z'
        0,0,0,0,0                         // 123-127
    };

    AZStd::unordered_map<AZ::u64, AZStd::unordered_set<AZ::u64>> GameplayTagRegistry::m_descendants;
    AZStd::unordered_map<AZ::u64, AZStd::unordered_set<AZ::u64>> GameplayTagRegistry::m_defaultDescendants;

    void GameplayTagRegistry::Reflect(AZ::ReflectContext* context)
    {
        if (auto behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->Class<GameplayTagRegistry>("Gameplay Tag Registry")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay Tags")
                ->Method("Gameplay Tag Registry Hash", &GameplayTagRegistry::Hash,
                    {{{ "Tag String", "The tag string to hash" }}})
                ->Method("Gameplay Tag Register From String Data", &GameplayTagRegistry::RegisterFromStringData,
                    {{{ "Tag String", "Line delimited list of tags to register" }}})
                ->Method("Gameplay Tag Is Registered", &GameplayTagRegistry::IsRegistered,
                    {{{ "Tag String", "The tag string to check" }}})
                ->Method("Gameplay Tag Unregister From String Data", &GameplayTagRegistry::UnregisterFromStringData,
                    {{{ "Tag String", "Line delimited list of tags to unregister" }}})
                ->Method("Gameplay Tag Unregister All", &GameplayTagRegistry::UnregisterAll);
        }
    }

    AZ::u64 GameplayTagRegistry::Hash(const AZStd::string& text)
    {
        return XXH3_64bits(text.data(), text.size());
    }

    void GameplayTagRegistry::RegisterFromStringData(const AZStd::string& tag_string)
    {
        AZStd::vector<AZStd::string> tags = ParseTagString(tag_string);

        if (tags.empty())
        {
            return;
        }

        for (const AZStd::string& tag : tags)
        {
            // Decompose tag into all prefix nodes
            // e.g. Effects.Buff.Strength -> [Effects, Effects.Buff, Effects.Buff.Strength]
            AZStd::vector<AZStd::string> prefixStrings;
            AZStd::string build;
            size_t start = 0;

            for (size_t i = 0; i <= tag.size(); ++i)
            {
                if (i == tag.size() || tag[i] == '.')
                {
                    if (i > start)
                    {
                        if (!build.empty())
                        {
                            build += '.';
                        }
                        build += tag.substr(start, i - start);
                        prefixStrings.push_back(build);
                    }
                    start = i + 1;
                }
            }

            // For each prefix node, insert all tags that fall below it
            // A tag is a descendant of a prefix if the tag starts with prefix + "."
            // The prefix itself is not a descendant of itself
            for (size_t i = 0; i < prefixStrings.size(); ++i)
            {
                const AZ::u64 prefixHash = Hash(prefixStrings[i]);

                // Ensure the node exists in the map even if it gets no descendants (leaf)
                m_descendants.emplace(prefixHash, AZStd::unordered_set<AZ::u64>{});

                // All prefix nodes below this one are descendants
                for (size_t j = i + 1; j < prefixStrings.size(); ++j)
                {
                    m_descendants[prefixHash].insert(Hash(prefixStrings[j]));
                }
            }
        }
    }

    bool GameplayTagRegistry::IsRegistered(const AZStd::string& tag_string)
    {
        if (!ValidateTag(tag_string))
        {
            return false;
        }
        return m_descendants.find(Hash(tag_string)) != m_descendants.end();
    }

    void GameplayTagRegistry::UnregisterFromStringData(const AZStd::string& tag_string)
    {
        AZStd::vector<AZStd::string> tags = ParseTagString(tag_string);

        if (tags.empty())
        {
            return;
        }

        // Build the full removal set
        // For each tag in input, collect it and all its descendants
        AZStd::unordered_set<AZ::u64> removalSet;

        for (const AZStd::string& tag : tags)
        {
            const AZ::u64 id = Hash(tag);

            // Add the tag itself
            removalSet.insert(id);

            // Add all its descendants if it is a category
            auto it = m_descendants.find(id);
            if (it != m_descendants.end())
            {
                for (const AZ::u64 descendant : it->second)
                {
                    removalSet.insert(descendant);
                }
            }
        }

        // Remove all nodes in the removal set from m_descendants
        for (const AZ::u64 id : removalSet)
        {
            m_descendants.erase(id);
        }

        // Scrub removal set from all remaining ancestor nodes
        // Collect nodes that become empty after scrubbing for a second pass removal
        AZStd::vector<AZ::u64> toRemove;

        for (auto& pair : m_descendants)
        {
            for (const AZ::u64 removed : removalSet)
            {
                pair.second.erase(removed);
            }

            if (pair.second.empty())
            {
                toRemove.push_back(pair.first);
            }
        }

        // Remove any ancestor nodes that became empty
        for (const AZ::u64 id : toRemove)
        {
            m_descendants.erase(id);
        }
    }

    void GameplayTagRegistry::UnregisterAll()
    {
        // Restore to the project defaults loaded from .tagbin products.
        // User-registered tags are discarded; default tags survive.
        m_descendants = m_defaultDescendants;
    }

    void GameplayTagRegistry::MergeDefaultTags(
        const AZStd::unordered_map<AZ::u64, AZStd::unordered_set<AZ::u64>>& defaults)
    {
        for (const auto& [parent, children] : defaults)
        {
            auto& dest = m_defaultDescendants[parent];
            for (AZ::u64 child : children)
                dest.insert(child);
        }
        // Also merge into the live working map so they are immediately visible.
        for (const auto& [parent, children] : defaults)
        {
            auto& dest = m_descendants[parent];
            for (AZ::u64 child : children)
                dest.insert(child);
        }
    }

    bool GameplayTagRegistry::IsDescendantOf(AZ::u64 tag, AZ::u64 ancestor)
    {
        auto it = m_descendants.find(ancestor);
        if (it == m_descendants.end())
        {
            return false;
        }

        const auto& children = it->second;

        return children.find(tag) != children.end();
    }

    bool GameplayTagRegistry::ContainsAnyDescendantOf(const AZStd::unordered_map<AZ::u64, AZ::u64>& collection, AZ::u64 ancestor)
    {
        auto it = m_descendants.find(ancestor);
        if (it == m_descendants.end())
        {
            return false;
        }
        const auto& descendants = it->second;
        if (collection.size() < descendants.size())
        {
            for (const auto& pair : collection)
            {
                if (descendants.find(pair.first) != descendants.end())
                {
                    return true;
                }
            }
        }
        else
        {
            for (const AZ::u64 descendant : descendants)
            {
                if (collection.find(descendant) != collection.end())
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool GameplayTagRegistry::ContainsOnlyDescendantOf(const AZStd::unordered_map<AZ::u64, AZ::u64>& collection, AZ::u64 ancestor)
    {
        auto it = m_descendants.find(ancestor);
        if (it == m_descendants.end())
        {
            return false;
        }

        const auto& descendants = it->second;

        if (collection.size() > descendants.size())
        {
            return false;
        }

        for (const auto& pair : collection)
        {
            if (descendants.find(pair.first) == descendants.end())
            {
                return false;
            }
        }
        return true;
    }

    bool GameplayTagRegistry::ContainsAllDescendantOf(const AZStd::unordered_map<AZ::u64, AZ::u64>& collection, AZ::u64 ancestor)
    {
        auto it = m_descendants.find(ancestor);
        if (it == m_descendants.end())
        {
            return false;
        }

        const auto& descendants = it->second;

        if (collection.size() < descendants.size())
        {
            return false;
        }

        // every descendant must exist in collection
        for (const AZ::u64 descendant : descendants)
        {
            if (collection.find(descendant) == collection.end())
            {
                return false;
            }
        }
        return true;
    }

    AZStd::unordered_set<AZ::u64> GameplayTagRegistry::GetDescendants(AZ::u64 tag)
    {
        auto it = m_descendants.find(tag);
        if (it == m_descendants.end())
        {
            return {};
        }

        return it->second;
    }

    bool GameplayTagRegistry::ValidateTag(const AZStd::string& tag_string)
    {
        if (tag_string.empty())
        {
            return false;
        }
        if (tag_string.front() == '.' || tag_string.back() == '.')
        {
            return false;
        }
        bool last_was_dot = false;
        for (char c : tag_string)
        {
            // Reject non-ASCII immediately
            if (static_cast<unsigned char>(c) >= 128)
            {
                return false;
            }
            if (!s_validTagChars[static_cast<int>(c)])
            {
                return false;
            }
            const bool is_dot = (c == '.');
            if (is_dot && last_was_dot)
            {
                return false;
            }
            last_was_dot = is_dot;
        }
        return true;
    }

    AZStd::vector<AZStd::string> GameplayTagRegistry::ParseTagString(const AZStd::string& tag_string)
    {
        AZStd::vector<AZStd::string> result;
        AZStd::string current;

        for (char c : tag_string)
        {
            if (c == '\n' || c == '\r')
            {
                if (!current.empty())
                {
                    if (ValidateTag(current))
                    {
                        result.push_back(current);
                    }
                    else
                    {
                        AZ_Warning("GameplayTagRegistry", false,
                            "Invalid tag rejected: %s", current.c_str());
                    }
                    current.clear();
                }
            }
            else
            {
                current.push_back(c);
            }
        }

        // Handle final line with no trailing newline
        if (!current.empty())
        {
            if (ValidateTag(current))
            {
                result.push_back(current);
            }
            else
            {
                AZ_Warning("GameplayTagRegistry", false,
                    "Invalid tag rejected: %s", current.c_str());
            }
        }

        return result;
    }
}
