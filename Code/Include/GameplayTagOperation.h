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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/vector.h>
#include "GameplayTag.h"
#include "GameplayTagCollection.h"   // GameplayTagArithmetic
#include "GameplayTagCondition.h"    // GameplayTagCondition, EvaluateConditions

namespace Heathen
{
    ///<summary>
    /// Describes an operation to apply to a GameplayTagCollection,
    /// optionally guarded by a list of GameplayTagConditions.
    ///
    /// The operation is only applied when all conditions evaluate to true
    /// (using C-style AND/OR/XOR precedence).  An empty conditions vector
    /// is unconditional — the operation always applies.
    ///
    /// Value convention (mirrors GameplayTagCollection):
    ///   Set(tag, 0)  == Remove(tag)
    ///   Set(tag, 1)  == presence flag (same as AddTag)
    ///</summary>
    struct GameplayTagOperation
    {
        AZ_TYPE_INFO(GameplayTagOperation, "{D4E5F6A7-B8C9-0123-DEF0-234567890123}")
        AZ_CLASS_ALLOCATOR(GameplayTagOperation, AZ::SystemAllocator, 0)

        GameplayTag                        tag;
        GameplayTagArithmetic              arithmetic  = GameplayTagArithmetic::Set;
        AZ::u64                            value       = 1;
        AZStd::vector<GameplayTagCondition> conditions; ///< empty = always applies

        /// Returns true if the conditions vector is satisfied (or empty).
        bool ShouldApply(const GameplayTagCollection& collection) const;

        /// Applies the operation to the collection if ShouldApply() returns true.
        /// Returns true if the operation was applied.
        bool Apply(GameplayTagCollection& collection) const;

        static void Reflect(AZ::ReflectContext* context);
    };

} // namespace Heathen
