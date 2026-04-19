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

#include "GameplayTagCondition.h"
#include "GameplayTagCollection.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Script/ScriptContext.h>

namespace Heathen
{
    ////////////////////////////////////////////////////////////////////////////
    // GameplayTagCondition::Evaluate

    bool GameplayTagCondition::Evaluate(const GameplayTagCollection& collection) const
    {
        switch (comparison)
        {
        case GameplayTagComparisonOp::Exists:
            return collection.Contains(tag, exactMatch);

        case GameplayTagComparisonOp::NotExists:
            return !collection.Contains(tag, exactMatch);

        case GameplayTagComparisonOp::Equal:
            return collection.GetValue(tag) == compareValue;

        case GameplayTagComparisonOp::NotEqual:
            return collection.GetValue(tag) != compareValue;

        case GameplayTagComparisonOp::Less:
            return collection.GetValue(tag) < compareValue;

        case GameplayTagComparisonOp::LessEqual:
            return collection.GetValue(tag) <= compareValue;

        case GameplayTagComparisonOp::Greater:
            return collection.GetValue(tag) > compareValue;

        case GameplayTagComparisonOp::GreaterEqual:
            return collection.GetValue(tag) >= compareValue;

        default:
            return false;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // GameplayTagCondition::Reflect

    void GameplayTagCondition::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Enum<GameplayTagComparisonOp>()
                ->Value("Exists",       GameplayTagComparisonOp::Exists)
                ->Value("Not Exists",   GameplayTagComparisonOp::NotExists)
                ->Value("Equal",        GameplayTagComparisonOp::Equal)
                ->Value("Not Equal",    GameplayTagComparisonOp::NotEqual)
                ->Value("Less",         GameplayTagComparisonOp::Less)
                ->Value("Less Equal",   GameplayTagComparisonOp::LessEqual)
                ->Value("Greater",      GameplayTagComparisonOp::Greater)
                ->Value("Greater Equal",GameplayTagComparisonOp::GreaterEqual);

            serialize->Enum<GameplayTagLogicOp>()
                ->Value("And", GameplayTagLogicOp::And)
                ->Value("Or",  GameplayTagLogicOp::Or)
                ->Value("Xor", GameplayTagLogicOp::Xor);

            serialize->Class<GameplayTagCondition>()
                ->Version(1)
                ->Field("Tag",          &GameplayTagCondition::tag)
                ->Field("Comparison",   &GameplayTagCondition::comparison)
                ->Field("CompareValue", &GameplayTagCondition::compareValue)
                ->Field("ExactMatch",   &GameplayTagCondition::exactMatch)
                ->Field("LogicOp",      &GameplayTagCondition::logicOp);

            if (auto* edit = serialize->GetEditContext())
            {
                edit->Enum<GameplayTagComparisonOp>("Gameplay Tag Comparison Op",
                        "How the tag's stored value is compared")
                    ->Value("Exists",        GameplayTagComparisonOp::Exists)
                    ->Value("Not Exists",    GameplayTagComparisonOp::NotExists)
                    ->Value("==",            GameplayTagComparisonOp::Equal)
                    ->Value("!=",            GameplayTagComparisonOp::NotEqual)
                    ->Value("<",             GameplayTagComparisonOp::Less)
                    ->Value("<=",            GameplayTagComparisonOp::LessEqual)
                    ->Value(">",             GameplayTagComparisonOp::Greater)
                    ->Value(">=",            GameplayTagComparisonOp::GreaterEqual);

                edit->Enum<GameplayTagLogicOp>("Gameplay Tag Logic Op",
                        "How this condition chains to the next")
                    ->Value("AND", GameplayTagLogicOp::And)
                    ->Value("OR",  GameplayTagLogicOp::Or)
                    ->Value("XOR", GameplayTagLogicOp::Xor);

                edit->Class<GameplayTagCondition>("Gameplay Tag Condition",
                        "Evaluates one tag in a collection. Chain multiple conditions using logicOp.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GameplayTagCondition::tag,
                        "Tag", "The tag to test")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GameplayTagCondition::comparison,
                        "Comparison", "How to compare the tag value")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GameplayTagCondition::compareValue,
                        "Compare Value",
                        "Value for numeric comparisons (Equal, Not Equal, Less, etc.). Ignored for Exists/Not Exists.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &GameplayTagCondition::exactMatch,
                        "Exact Match",
                        "For Exists/Not Exists: true = exact tag only; false = also match descendants.")
                    ->DataElement(AZ::Edit::UIHandlers::ComboBox, &GameplayTagCondition::logicOp,
                        "Logic Op",
                        "How this condition chains to the NEXT condition (ignored on the last element).");
            }
        }

        if (auto* behavior = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            GameplayTagComparisonOpReflect(*behavior);
            GameplayTagLogicOpReflect(*behavior);

            behavior->Class<GameplayTagCondition>("Gameplay Tag Condition")
                ->Attribute(AZ::Script::Attributes::Category, "Gameplay Tags")
                ->Constructor<>()
                ->Property("tag",          BehaviorValueProperty(&GameplayTagCondition::tag))
                ->Property("comparison",   BehaviorValueProperty(&GameplayTagCondition::comparison))
                ->Property("compareValue", BehaviorValueProperty(&GameplayTagCondition::compareValue))
                ->Property("exactMatch",   BehaviorValueProperty(&GameplayTagCondition::exactMatch))
                ->Property("logicOp",      BehaviorValueProperty(&GameplayTagCondition::logicOp))
                ->Method("Evaluate", &GameplayTagCondition::Evaluate,
                    {{{ "Collection", "The tag collection to test against" }}},
                    "Returns true if this condition is satisfied");
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // EvaluateConditions — C-style precedence: AND > OR > XOR

    bool EvaluateConditions(
        const AZStd::vector<GameplayTagCondition>& conditions,
        const GameplayTagCollection& collection)
    {
        if (conditions.empty())
        {
            return true;
        }

        const size_t n = conditions.size();

        // Step 1: evaluate each condition individually.
        AZStd::vector<bool> vals(n);
        for (size_t i = 0; i < n; ++i)
        {
            vals[i] = conditions[i].Evaluate(collection);
        }

        // Step 2: AND pass — fold consecutive AND pairs into single tokens,
        //         collecting the separator ops (OR or XOR) between AND-groups.
        AZStd::vector<bool>         andTokens;
        AZStd::vector<GameplayTagLogicOp> andOps;
        {
            bool acc = vals[0];
            for (size_t i = 0; i < n - 1; ++i)
            {
                if (conditions[i].logicOp == GameplayTagLogicOp::And)
                {
                    acc = acc && vals[i + 1];
                }
                else
                {
                    andTokens.push_back(acc);
                    andOps.push_back(conditions[i].logicOp); // OR or XOR
                    acc = vals[i + 1];
                }
            }
            andTokens.push_back(acc);
        }

        // Step 3: OR pass — fold consecutive OR pairs,
        //         collecting the remaining XOR separators.
        AZStd::vector<bool> orTokens;
        {
            bool acc = andTokens[0];
            for (size_t i = 0; i < andOps.size(); ++i)
            {
                if (andOps[i] == GameplayTagLogicOp::Or)
                {
                    acc = acc || andTokens[i + 1];
                }
                else // XOR
                {
                    orTokens.push_back(acc);
                    acc = andTokens[i + 1];
                }
            }
            orTokens.push_back(acc);
        }

        // Step 4: XOR pass — fold remaining XOR pairs to produce final result.
        bool result = orTokens[0];
        for (size_t i = 1; i < orTokens.size(); ++i)
        {
            result = result ^ orTokens[i];
        }

        return result;
    }

} // namespace Heathen
