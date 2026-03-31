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

#include "GameplayTag.h"
#include "GameplayTagCollection.h"
#include "GameplayTagRegistry.h"

namespace Heathen
{
    GameplayTag::GameplayTag(AZ::u64 id) : m_id(id) {}

    GameplayTag::GameplayTag(const AZStd::string& name)
    {
        if (GameplayTagRegistry::ValidateTag(name))
            m_id = GameplayTagRegistry::Hash(name);
        else
            m_id = 0;
    }

    AZ::u64 GameplayTag::GetId() const
    {
        return m_id;
    }

    bool GameplayTag::IsValid() const
    {
        return m_id != 0;
    }

    bool GameplayTag::IsDescendantOf(const GameplayTag& ancestor) const
    {
        return GameplayTagRegistry::IsDescendantOf(m_id, ancestor.m_id);
    }

    GameplayTagCollection GameplayTag::GetDescendants() const
    {
        AZStd::unordered_set<AZ::u64> descendants = GameplayTagRegistry::GetDescendants(m_id);
        GameplayTagCollection result;
        for (const AZ::u64 id : descendants)
        {
            result.AddTag(GameplayTag(id));
        }
        return result;
    }

    void GameplayTag::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<GameplayTag>()
                ->Version(1)
                ->Field("Id", &GameplayTag::m_id);
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->Class<GameplayTag>("Gameplay Tag")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay Tags")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Constructor<>()
                ->Constructor<AZ::u64>()
                ->Constructor<const AZStd::string&>()
                ->Method("Make Gameplay Tag", &GameplayTag::Make,
                    {{{ "Tag String", "The dot-delimited tag string e.g. Effects.Buff.Strength" }}})
                ->Method("Cast to Gameplay Tag", &GameplayTag::Cast,
                    {{{ "Id", "A raw u64 hash value representing a tag" }}})
                ->Method("Get Id", &GameplayTag::GetId)
                ->Method("Is Valid", &GameplayTag::IsValid)
                ->Method("Is DescendantOf", &GameplayTag::IsDescendantOf,
                    {{{ "Ancestor", "The ancestor tag to test against" }}})
                ->Method("Get Descendants", &GameplayTag::GetDescendants)
                ->Method("Equal", &GameplayTag::operator==,
                    {{{ "Other", "The tag to compare against" }}})
                ->Method("Not Equal", &GameplayTag::operator!=,
                    {{{ "Other", "The tag to compare against" }}});
        }
    }

    // In GameplayTag.cpp
    GameplayTag GameplayTag::Make(const AZStd::string& name)
    {
        return GameplayTag(name);
    }

    GameplayTag GameplayTag::Cast(AZ::u64 id)
    {
        return GameplayTag(id);
    }

    bool GameplayTag::operator==(const GameplayTag& other) const
    {
        return m_id == other.m_id;
    }

    bool GameplayTag::operator!=(const GameplayTag& other) const
    {
        return m_id != other.m_id;
    }
}
