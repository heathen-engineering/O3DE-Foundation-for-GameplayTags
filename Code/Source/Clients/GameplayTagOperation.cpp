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

#include "GameplayTagOperation.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Script/ScriptContext.h>

namespace Heathen
{
    bool GameplayTagOperation::ShouldApply(const GameplayTagCollection& collection) const
    {
        return EvaluateConditions(conditions, collection);
    }

    bool GameplayTagOperation::Apply(GameplayTagCollection& collection) const
    {
        if (!ShouldApply(collection))
        {
            return false;
        }
        collection.Apply(tag, arithmetic, value);
        return true;
    }

    void GameplayTagOperation::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<GameplayTagOperation>()
                ->Version(1)
                ->Field("Tag",        &GameplayTagOperation::tag)
                ->Field("Arithmetic", &GameplayTagOperation::arithmetic)
                ->Field("Value",      &GameplayTagOperation::value)
                ->Field("Conditions", &GameplayTagOperation::conditions);

            if (auto* edit = serialize->GetEditContext())
            {
                edit->Class<GameplayTagOperation>("Gameplay Tag Operation",
                        "Applies an arithmetic operation to a tag in a collection, "
                        "optionally guarded by conditions.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GameplayTagOperation::tag,
                        "Tag", "The tag to operate on")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GameplayTagOperation::arithmetic,
                        "Arithmetic", "Set / Add / Sub / Mul / Div / Min / Max")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GameplayTagOperation::value,
                        "Value", "Operand value. Set(tag, 0) removes the tag.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GameplayTagOperation::conditions,
                        "Conditions",
                        "Optional guard conditions. All must be satisfied for the operation to apply. "
                        "Leave empty to apply unconditionally.");
            }
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behavior->Class<GameplayTagOperation>("Gameplay Tag Operation")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay Tags")
                ->Constructor<>()
                ->Property("tag",        BehaviorValueProperty(&GameplayTagOperation::tag))
                ->Property("arithmetic", BehaviorValueProperty(&GameplayTagOperation::arithmetic))
                ->Property("value",      BehaviorValueProperty(&GameplayTagOperation::value))
                ->Property("conditions", BehaviorValueProperty(&GameplayTagOperation::conditions))
                ->Method("Should Apply", &GameplayTagOperation::ShouldApply,
                    {{{ "Collection", "The collection to test conditions against" }}},
                    "Returns true if all conditions are satisfied (or there are none)")
                ->Method("Apply", &GameplayTagOperation::Apply,
                    {{{ "Collection", "The collection to modify" }}},
                    "Applies the operation if conditions pass. Returns true if applied.");
        }
    }

} // namespace Heathen
