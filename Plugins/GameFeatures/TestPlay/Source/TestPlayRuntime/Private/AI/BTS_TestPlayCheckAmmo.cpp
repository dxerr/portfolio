// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTS_TestPlayCheckAmmo.h"
#include "AI/TestPlayAIConstants.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Weapons/LyraRangedWeaponInstance.h"
#include "Character/LyraCharacter.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"

UBTS_TestPlayCheckAmmo::UBTS_TestPlayCheckAmmo()
{
    NodeName = "TestPlay Check Ammo";
    
    // 0.5초마다 체크
    Interval = 0.5f;
    RandomDeviation = 0.05f;
    
    bNotifyTick = true;
}

FString UBTS_TestPlayCheckAmmo::GetStaticDescription() const
{
    return TEXT("현재 무기의 탄약을 확인하고 OutOfAmmo 블랙보드 키를 업데이트합니다.");
}

void UBTS_TestPlayCheckAmmo::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    AAIController* AIC = OwnerComp.GetAIOwner();
    
    if (!BlackboardComp || !AIC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTS_TestPlayCheckAmmo] BlackboardComponent 또는 AIController가 없습니다."));
        return;
    }

    APawn* Pawn = AIC->GetPawn();
    if (!Pawn)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTS_TestPlayCheckAmmo] Pawn이 없습니다."));
        return;
    }

    // 1. 현재 탄약 확인
    const int32 CurrentAmmo = GetCurrentAmmo(Pawn);
    
    // 2. 탄약 부족 상태 결정
    // -1은 무기가 없는 경우로, 탄약 부족으로 취급하지 않음
    bool bIsOutOfAmmo = false;
    if (CurrentAmmo >= 0)
    {
        bIsOutOfAmmo = (CurrentAmmo <= 0);
    }

    // 3. 블랙보드 업데이트
    const bool bPreviousValue = BlackboardComp->GetValueAsBool(TestPlayAIKeys::OutOfAmmo);
    if (bPreviousValue != bIsOutOfAmmo)
    {
        BlackboardComp->SetValueAsBool(TestPlayAIKeys::OutOfAmmo, bIsOutOfAmmo);
        UE_LOG(LogTemp, Log, TEXT("[BTS_TestPlayCheckAmmo] OutOfAmmo 상태 변경: %s (탄약: %d)"), 
            bIsOutOfAmmo ? TEXT("true") : TEXT("false"), CurrentAmmo);
    }
}

int32 UBTS_TestPlayCheckAmmo::GetCurrentAmmo(APawn* Pawn) const
{
    if (!Pawn)
    {
        return -1;
    }

    // LyraCharacter로 캐스트
    ALyraCharacter* LyraChar = Cast<ALyraCharacter>(Pawn);
    if (!LyraChar)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[BTS_TestPlayCheckAmmo] LyraCharacter가 아닙니다."));
        return -1;
    }

    // 장비 매니저에서 무기 가져오기
    ULyraEquipmentManagerComponent* EquipmentComp = LyraChar->FindComponentByClass<ULyraEquipmentManagerComponent>();
    if (!EquipmentComp)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[BTS_TestPlayCheckAmmo] EquipmentManagerComponent가 없습니다."));
        return -1;
    }

    // 첫 번째 원거리 무기 인스턴스를 찾음
    ULyraRangedWeaponInstance* RangedWeapon = EquipmentComp->GetFirstInstanceOfType<ULyraRangedWeaponInstance>();
    if (!RangedWeapon)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[BTS_TestPlayCheckAmmo] RangedWeaponInstance가 없습니다."));
        return -1;
    }

    // TODO: Lyra의 실제 탄약 시스템에 맞게 수정 필요
    // 현재 Lyra 5.0에서는 탄약을 LyraRangedWeaponInstance 내부에서 관리하거나
    // ULyraWeaponStateComponent를 통해 관리함
    // 
    // 옵션 1: Attribute를 통한 탄약 확인 (GAS 사용 시)
    // ULyraAbilitySystemComponent* ASC = LyraChar->GetLyraAbilitySystemComponent();
    // if (ASC)
    // {
    //     // 탄약 Attribute 확인 (프로젝트별로 다름)
    //     // return ASC->GetNumericAttribute(ULyraAmmoAttributeSet::GetAmmoAttribute());
    // }
    //
    // 옵션 2: WeaponInstance에서 직접 확인 (커스텀 구현 필요)
    // return RangedWeapon->GetCurrentAmmo();
    
    // 임시 구현: 무기가 있으면 탄약이 있다고 가정
    // 실제 프로젝트에서는 위의 옵션 중 하나를 구현해야 함
    UE_LOG(LogTemp, Verbose, TEXT("[BTS_TestPlayCheckAmmo] 무기 발견: %s"), *RangedWeapon->GetName());
    
    // 테스트용: 항상 탄약이 있다고 반환 (실제 구현 시 변경 필요)
    return 30;
}
