
#include "FoundationGameplayTagsSystemComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Utils/Utils.h>
#include <FoundationGameplayTags/FoundationGameplayTagsTypeIds.h>

#include "GameplayTagCollection.h"
#include "GameplayTagRegistry.h"
#include "GameplayTagCondition.h"
#include "GameplayTagOperation.h"

namespace FoundationGameplayTags
{
    AZ_COMPONENT_IMPL(FoundationGameplayTagsSystemComponent, "FoundationGameplayTagsSystemComponent",
        FoundationGameplayTagsSystemComponentTypeId);

    void FoundationGameplayTagsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<FoundationGameplayTagsSystemComponent, AZ::Component>()
                ->Version(0);
        }

        Heathen::GameplayTag::Reflect(context);
        Heathen::GameplayTagCollection::Reflect(context);
        Heathen::GameplayTagRegistry::Reflect(context);
        Heathen::GameplayTagCondition::Reflect(context);
        Heathen::GameplayTagOperation::Reflect(context);
    }

    void FoundationGameplayTagsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("FoundationGameplayTagsService"));
    }

    void FoundationGameplayTagsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("FoundationGameplayTagsService"));
    }

    void FoundationGameplayTagsSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void FoundationGameplayTagsSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void FoundationGameplayTagsSystemComponent::Init()
    {
    }

    void FoundationGameplayTagsSystemComponent::Activate()
    {
        LoadDefaultTagsFromDisk();
    }

    void FoundationGameplayTagsSystemComponent::Deactivate()
    {
    }

    // -------------------------------------------------------------------------
    // .tagbin loading
    // -------------------------------------------------------------------------

    static constexpr AZ::u32 kTagBinMagic   = 0x4E474154u; // 'TAGN'
    static constexpr AZ::u32 kTagBinVersion = 1u;

    void FoundationGameplayTagsSystemComponent::LoadDefaultTagsFromDisk()
    {
        AZ::IO::FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
        if (!fileIO)
            return;

        // Resolve the project source root — works in both editor and game.
        AZ::IO::FixedMaxPath projectRoot = AZ::Utils::GetProjectPath();
        if (projectRoot.empty())
            return;

        AZStd::vector<AZStd::string> tagbinPaths;
        fileIO->FindFiles(projectRoot.c_str(), "*.tagbin",
            [&tagbinPaths](const char* filePath) -> bool
            {
                tagbinPaths.emplace_back(filePath);
                return true; // continue
            });

        for (const AZStd::string& path : tagbinPaths)
        {
            AZ::IO::FileIOStream stream(path.c_str(),
                AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary);
            if (!stream.IsOpen())
                continue;

            // Validate header
            AZ::u32 magic = 0, version = 0;
            stream.Read(sizeof(magic),   &magic);
            stream.Read(sizeof(version), &version);
            if (magic != kTagBinMagic || version != kTagBinVersion)
                continue;

            AZ::u64 entryCount = 0;
            stream.Read(sizeof(entryCount), &entryCount);

            AZStd::unordered_map<AZ::u64, AZStd::unordered_set<AZ::u64>> fileMap;
            fileMap.reserve(static_cast<size_t>(entryCount));

            for (AZ::u64 e = 0; e < entryCount; ++e)
            {
                AZ::u64 parentHash  = 0;
                AZ::u32 childCount  = 0;
                stream.Read(sizeof(parentHash), &parentHash);
                stream.Read(sizeof(childCount), &childCount);

                AZStd::unordered_set<AZ::u64> children;
                children.reserve(childCount);
                for (AZ::u32 c = 0; c < childCount; ++c)
                {
                    AZ::u64 childHash = 0;
                    stream.Read(sizeof(childHash), &childHash);
                    children.insert(childHash);
                }
                fileMap.emplace(parentHash, AZStd::move(children));
            }

            Heathen::GameplayTagRegistry::MergeDefaultTags(fileMap);
        }
    }

} // namespace FoundationGameplayTags
