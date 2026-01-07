// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * AI Behavior Tree에서 사용되는 블랙보드 키 상수 정의
 */
namespace TestPlayAIKeys
{
    // 현재 타겟 적 (Object/Actor)
    static const FName TargetEnemy = FName("TargetEnemy");
    
    // 탄약 부족 상태 (Bool)
    static const FName OutOfAmmo = FName("OutOfAmmo");
    
    // AI 자신의 액터 (Object/Actor)
    static const FName SelfActor = FName("SelfActor");
    
    // 이동 목표 위치 (Vector)
    static const FName MoveGoal = FName("MoveGoal");

    // 목표 골 반경
    static const FName GoalRadius = FName("GoalRadius");
}

/**
 * AI에서 사용되는 Gameplay 태그 상수 정의
 * TryActivateAbilitiesByTag에서 사용됨 - 어빌리티의 AbilityTags와 일치해야 함
 */
namespace TestPlayAITags
{
    // 발사 어빌리티 태그 (GA_Weapon_Fire의 AbilityTags)
    static const FName AbilityTag_Weapon_Fire = FName("Ability.Type.Action.WeaponFire");
    
    // 근접 무기 공격 쿨다운 태그 (BT에서 공격 딜레이 체크용)
    static const FName CooldownTag_Weapon_MeleeFire = FName("Cooldown.Weapon.MeleeFire");
    
    // 재장전 입력 태그 (TODO: 실제 재장전 어빌리티 태그로 변경 필요)
    static const FName InputTag_Weapon_Reload = FName("InputTag.Weapon.Reload");
}
