
#include <FoundationGameplayTags/FoundationGameplayTagsTypeIds.h>
#include <FoundationGameplayTagsModuleInterface.h>
#include "FoundationGameplayTagsSystemComponent.h"

namespace FoundationGameplayTags
{
    class FoundationGameplayTagsModule
        : public FoundationGameplayTagsModuleInterface
    {
    public:
        AZ_RTTI(FoundationGameplayTagsModule, FoundationGameplayTagsModuleTypeId, FoundationGameplayTagsModuleInterface);
        AZ_CLASS_ALLOCATOR(FoundationGameplayTagsModule, AZ::SystemAllocator);
    };
}// namespace FoundationGameplayTags

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), FoundationGameplayTags::FoundationGameplayTagsModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_FoundationGameplayTags, FoundationGameplayTags::FoundationGameplayTagsModule)
#endif
