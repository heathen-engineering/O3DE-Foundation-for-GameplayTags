
#include "FoundationGameplayTagsSystemComponent.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <FoundationGameplayTags/FoundationGameplayTagsTypeIds.h>

#include "GameplayTagCollection.h"
#include "GameplayTagRegistry.h"

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
    }

    void FoundationGameplayTagsSystemComponent::Deactivate()
    {
    }
} // namespace FoundationGameplayTags
