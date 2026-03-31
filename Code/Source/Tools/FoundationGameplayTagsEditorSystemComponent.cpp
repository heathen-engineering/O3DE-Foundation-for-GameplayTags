
#include <AzCore/Serialization/SerializeContext.h>

#include <AzToolsFramework/API/ViewPaneOptions.h>

#include "FoundationGameplayTagsWidget.h"
#include "FoundationGameplayTagsEditorSystemComponent.h"

#include <FoundationGameplayTags/FoundationGameplayTagsTypeIds.h>

namespace FoundationGameplayTags
{
    AZ_COMPONENT_IMPL(FoundationGameplayTagsEditorSystemComponent, "FoundationGameplayTagsEditorSystemComponent",
        FoundationGameplayTagsEditorSystemComponentTypeId, BaseSystemComponent);

    void FoundationGameplayTagsEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<FoundationGameplayTagsEditorSystemComponent, FoundationGameplayTagsSystemComponent>()
                ->Version(0);
        }
    }

    FoundationGameplayTagsEditorSystemComponent::FoundationGameplayTagsEditorSystemComponent() = default;

    FoundationGameplayTagsEditorSystemComponent::~FoundationGameplayTagsEditorSystemComponent() = default;

    void FoundationGameplayTagsEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("FoundationGameplayTagsEditorService"));
    }

    void FoundationGameplayTagsEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("FoundationGameplayTagsEditorService"));
    }

    void FoundationGameplayTagsEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void FoundationGameplayTagsEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void FoundationGameplayTagsEditorSystemComponent::Activate()
    {
        FoundationGameplayTagsSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void FoundationGameplayTagsEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        FoundationGameplayTagsSystemComponent::Deactivate();
    }

    void FoundationGameplayTagsEditorSystemComponent::NotifyRegisterViews()
    {
        AzToolsFramework::ViewPaneOptions options;
        options.paneRect = QRect(100, 100, 500, 400);
        options.showOnToolsToolbar = true;
        options.toolbarIcon = ":/FoundationGameplayTags/toolbar_icon.svg";

        // Register our custom widget as a dockable tool with the Editor under an Examples sub-menu
        AzToolsFramework::RegisterViewPane<FoundationGameplayTagsWidget>("FoundationGameplayTags", "Examples", options);
    }

} // namespace FoundationGameplayTags
