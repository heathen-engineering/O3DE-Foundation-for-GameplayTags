
#pragma once

#include <Clients/FoundationGameplayTagsSystemComponent.h>
#include "GameplayTagBinaryBuilder.h"

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace FoundationGameplayTags
{
    /// System component for FoundationGameplayTags editor
    class FoundationGameplayTagsEditorSystemComponent
        : public FoundationGameplayTagsSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
        , public AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
        using BaseSystemComponent = FoundationGameplayTagsSystemComponent;
    public:
        AZ_COMPONENT_DECL(FoundationGameplayTagsEditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        FoundationGameplayTagsEditorSystemComponent();
        ~FoundationGameplayTagsEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // AzToolsFramework::EditorEventsBus overrides ...
        void NotifyRegisterViews() override;

        // AzToolsFramework::ActionManagerRegistrationNotificationBus
        void OnMenuRegistrationHook()  override;
        void OnMenuBindingHook()       override;
        void OnActionRegistrationHook() override;

        GameplayTagBinaryBuilder m_tagBinaryBuilder;
    };
} // namespace FoundationGameplayTags
