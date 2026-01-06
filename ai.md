🛠️ Lyra AI Behavior Tree Service C++ 구현 가이드

이 문서는 Lyra 프로젝트의 블루프린트 AI 로직(BTS_Shoot, CheckAmmo 등)을 **Native C++**로 포팅하기 위한 상세 명세서 및 소스 코드입니다.

📁 프로젝트 환경 설정 (Module Setup)

이 코드는 독립된 GameFeature 플러그인(TestPlay) 내부에서 동작합니다. 컴파일을 위해 Build.cs에 Lyra 관련 모듈 의존성을 반드시 추가해야 합니다.

1. Build.cs 의존성 설정

TestPlayRuntime.Build.cs 파일에 다음 모듈이 포함되어야 합니다.

// TestPlayRuntime.Build.cs
PublicDependencyModuleNames.AddRange(new string[] {
    "Core",
    "CoreUObject",
    "Engine",
    "AIModule",
    "GameplayTags",
    "GameplayAbilities",
    "LyraGame", // Lyra 핵심 클래스 접근용
    "ModularGameplay"
});


🔑 2. 공통 헤더: 키 & 태그 정의 (Constants)

하드코딩을 방지하기 위해 블랙보드 키와 게임플레이 태그를 관리하는 헤더 파일을 먼저 정의합니다.

TestPlayAIConstants.h

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

namespace TestPlayAIKeys
{
    // Blackboard Keys
    static const FName TargetEnemy = FName("TargetEnemy");
    static const FName OutOfAmmo = FName("OutOfAmmo");
    static const FName SelfActor = FName("SelfActor");
    static const FName MoveGoal = FName("MoveGoal");
}

namespace TestPlayAITags
{
    // Gameplay Tags
    static const FName InputTag_Weapon_Fire = FName("InputTag.Weapon.Fire");
    static const FName InputTag_Weapon_Reload = FName("InputTag.Weapon.Reload");
}


🔫 3. 사격 서비스 (Shoot Service)

로직 분석 (Logic Analysis)

PDF의 BTS_Shoot 블루프린트는 다음 로직을 수행합니다.

Tick Node: 매 프레임(또는 지정된 간격)마다 실행됩니다.

Target Validation: 블랙보드의 TargetEnemy가 유효한지 확인합니다.

Tag Injection:

타겟이 있다면: ASC(Ability System Component)에 InputTag.Weapon.Fire 태그를 추가합니다. (트리거를 당김)

타겟이 없다면: ASC에서 해당 태그를 제거합니다. (트리거를 놓음)

C++ 구현 (UBTS_TestPlayShoot)

// BTS_TestPlayShoot.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_TestPlayShoot.generated.h"

UCLASS()
class TESTPLAYRUNTIME_API UBTS_TestPlayShoot : public UBTService
{
    GENERATED_BODY()

public:
    UBTS_TestPlayShoot();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

    // 헬퍼 함수: 태그 추가/제거
    void SetFireInputTag(UBehaviorTreeComponent& OwnerComp, bool bShouldFire);
};


// BTS_TestPlayShoot.cpp
#include "BTS_TestPlayShoot.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "TestPlayAIConstants.h" // 위에서 정의한 상수 헤더

UBTS_TestPlayShoot::UBTS_TestPlayShoot()
{
    NodeName = "TestPlay Shoot";
    Interval = 0.5f;        // 0.5초마다 체크
    RandomDeviation = 0.1f; // 랜덤 오차
}

void UBTS_TestPlayShoot::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp) return;

    // 1. 타겟 확인
    AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TestPlayAIKeys::TargetEnemy));
    bool bHasTarget = IsValid(TargetActor);

    // 2. 태그 주입/제거
    SetFireInputTag(OwnerComp, bHasTarget);
}

void UBTS_TestPlayShoot::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 노드가 비활성화될 때(다른 행동으로 전환 시) 반드시 사격을 멈춰야 함
    SetFireInputTag(OwnerComp, false);
    Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

void UBTS_TestPlayShoot::SetFireInputTag(UBehaviorTreeComponent& OwnerComp, bool bShouldFire)
{
    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC) return;

    APawn* Pawn = AIC->GetPawn();
    if (!Pawn) return;

    // Lyra의 ASC 가져오기
    UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
    if (ASC)
    {
        FGameplayTag FireTag = FGameplayTag::RequestGameplayTag(TestPlayAITags::InputTag_Weapon_Fire);

        if (bShouldFire)
        {
            // 태그를 추가하여 '버튼 누름' 상태 유지
            ASC->AddLooseGameplayTag(FireTag);
        }
        else
        {
            // 태그를 제거하여 '버튼 뗌' 상태로 전환
            ASC->RemoveLooseGameplayTag(FireTag);
        }
    }
}


🔄 4. 재장전 서비스 (Reload Service)

로직 분석

Check Flag: 블랙보드의 OutOfAmmo 값이 true인지 확인합니다.

Activate Ability: true라면 InputTag.Weapon.Reload 태그를 사용하여 재장전 어빌리티를 즉시 실행합니다.

Lyra의 재장전은 상태(State)가 아니라 일회성 트리거(Trigger)이므로 TryActivateAbilitiesByTag를 사용하는 것이 적합합니다.

C++ 구현 (UBTS_TestPlayReloadWeapon)

// BTS_TestPlayReloadWeapon.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_TestPlayReloadWeapon.generated.h"

UCLASS()
class TESTPLAYRUNTIME_API UBTS_TestPlayReloadWeapon : public UBTService
{
    GENERATED_BODY()

public:
    UBTS_TestPlayReloadWeapon();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};


// BTS_TestPlayReloadWeapon.cpp
#include "BTS_TestPlayReloadWeapon.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "TestPlayAIConstants.h"

UBTS_TestPlayReloadWeapon::UBTS_TestPlayReloadWeapon()
{
    NodeName = "TestPlay Reload Weapon";
    Interval = 1.0f; // 1초마다 체크
}

void UBTS_TestPlayReloadWeapon::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp) return;

    // 1. 탄약 부족 상태 확인
    bool bOutOfAmmo = BlackboardComp->GetValueAsBool(TestPlayAIKeys::OutOfAmmo);

    if (bOutOfAmmo)
    {
        AAIController* AIC = OwnerComp.GetAIOwner();
        if (AIC && AIC->GetPawn())
        {
            UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AIC->GetPawn());
            if (ASC)
            {
                // 2. 재장전 어빌리티 시도 (태그로 트리거)
                FGameplayTagContainer ReloadTags;
                ReloadTags.AddTag(FGameplayTag::RequestGameplayTag(TestPlayAITags::InputTag_Weapon_Reload));
                
                ASC->TryActivateAbilitiesByTag(ReloadTags);
            }
        }
    }
}


📦 5. 탄약 확인 서비스 (Check Ammo Service)

로직 분석

Get Weapon: 현재 캐릭터가 들고 있는 무기(Equipment)를 찾습니다.

Get Ammo: 무기의 현재 탄약량을 확인합니다. (Lyra에서는 보통 AttributeSet이나 WeaponInstance에 저장됨)

Update Blackboard: 탄약이 0 이하면 OutOfAmmo를 true로 설정합니다.

C++ 구현 (UBTS_TestPlayCheckAmmo)

// BTS_TestPlayCheckAmmo.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_TestPlayCheckAmmo.generated.h"

UCLASS()
class TESTPLAYRUNTIME_API UBTS_TestPlayCheckAmmo : public UBTService
{
    GENERATED_BODY()

public:
    UBTS_TestPlayCheckAmmo();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};


// BTS_TestPlayCheckAmmo.cpp
#include "BTS_TestPlayCheckAmmo.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "Weapons/LyraWeaponInstance.h" // Lyra 무기 클래스
#include "Equipment/LyraEquipmentManagerComponent.h" // 장비 매니저
#include "Character/LyraCharacter.h" // Lyra 캐릭터
#include "TestPlayAIConstants.h"

UBTS_TestPlayCheckAmmo::UBTS_TestPlayCheckAmmo()
{
    NodeName = "TestPlay Check Ammo";
    Interval = 0.5f;
}

void UBTS_TestPlayCheckAmmo::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    AAIController* AIC = OwnerComp.GetAIOwner();
    
    if (!BlackboardComp || !AIC) return;

    ALyraCharacter* LyraChar = Cast<ALyraCharacter>(AIC->GetPawn());
    if (!LyraChar) return;

    // 1. 장비 매니저에서 현재 무기 가져오기
    // (Lyra 버전에 따라 접근 방식이 다를 수 있으나, 일반적으로 EquipmentManager를 통함)
    bool bIsOutOfAmmo = false;
    
    if (ULyraEquipmentManagerComponent* EquipmentComp = LyraChar->FindComponentByClass<ULyraEquipmentManagerComponent>())
    {
        // 첫 번째 무기 인스턴스를 찾거나, 현재 활성화된 무기를 찾음
        // 여기서는 예시로 첫 번째 LyraWeaponInstance를 가져옴
        TArray<ULyraEquipmentInstance*> Equipments = EquipmentComp->GetEquipmentInstancesOfType(ULyraWeaponInstance::StaticClass());
        
        for (ULyraEquipmentInstance* Equip : Equipments)
        {
            if (ULyraWeaponInstance* Weapon = Cast<ULyraWeaponInstance>(Equip))
            {
                // LyraWeaponInstance 내부의 탄약 확인 (혹은 AttributeSet 확인)
                // Lyra 5.0+ 기준: WeaponInstance가 탄약을 관리하거나 ASC Attribute를 참조함
                // 여기서는 가상 함수 GetCurrentAmmo()를 가정하거나 Attribute를 체크해야 함
                
                // [간소화 로직] 실제 Lyra 탄약 Attribute 확인 필요 (LyraAmmoAttributeSet)
                // 예시: if (Weapon->GetAmmo() <= 0) ...
                
                // 임시: 무기가 유효하면 탄약이 있다고 가정, 실제 구현 시 AttributeSet 값 읽어야 함
                // bIsOutOfAmmo = (CurrentAmmo <= 0);
                break; 
            }
        }
    }

    // 2. 블랙보드 업데이트
    // (디버깅을 위해 임시로 false 고정 혹은 로직 연결 필요)
    BlackboardComp->SetValueAsBool(TestPlayAIKeys::OutOfAmmo, bIsOutOfAmmo);
}


🎯 6. 포커스 설정 서비스 (Set Focus Service)

로직 분석

Get Target: 블랙보드에서 TargetEnemy를 가져옵니다.

Set Focus: AI 컨트롤러의 SetFocus를 사용하여 시선을 고정합니다.

Clear Focus: 타겟이 없으면 ClearFocus합니다.

C++ 구현 (UBTS_TestPlaySetFocus)

// BTS_TestPlaySetFocus.h
#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_TestPlaySetFocus.generated.h"

UCLASS()
class TESTPLAYRUNTIME_API UBTS_TestPlaySetFocus : public UBTService
{
    GENERATED_BODY()

public:
    UBTS_TestPlaySetFocus();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};


// BTS_TestPlaySetFocus.cpp (헤더는 위와 유사하므로 생략)

#include "BTS_TestPlaySetFocus.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "TestPlayAIConstants.h"

UBTS_TestPlaySetFocus::UBTS_TestPlaySetFocus()
{
    NodeName = "TestPlay Set Focus";
    Interval = 0.1f; // 시선 처리는 자주 업데이트 (0.1초)
}

void UBTS_TestPlaySetFocus::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    AAIController* AIC = OwnerComp.GetAIOwner();

    if (!BlackboardComp || !AIC) return;

    // 1. 타겟 가져오기
    AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TestPlayAIKeys::TargetEnemy));

    // 2. 포커스 설정
    if (IsValid(TargetActor))
    {
        AIC->SetFocus(TargetActor);
    }
    else
    {
        AIC->ClearFocus(EAIFocusPriority::Gameplay);
    }
}


📝 요약 및 적용 방법

파일 생성: 위 코드를 참고하여 Plugins/GameFeatures/TestPlay/Source/TestPlayRuntime 경로에 .h와 .cpp 파일들을 생성합니다.

컴파일: 에디터를 닫고 IDE(Visual Studio/Rider)에서 프로젝트를 컴파일합니다.

블루프린트 교체:

기존 Behavior Tree (BT_Lyra_Shooter_...)를 엽니다.

기존의 블루프린트 버전 BTS (BTS_Shoot 등)를 삭제합니다.

새로 만든 C++ 버전 (TestPlay Shoot 등)을 추가합니다.

테스트: SetFireInputTag 등의 함수에 중단점(Breakpoint)을 걸고 정상적으로 태그가 주입되는지 확인합니다.