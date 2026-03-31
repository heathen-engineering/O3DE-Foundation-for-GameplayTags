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
#include <AzCore/Serialization/SerializeContext.h>

namespace Heathen
{
    class GameplayTagCollection;

    class GameplayTag
    {
    public:
        AZ_TYPE_INFO(GameplayTag, "{D2B7C9A1-8E7C-4A6B-9C3D-1F4A8B2E9910}")
        AZ_CLASS_ALLOCATOR(GameplayTag, AZ::SystemAllocator, 0)

        GameplayTag() = default;
        explicit GameplayTag(const AZStd::string& name);
        explicit GameplayTag(AZ::u64 id);

        AZ::u64 GetId() const;
        bool IsValid() const;
        
        bool IsDescendantOf(const GameplayTag& ancestor) const;
        GameplayTagCollection GetDescendants() const;

        static void Reflect(AZ::ReflectContext* context);
        static GameplayTag Make(const AZStd::string& name);
        static GameplayTag Cast(AZ::u64 id);

        bool operator==(const GameplayTag& other) const;
        bool operator!=(const GameplayTag& other) const;

    private:
        AZ::u64 m_id = 0;
    };
}