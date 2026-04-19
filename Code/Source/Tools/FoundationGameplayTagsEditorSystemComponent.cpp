
#include <AzCore/Serialization/SerializeContext.h>

#include "FoundationGameplayTagsEditorSystemComponent.h"
#include "GameplayTagBinaryBuilder.h"
#include "GameplayTagsWindow.h"

#include <FoundationGameplayTags/FoundationGameplayTagsTypeIds.h>
#include "GameplayTagRegistry.h"

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/ViewPaneOptions.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>

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
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();
        m_tagBinaryBuilder.RegisterBuilder();

        // Auto-register all registered tags from .gptags files in the project.
        // Only files with "registered": true are included (LoadAllProjectTags filters).
        // This ensures hierarchy queries work for design-time tags in the editor.
        const QStringList tags = GameplayTagsWindow::LoadAllProjectTags();
        for (const QString& tag : tags)
        {
            Heathen::GameplayTagRegistry::RegisterFromStringData(
                AZStd::string(tag.toUtf8().constData()));
        }
    }

    void FoundationGameplayTagsEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
        m_tagBinaryBuilder.BusDisconnect();
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        FoundationGameplayTagsSystemComponent::Deactivate();
    }

    void FoundationGameplayTagsEditorSystemComponent::NotifyRegisterViews()
    {
        AzToolsFramework::ViewPaneOptions options;
        // showInMenu = false: menu registration is handled by the Action Manager hooks
        // (OnMenuBindingHook / OnActionRegistrationHook). Keeping it true would add a
        // duplicate flat entry alongside the Heathen Tools submenu entry.
        options.showInMenu = false;
        options.isDockable = true;

        AzToolsFramework::RegisterViewPane<GameplayTagsWindow>(
            GameplayTagsWindow::PanelName(),
            GameplayTagsWindow::MenuCategory(),
            options);
    }

    // ── Action Manager hooks ────────────────────────────────────────────────────

    void FoundationGameplayTagsEditorSystemComponent::OnMenuRegistrationHook()
    {
        auto* menuManager = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        if (!menuManager)
            return;

        // Register the shared "Heathen Tools" submenu once — any of the three Heathen
        // editor system components may arrive here first, so guard against duplicates.
        if (!menuManager->IsMenuRegistered("heathen.menu.tools"))
        {
            AzToolsFramework::MenuProperties props;
            props.m_name = "Heathen Tools";
            menuManager->RegisterMenu("heathen.menu.tools", props);
        }
    }

    void FoundationGameplayTagsEditorSystemComponent::OnMenuBindingHook()
    {
        auto* menuManager = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        if (!menuManager)
            return;

        // Add the submenu to the editor Tools menu (once — check first).
        // sortIndex 9000 places it after the built-in Tool entries.
        static bool s_submenuAdded = false;
        if (!s_submenuAdded)
        {
            menuManager->AddSubMenuToMenu(
                AZStd::string(EditorIdentifiers::ToolsMenuIdentifier),
                "heathen.menu.tools", 9000);
            s_submenuAdded = true;
        }

        // Add this tool's action into the Heathen Tools submenu.
        menuManager->AddActionToMenu("heathen.menu.tools",
            "heathen.action.gameplaytags", 100);
    }

    void FoundationGameplayTagsEditorSystemComponent::OnActionRegistrationHook()
    {
        auto* actionManager = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        if (!actionManager)
            return;

        AzToolsFramework::ActionProperties props;
        props.m_name        = "Gameplay Tags";
        props.m_description = "Open the Gameplay Tags editor";
        props.m_category    = "Heathen Tools";

        actionManager->RegisterAction(
            AZStd::string(EditorIdentifiers::MainWindowActionContextIdentifier),
            "heathen.action.gameplaytags",
            props,
            []()
            {
                AzToolsFramework::OpenViewPane(GameplayTagsWindow::PanelName());
            });
    }

} // namespace FoundationGameplayTags
