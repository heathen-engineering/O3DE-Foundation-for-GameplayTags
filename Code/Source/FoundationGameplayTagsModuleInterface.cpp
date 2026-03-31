
#include "FoundationGameplayTagsModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <FoundationGameplayTags/FoundationGameplayTagsTypeIds.h>

#include <Clients/FoundationGameplayTagsSystemComponent.h>

namespace FoundationGameplayTags
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(FoundationGameplayTagsModuleInterface,
        "FoundationGameplayTagsModuleInterface", FoundationGameplayTagsModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(FoundationGameplayTagsModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(FoundationGameplayTagsModuleInterface, AZ::SystemAllocator);

    FoundationGameplayTagsModuleInterface::FoundationGameplayTagsModuleInterface()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        // Add ALL components descriptors associated with this gem to m_descriptors.
        // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
        // This happens through the [MyComponent]::Reflect() function.
        m_descriptors.insert(m_descriptors.end(), {
            FoundationGameplayTagsSystemComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList FoundationGameplayTagsModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<FoundationGameplayTagsSystemComponent>(),
        };
    }
} // namespace FoundationGameplayTags
