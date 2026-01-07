// Copyright Epic Games, Inc. All Rights Reserved.

#include "TestPlayAIBlueprintLibrary.h"
#include "Character/LyraCharacter.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Weapons/LyraRangedWeaponInstance.h"
#include "GameFramework/Pawn.h"

bool UTestPlayAIBlueprintLibrary::IsTargetInWeaponRange(APawn* ShooterPawn, AActor* TargetActor, float RangeTolerance)
{
    // 1. 필수 유효성 검사
    if (!ShooterPawn || !TargetActor)
    {
        return false;
    }

    // 2. Lyra 캐릭터 및 장비 컴포넌트 가져오기
    ALyraCharacter* LyraChar = Cast<ALyraCharacter>(ShooterPawn);
    if (!LyraChar)
    {
        return false;
    }

    ULyraEquipmentManagerComponent* EquipmentComp = LyraChar->FindComponentByClass<ULyraEquipmentManagerComponent>();
    if (!EquipmentComp)
    {
        return false;
    }

    // 3. 현재 장착된 원거리 무기(Ranged Weapon) 찾기
    // LyraRangedWeaponInstance는 사거리 정보를 가지고 있습니다.
    ULyraRangedWeaponInstance* WeaponInstance = EquipmentComp->GetFirstInstanceOfType<ULyraRangedWeaponInstance>();
    
    // 만약 무기가 없다면? (무기가 없으면 사거리가 0이므로 못 쏘는 게 맞음 -> false)
    if (!WeaponInstance)
    {
        return false;
    }

    // 4. 거리 계산 및 판별
    float MaxRange = WeaponInstance->GetMaxDamageRange();
    float DistanceSq = ShooterPawn->GetSquaredDistanceTo(TargetActor);
    
    // 연산 최적화를 위해 제곱 거리(Squared Distance) 사용
    // (Range + Tolerance)^2
    float CheckDistance = MaxRange + RangeTolerance;
    
    return DistanceSq <= (CheckDistance * CheckDistance);
}

float UTestPlayAIBlueprintLibrary::GetCurrentWeaponMaxRange(APawn* ShooterPawn)
{
    if (ALyraCharacter* LyraChar = Cast<ALyraCharacter>(ShooterPawn))
    {
        if (ULyraEquipmentManagerComponent* EquipmentComp = LyraChar->FindComponentByClass<ULyraEquipmentManagerComponent>())
        {
            if (ULyraRangedWeaponInstance* WeaponInstance = EquipmentComp->GetFirstInstanceOfType<ULyraRangedWeaponInstance>())
            {
                return WeaponInstance->GetMaxDamageRange();
            }
        }
    }
    return 0.0f;
}