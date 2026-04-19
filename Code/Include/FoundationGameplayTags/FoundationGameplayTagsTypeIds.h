
#pragma once

namespace FoundationGameplayTags
{
    // System Component TypeIds
    inline constexpr const char* FoundationGameplayTagsSystemComponentTypeId = "{7F7F06A1-42B3-43AE-8855-1C272268CD3C}";
    inline constexpr const char* FoundationGameplayTagsEditorSystemComponentTypeId = "{6D69D2CD-7B26-45F9-AB2B-923C440FF659}";

    // Module derived classes TypeIds
    inline constexpr const char* FoundationGameplayTagsModuleInterfaceTypeId = "{B0C6F348-BCFD-4BA7-993D-61C9E40E1C19}";
    inline constexpr const char* FoundationGameplayTagsModuleTypeId = "{BAF39243-3F29-4710-8C32-F24E83512679}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* FoundationGameplayTagsEditorModuleTypeId = FoundationGameplayTagsModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* FoundationGameplayTagsRequestsTypeId = "{9D770D07-AA1E-479C-9BE7-9380C81B7082}";

    // Builder TypeIds
    inline constexpr const char* GameplayTagBinaryBuilderTypeId = "{A3B4C5D6-E7F8-4901-B2C3-D4E5F6A7B8C9}";
} // namespace FoundationGameplayTags
