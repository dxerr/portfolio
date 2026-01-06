// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTS_TestPlayReloadWeapon.h"
#include "AI/TestPlayAIConstants.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Character/LyraCharacter.h"

UBTS_TestPlayReloadWeapon::UBTS_TestPlayReloadWeapon()
{
    NodeName = "TestPlay Reload Weapon";
    
    // 1초마다 체크 (재장전은 빈번하게 체크할 필요 없음)
    Interval = 1.0f;
    RandomDeviation = 0.1f;
    
    bNotifyTick = true;
    
    // 기본 재장전 쿨다운 2초
    ReloadCooldown = 2.0f;
}

FString UBTS_TestPlayReloadWeapon::GetStaticDescription() const
{
    return FString::Printf(TEXT("탄약이 부족하면 재장전을 시도합니다.\n쿨다운: %.1f초"), ReloadCooldown);
}

void UBTS_TestPlayReloadWeapon::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTS_TestPlayReloadWeapon] BlackboardComponent가 없습니다."));
        return;
    }

    // 1. 탄약 부족 상태 확인
    const bool bOutOfAmmo = BlackboardComp->GetValueAsBool(TestPlayAIKeys::OutOfAmmo);
    
    if (!bOutOfAmmo)
    {
        // 탄약이 있으면 아무것도 하지 않음
        return;
    }

    // 2. 재장전 쿨다운 체크
    UWorld* World = OwnerComp.GetWorld();
    if (!World)
    {
        return;
    }
    
    const float CurrentTime = World->GetTimeSeconds();
    if (CurrentTime - LastReloadAttemptTime < ReloadCooldown)
    {
        // 아직 쿨다운 중
        return;
    }

    // 3. AIController와 Pawn 확인
    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC || !AIC->GetPawn())
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTS_TestPlayReloadWeapon] AIController 또는 Pawn이 없습니다."));
        return;
    }

    // 4. LyraAbilitySystemComponent 가져오기
    ALyraCharacter* LyraChar = Cast<ALyraCharacter>(AIC->GetPawn());
    if (!LyraChar)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTS_TestPlayReloadWeapon] LyraCharacter가 아닙니다."));
        return;
    }

    ULyraAbilitySystemComponent* LyraASC = LyraChar->GetLyraAbilitySystemComponent();
    if (!LyraASC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTS_TestPlayReloadWeapon] LyraAbilitySystemComponent가 없습니다."));
        return;
    }

    // 5. 재장전 어빌리티 직접 활성화 (TryActivateAbilitiesByTag 사용)
    FGameplayTag ReloadTag = FGameplayTag::RequestGameplayTag(TestPlayAITags::InputTag_Weapon_Reload);
    
    if (!ReloadTag.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTS_TestPlayReloadWeapon] InputTag.Weapon.Reload 태그가 유효하지 않습니다."));
        return;
    }

    FGameplayTagContainer ReloadTags;
    ReloadTags.AddTag(ReloadTag);
    
    // TryActivateAbilitiesByTag는 안전하게 어빌리티 활성화 가능
    const bool bSuccess = LyraASC->TryActivateAbilitiesByTag(ReloadTags);
    LastReloadAttemptTime = CurrentTime;
    
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("[BTS_TestPlayReloadWeapon] 재장전 어빌리티 활성화 성공"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[BTS_TestPlayReloadWeapon] 재장전 어빌리티 활성화 실패"));
    }
}
