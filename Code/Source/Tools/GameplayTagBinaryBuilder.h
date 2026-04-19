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

#include <AssetBuilderSDK/AssetBuilderBusses.h>
#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <FoundationGameplayTags/FoundationGameplayTagsTypeIds.h>

namespace FoundationGameplayTags
{
    /// Converts registered .gptags source files into binary .tagbin products.
    ///
    /// For each .gptags file:
    ///   - If "registered": false (or absent), outputs zero products (file is skipped).
    ///   - If "registered": true, computes the full GameplayTagRegistry descendants map
    ///     from that file's tag list and serialises it as a compact binary .tagbin.
    ///
    /// .tagbin format:
    ///   uint32  magic   = 0x4E474154  ('TAGN')
    ///   uint32  version = 1
    ///   uint64  entryCount
    ///   For each entry:
    ///     uint64  parentHash
    ///     uint32  childCount
    ///     uint64  childHash[childCount]
    ///
    /// At runtime the FoundationGameplayTagsSystemComponent scans for *.tagbin,
    /// deserialises each file, and calls GameplayTagRegistry::MergeDefaultTags().
    class GameplayTagBinaryBuilder
        : public AssetBuilderSDK::AssetBuilderCommandBus::Handler
    {
    public:
        AZ_RTTI(GameplayTagBinaryBuilder, FoundationGameplayTags::GameplayTagBinaryBuilderTypeId);

        static constexpr const char* BuilderName = "Gameplay Tag Binary Builder";
        static constexpr const char* FilePattern = "*.gptags";
        static constexpr const char* JobKey      = "Compile Gameplay Tags";
        static constexpr const char* ProductExt  = ".tagbin";

        GameplayTagBinaryBuilder()  = default;
        ~GameplayTagBinaryBuilder() = default;

        void RegisterBuilder();

        void CreateJobs(
            const AssetBuilderSDK::CreateJobsRequest&  request,
                  AssetBuilderSDK::CreateJobsResponse& response);

        void ProcessJob(
            const AssetBuilderSDK::ProcessJobRequest&  request,
                  AssetBuilderSDK::ProcessJobResponse& response);

        // AssetBuilderSDK::AssetBuilderCommandBus::Handler
        void ShutDown() override;

    private:
        bool m_isShuttingDown = false;
    };

} // namespace FoundationGameplayTags
