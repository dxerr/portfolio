// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTS_TestPlayShoot.h"
#include "AI/TestPlayAIConstants.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystem/LyraAbilitySystemComponent.h"
#include "Character/LyraCharacter.h"
#include "Equipment/LyraEquipmentManagerComponent.h"
#include "Weapons/LyraRangedWeaponInstance.h"

// 디버그 로그 카테고리
DEFINE_LOG_CATEGORY_STATIC(LogTestPlayShoot, Log, All);

UBTS_TestPlayShoot::UBTS_TestPlayShoot()
{
    NodeName = "TestPlay Shoot";
    
    // 0.2초마다 체크 (빈번한 체크로 인한 문제 방지)
    Interval = 0.2f;
    RandomDeviation = 0.02f;
    
    // 틱 알림 설정
    bNotifyTick = true;
    bNotifyCeaseRelevant = true;
}

FString UBTS_TestPlayShoot::GetStaticDescription() const
{
    return TEXT("타겟이 유효하고 무기가 있으면 사격 어빌리티를 활성화합니다.");
}

void UBTS_TestPlayShoot::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return;
    }

    // 1. 블랙보드에서 타겟 확인
    AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TestPlayAIKeys::TargetEnemy));
    const bool bHasValidTarget = IsValid(TargetActor);

    if (!bHasValidTarget)
    {
        if (bIsFiring)
        {
            StopFiring(OwnerComp);
            bIsFiring = false;
            UE_LOG(LogTestPlayShoot, Log, TEXT("사격 중지 (타겟 없음)"));
        }
        return;
    }

    // 2. AI Controller 및 Pawn 확인
    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC) return;

    APawn* Pawn = AIC->GetPawn();
    if (!Pawn) return;

    ALyraCharacter* LyraChar = Cast<ALyraCharacter>(Pawn);
    if (!LyraChar) return;

    ULyraAbilitySystemComponent* LyraASC = LyraChar->GetLyraAbilitySystemComponent();
    if (!LyraASC) return;

    // 3. 현재 장착된 무기 확인
    ULyraEquipmentManagerComponent* EquipmentComp = LyraChar->FindComponentByClass<ULyraEquipmentManagerComponent>();
    if (!EquipmentComp) return;

    ULyraRangedWeaponInstance* RangedWeapon = EquipmentComp->GetFirstInstanceOfType<ULyraRangedWeaponInstance>();
    if (!RangedWeapon)
    {
        // 무기가 없는 경우 로그 출력 (1초에 한 번만)
        static double LastLogTime = 0.0;
        double CurrentTime = FPlatformTime::Seconds();
        if (CurrentTime - LastLogTime > 1.0)
        {
            UE_LOG(LogTestPlayShoot, Warning, TEXT("[BTS] 무기가 없습니다! EquipmentManager에서 RangedWeapon을 찾을 수 없습니다."));
            LastLogTime = CurrentTime;
        }

        if (bIsFiring)
        {
            StopFiring(OwnerComp);
            bIsFiring = false;
        }
        return;
    }

    // 4. 무기 사거리 확인 (MaxDamageRange 사용)
    float DistanceToTarget = FVector::Dist(Pawn->GetActorLocation(), TargetActor->GetActorLocation());
    float WeaponRange = RangedWeapon->GetMaxDamageRange();
    
    if (DistanceToTarget > WeaponRange)
    {
        // 사거리 밖 - 사격 중지
        if (bIsFiring)
        {
            StopFiring(OwnerComp);
            bIsFiring = false;
            UE_LOG(LogTestPlayShoot, Log, TEXT("사격 중지 (사거리 밖: %.0fcm > %.0fcm)"), DistanceToTarget, WeaponRange);
        }
        return;
    }

    // 5. Fire 태그 확인
    FGameplayTag FireTag = FGameplayTag::RequestGameplayTag(TestPlayAITags::AbilityTag_Weapon_Fire, false);
    if (!FireTag.IsValid())
    {
        UE_LOG(LogTestPlayShoot, Error, TEXT("Ability.Type.Action.WeaponFire 태그가 존재하지 않습니다!"));
        return;
    }

    // 6. 쿨다운 태그 체크 - 공격 딜레이 중이면 스킵
    FGameplayTag CooldownTag = FGameplayTag::RequestGameplayTag(TestPlayAITags::CooldownTag_Weapon_MeleeFire, false);
    if (CooldownTag.IsValid() && LyraASC->HasMatchingGameplayTag(CooldownTag))
    {
        // 쿨다운 중 - 공격 시도하지 않음 (딜레이 유지)
        return;
    }

    // 5. 사격 상태 표시 (처음 시작 시에만)
    if (!bIsFiring)
    {
        bIsFiring = true;
        UE_LOG(LogTestPlayShoot, Log, TEXT("사격 시작 - 타겟: %s, 무기: %s"), 
            *TargetActor->GetName(), *RangedWeapon->GetName());
        
        // Fire 태그 어빌리티 확인 (처음 시작 시 한 번만)
        bool bFoundFireAbility = false;
        for (const FGameplayAbilitySpec& Spec : LyraASC->GetActivatableAbilities())
        {
            if (Spec.Ability && Spec.GetDynamicSpecSourceTags().HasTag(FireTag))
            {
                bFoundFireAbility = true;
                UE_LOG(LogTestPlayShoot, Log, TEXT("Fire 어빌리티 발견: %s, 활성 상태: %s"),
                    *Spec.Ability->GetClass()->GetName(),
                    Spec.IsActive() ? TEXT("Yes") : TEXT("No"));
                break;
            }
        }
        
        if (!bFoundFireAbility)
        {
            UE_LOG(LogTestPlayShoot, Warning, TEXT("Fire 태그(%s)를 가진 어빌리티가 없습니다! 등록된 어빌리티 수: %d"), 
                *FireTag.ToString(), LyraASC->GetActivatableAbilities().Num());
        }
    }
    
    // 6. 사격 어빌리티 활성화 시도
    FGameplayTagContainer FireTags;
    FireTags.AddTag(FireTag);
    
    bool bActivated = LyraASC->TryActivateAbilitiesByTag(FireTags);
    
    // 결과 로깅 (처음 실패 시 한 번만)
    static bool bLoggedFailure = false;
    if (!bActivated && !bLoggedFailure)
    {
        UE_LOG(LogTestPlayShoot, Warning, TEXT("TryActivateAbilitiesByTag 실패! Fire 태그: %s"), *FireTag.ToString());
        
        // Blocked 태그 확인
        FGameplayTag BlockedTag = FGameplayTag::RequestGameplayTag(FName("Ability.Weapon.NoFiring"), false);
        if (BlockedTag.IsValid() && LyraASC->HasMatchingGameplayTag(BlockedTag))
        {
            UE_LOG(LogTestPlayShoot, Warning, TEXT("사격 차단됨: Ability.Weapon.NoFiring 태그 활성화"));
        }
        
        // 현재 태그 출력
        FGameplayTagContainer OwnedTags;
        LyraASC->GetOwnedGameplayTags(OwnedTags);
        UE_LOG(LogTestPlayShoot, Warning, TEXT("현재 ASC 태그: %s"), *OwnedTags.ToString());
        
        bLoggedFailure = true;
    }
    else if (bActivated)
    {
        bLoggedFailure = false; // 성공하면 다음 실패 시 다시 로깅
    }
}

void UBTS_TestPlayShoot::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    if (bIsFiring)
    {
        StopFiring(OwnerComp);
        bIsFiring = false;
        UE_LOG(LogTestPlayShoot, Log, TEXT("노드 비활성화 - 사격 중지"));
    }
    
    Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

void UBTS_TestPlayShoot::StopFiring(UBehaviorTreeComponent& OwnerComp)
{
    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC) return;

    APawn* Pawn = AIC->GetPawn();
    if (!Pawn) return;

    ALyraCharacter* LyraChar = Cast<ALyraCharacter>(Pawn);
    if (!LyraChar) return;

    ULyraAbilitySystemComponent* LyraASC = LyraChar->GetLyraAbilitySystemComponent();
    if (!LyraASC) return;

    FGameplayTag FireTag = FGameplayTag::RequestGameplayTag(TestPlayAITags::AbilityTag_Weapon_Fire, false);
    if (FireTag.IsValid())
    {
        FGameplayTagContainer FireTags;
        FireTags.AddTag(FireTag);
        LyraASC->CancelAbilities(&FireTags);
    }
}
