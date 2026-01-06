// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_TestPlayShoot.generated.h"

class ULyraAbilitySystemComponent;

/**
 * UBTS_TestPlayShoot
 * 
 * 사격 서비스: 타겟(TargetEnemy)이 유효하고 무기가 있을 때 사격 어빌리티를 활성화합니다.
 * 
 * 로직:
 * 1. TickNode에서 Blackboard의 TargetEnemy가 유효한지 확인
 * 2. EquipmentManagerComponent에서 현재 무기 확인
 * 3. 무기가 있다면 TryActivateAbilitiesByTag로 사격 어빌리티 활성화
 * 4. 타겟이 없거나 무기가 없으면 CancelAbilities로 사격 중지
 */
UCLASS()
class TESTPLAYRUNTIME_API UBTS_TestPlayShoot : public UBTService
{
    GENERATED_BODY()

public:
    UBTS_TestPlayShoot();

    /** 노드에 대한 설명 반환 */
    virtual FString GetStaticDescription() const override;

protected:
    /** 매 틱마다 호출되어 사격 상태를 업데이트 */
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    
    /** 노드가 비활성화될 때 호출되어 사격을 중지 */
    virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
    /**
     * 사격 중지 헬퍼 함수
     * @param OwnerComp - Behavior Tree 컴포넌트
     */
    void StopFiring(UBehaviorTreeComponent& OwnerComp);

    /** 현재 사격 중인지 여부 */
    bool bIsFiring = false;
};
