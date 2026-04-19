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

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/vector.h>
#include "GameplayTag.h"

namespace Heathen
{
    class GameplayTagCollection;

    // ---------------------------------------------------------------------------
    // Enums
    // ---------------------------------------------------------------------------

    /// How the tag's stored value is compared in a GameplayTagCondition.
    /// Exists / NotExists treat the tag as a boolean flag (value != 0 / value == 0).
    /// The numeric operators compare the stored u64 value against GameplayTagCondition::compareValue.
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(GameplayTagComparisonOp, AZ::u8,
        Exists,       ///< tag is present (value != 0); exactMatch applies
        NotExists,    ///< tag is absent  (value == 0); exactMatch applies
        Equal,        ///< stored value == compareValue (exact tag only)
        NotEqual,     ///< stored value != compareValue (exact tag only)
        Less,         ///< stored value <  compareValue (exact tag only)
        LessEqual,    ///< stored value <= compareValue (exact tag only)
        Greater,      ///< stored value >  compareValue (exact tag only)
        GreaterEqual  ///< stored value >= compareValue (exact tag only)
    );

    AZ_ENUM_DEFINE_REFLECT_UTILITIES(GameplayTagComparisonOp)
    AZ_TYPE_INFO_SPECIALIZE(Heathen::GameplayTagComparisonOp, "{E5F6A7B8-C9D0-4E1F-A2B3-C4D5E6F7A8B9}");

    /// How this condition chains to the NEXT condition in a vector.
    /// The logicOp of the LAST condition in the vector is ignored.
    /// Evaluation uses C-style precedence: AND binds tightest, then OR, then XOR.
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(GameplayTagLogicOp, AZ::u8,
        And,  ///< this result AND next
        Or,   ///< this result OR  next
        Xor   ///< this result XOR next
    );

    AZ_ENUM_DEFINE_REFLECT_UTILITIES(GameplayTagLogicOp)
    AZ_TYPE_INFO_SPECIALIZE(Heathen::GameplayTagLogicOp, "{F6A7B8C9-D0E1-4F2A-B3C4-D5E6F7A8B9C0}");

    // ---------------------------------------------------------------------------
    // GameplayTagCondition
    // ---------------------------------------------------------------------------

    ///<summary>
    /// A single predicate that tests one tag in a GameplayTagCollection.
    ///
    /// Usage in a conditions vector:
    ///   Each element's logicOp describes how it connects to the NEXT element.
    ///   The final element's logicOp is ignored.
    ///   Call EvaluateConditions() to reduce the whole vector to a single bool using
    ///   C-style precedence (AND > OR > XOR).
    ///
    /// Numeric comparisons (Equal, NotEqual, Less, …) always use the exact tag value;
    /// exactMatch is ignored for those operators.
    ///</summary>
    struct GameplayTagCondition
    {
        AZ_TYPE_INFO(GameplayTagCondition, "{A7B8C9D0-E1F2-4A3B-C4D5-E6F7A8B9C0D1}")
        AZ_CLASS_ALLOCATOR(GameplayTagCondition, AZ::SystemAllocator, 0)

        GameplayTag            tag;
        GameplayTagComparisonOp comparison  = GameplayTagComparisonOp::Exists;
        AZ::u64                compareValue = 1;
        bool                   exactMatch   = true;
        /// How this condition chains to the next one. Ignored on the last element.
        GameplayTagLogicOp     logicOp      = GameplayTagLogicOp::And;

        /// Evaluates this single condition against the given collection.
        bool Evaluate(const GameplayTagCollection& collection) const;

        static void Reflect(AZ::ReflectContext* context);
    };

    // ---------------------------------------------------------------------------
    // EvaluateConditions — free function
    // ---------------------------------------------------------------------------

    ///<summary>
    /// Reduces a vector of GameplayTagConditions to a single bool using
    /// C-style operator precedence: AND > OR > XOR.
    /// Returns true when the vector is empty (unconditional).
    ///</summary>
    bool EvaluateConditions(
        const AZStd::vector<GameplayTagCondition>& conditions,
        const GameplayTagCollection& collection);

} // namespace Heathen
