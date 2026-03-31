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

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>

namespace Heathen
{
    ///<summary>
    /// A standardised registry with understanding of tree structure. This can be used to selectively register tags and enable hierarchy aware comparisons
    ///</summary>
    class GameplayTagRegistry
    {
    public:
        AZ_TYPE_INFO(GameplayTagRegistry, "{A1B2C3D4-E5F6-4789-9ABC-DEF012345678}");

        static void Reflect(AZ::ReflectContext* context);

        static AZ::u64 Hash(const AZStd::string& text);
        static void RegisterFromStringData(const AZStd::string& tag_string);
        static bool IsRegistered(const AZStd::string& tag_string);
        static void UnregisterFromStringData(const AZStd::string& tag_string);
        static void UnregisterAll();
        static bool IsDescendantOf(AZ::u64 tag, AZ::u64 ancestor);
        static bool ContainsAnyDescendantOf(const AZStd::unordered_map<AZ::u64, AZ::u64>& collection, AZ::u64 ancestor);
        static bool ContainsOnlyDescendantOf(const AZStd::unordered_map<AZ::u64, AZ::u64>& collection, AZ::u64 ancestor);
        static bool ContainsAllDescendantOf(const AZStd::unordered_map<AZ::u64, AZ::u64>& collection, AZ::u64 ancestor);
        static AZStd::unordered_set<AZ::u64> GetDescendants(AZ::u64 tag);

        static bool ValidateTag(const AZStd::string& tag_string);
    private:
        static AZStd::unordered_map<AZ::u64, AZStd::unordered_set<AZ::u64>> m_descendants;
        static AZStd::vector<AZStd::string> ParseTagString(const AZStd::string& tag_string);
    };
}