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

#include "GameplayTagBinaryBuilder.h"

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option")
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
AZ_POP_DISABLE_WARNING

#include <xxHash/xxhash.h>

namespace FoundationGameplayTags
{
    static constexpr AZ::u32 kMagic   = 0x4E474154u; // 'TAGN'
    static constexpr AZ::u32 kVersion = 1u;

    // -------------------------------------------------------------------------
    // Compute the descendants map for a flat list of tag strings.
    // Mirrors the logic in GameplayTagRegistry::RegisterFromStringData but into
    // a local map so we never touch the global registry state.
    // -------------------------------------------------------------------------
    static AZStd::unordered_map<AZ::u64, AZStd::unordered_set<AZ::u64>>
        BuildDescendantsMap(const AZStd::vector<AZStd::string>& tags)
    {
        AZStd::unordered_map<AZ::u64, AZStd::unordered_set<AZ::u64>> map;

        for (const AZStd::string& tag : tags)
        {
            // Build the ordered list of prefix nodes.
            // e.g. "Effects.Buff.Strength" → ["Effects", "Effects.Buff", "Effects.Buff.Strength"]
            AZStd::vector<AZStd::string> prefixes;
            size_t dot = 0;
            while ((dot = tag.find('.', dot)) != AZStd::string::npos)
            {
                prefixes.push_back(tag.substr(0, dot));
                ++dot;
            }
            prefixes.push_back(tag); // full tag itself

            for (size_t i = 0; i < prefixes.size(); ++i)
            {
                const AZ::u64 parentHash =
                    XXH3_64bits(prefixes[i].data(), prefixes[i].size());

                // Ensure the node exists even if it has no children (leaf).
                map.emplace(parentHash, AZStd::unordered_set<AZ::u64>{});

                // All deeper prefixes are descendants of this node.
                for (size_t j = i + 1; j < prefixes.size(); ++j)
                {
                    const AZ::u64 childHash =
                        XXH3_64bits(prefixes[j].data(), prefixes[j].size());
                    map[parentHash].insert(childHash);
                }
            }
        }

        return map;
    }

    // -------------------------------------------------------------------------
    // Registration
    // -------------------------------------------------------------------------

    void GameplayTagBinaryBuilder::RegisterBuilder()
    {
        AssetBuilderSDK::AssetBuilderDesc desc;
        desc.m_name       = BuilderName;
        desc.m_busId      = azrtti_typeid<GameplayTagBinaryBuilder>();
        desc.m_version    = 1;
        desc.m_builderType = AssetBuilderSDK::AssetBuilderDesc::AssetBuilderType::External;

        desc.m_patterns.emplace_back(
            AssetBuilderSDK::AssetBuilderPattern(FilePattern,
                AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));

        desc.m_createJobFunction = AZStd::bind(
            &GameplayTagBinaryBuilder::CreateJobs, this,
            AZStd::placeholders::_1, AZStd::placeholders::_2);

        desc.m_processJobFunction = AZStd::bind(
            &GameplayTagBinaryBuilder::ProcessJob, this,
            AZStd::placeholders::_1, AZStd::placeholders::_2);

        BusConnect(desc.m_busId);
        AssetBuilderSDK::AssetBuilderBus::Broadcast(
            &AssetBuilderSDK::AssetBuilderBus::Events::RegisterBuilderInformation, desc);
    }

    void GameplayTagBinaryBuilder::ShutDown()
    {
        m_isShuttingDown = true;
    }

    // -------------------------------------------------------------------------
    // CreateJobs — always create a job so the processor tracks the file.
    // ProcessJob decides whether to emit a product based on the registered flag.
    // -------------------------------------------------------------------------

    void GameplayTagBinaryBuilder::CreateJobs(
        const AssetBuilderSDK::CreateJobsRequest&  request,
              AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        for (const AssetBuilderSDK::PlatformInfo& platform : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor job;
            job.m_jobKey = JobKey;
            job.SetPlatformIdentifier(platform.m_identifier.c_str());
            response.m_createJobOutputs.push_back(AZStd::move(job));
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    // -------------------------------------------------------------------------
    // ProcessJob
    // -------------------------------------------------------------------------

    void GameplayTagBinaryBuilder::ProcessJob(
        const AssetBuilderSDK::ProcessJobRequest&  request,
              AssetBuilderSDK::ProcessJobResponse& response)
    {
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;

        if (m_isShuttingDown)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        // ---- 1. Read source file ----
        AZ::IO::FileIOStream fileStream(
            request.m_fullPath.c_str(),
            AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary);

        if (!fileStream.IsOpen())
        {
            AZ_Error("GameplayTagBinaryBuilder", false,
                "Failed to open source file: %s", request.m_fullPath.c_str());
            return;
        }

        const AZ::IO::SizeType fileSize = fileStream.GetLength();
        AZStd::vector<char> content(fileSize + 1, '\0');
        fileStream.Read(fileSize, content.data());
        fileStream.Close();

        // ---- 2. Parse JSON ----
        rapidjson::Document doc;
        doc.ParseInsitu(content.data());

        if (doc.HasParseError())
        {
            AZ_Error("GameplayTagBinaryBuilder", false,
                "%s: JSON parse error at offset %zu: %s",
                request.m_fullPath.c_str(),
                doc.GetErrorOffset(),
                rapidjson::GetParseError_En(doc.GetParseError()));
            return;
        }

        if (!doc.IsObject())
        {
            AZ_Error("GameplayTagBinaryBuilder", false,
                "%s: root element must be a JSON object.", request.m_fullPath.c_str());
            return;
        }

        // ---- 3. Check registered flag ----
        // If not registered, succeed with zero products. The processor will track
        // the file so it re-runs if registered is later toggled to true.
        const bool isRegistered =
            doc.HasMember("registered") && doc["registered"].IsBool()
            && doc["registered"].GetBool();

        if (!isRegistered)
        {
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            return;
        }

        // ---- 4. Collect tags ----
        AZStd::vector<AZStd::string> tags;
        if (doc.HasMember("tags") && doc["tags"].IsArray())
        {
            for (const auto& tv : doc["tags"].GetArray())
            {
                if (tv.IsString() && tv.GetStringLength() > 0)
                    tags.emplace_back(tv.GetString(), tv.GetStringLength());
            }
        }

        if (tags.empty())
        {
            // Registered but has no tags — emit an empty .tagbin (valid, no-op at load).
            AZ_Warning("GameplayTagBinaryBuilder", false,
                "%s: registered file has no tags; emitting empty .tagbin.",
                request.m_fullPath.c_str());
        }

        // ---- 5. Compute descendants map ----
        const auto descendants = BuildDescendantsMap(tags);

        // ---- 6. Write .tagbin binary ----
        const AZStd::string stemName{
            AZ::IO::Path(request.m_sourceFile.c_str()).Stem().Native() };
        const AZ::IO::Path outPath =
            AZ::IO::Path(request.m_tempDirPath.c_str()) /
            AZ::IO::Path(stemName + ProductExt);

        AZ::IO::FileIOStream out(outPath.c_str(),
            AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeBinary);

        if (!out.IsOpen())
        {
            AZ_Error("GameplayTagBinaryBuilder", false,
                "Failed to open output file for writing: %s", outPath.c_str());
            return;
        }

        const AZ::u32 magic   = kMagic;
        const AZ::u32 version = kVersion;
        const AZ::u64 count   = static_cast<AZ::u64>(descendants.size());

        out.Write(sizeof(magic),   &magic);
        out.Write(sizeof(version), &version);
        out.Write(sizeof(count),   &count);

        for (const auto& [parent, children] : descendants)
        {
            const AZ::u32 childCount = static_cast<AZ::u32>(children.size());
            out.Write(sizeof(parent),     &parent);
            out.Write(sizeof(childCount), &childCount);
            for (AZ::u64 child : children)
                out.Write(sizeof(child), &child);
        }
        out.Close();

        // ---- 7. Register product ----
        AssetBuilderSDK::JobProduct product;
        product.m_productFileName = outPath.c_str();
        product.m_productAssetType = AZ::Data::AssetType::CreateNull(); // raw binary
        product.m_productSubID     = 0;
        response.m_outputProducts.push_back(AZStd::move(product));

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

} // namespace FoundationGameplayTags
