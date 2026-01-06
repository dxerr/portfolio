// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_TestPlayReloadWeapon.generated.h"

/**
 * UBTS_TestPlayReloadWeapon
 * 
 * 재장전 서비스: 탄약이 떨어졌다는 신호가 오면 재장전을 수행합니다.
 * 
 * 로직:
 * 1. Blackboard의 OutOfAmmo 값이 true인지 확인
 * 2. true라면 ASC에 InputTag.Weapon.Reload 태그로 어빌리티를 활성화
 * 
 * Note: Lyra의 재장전은 상태(State)가 아니라 일회성 트리거(Trigger)이므로
 *       TryActivateAbilitiesByTag를 사용합니다.
 */
UCLASS()
class TESTPLAYRUNTIME_API UBTS_TestPlayReloadWeapon : public UBTService
{
    GENERATED_BODY()

public:
    UBTS_TestPlayReloadWeapon();

    /** 노드에 대한 설명 반환 */
    virtual FString GetStaticDescription() const override;

protected:
    /** 매 틱마다 호출되어 재장전 여부를 확인 */
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
    /** 재장전 쿨다운 방지용 (연속 재장전 방지) */
    float LastReloadAttemptTime = 0.0f;
    
    /** 재장전 시도 간 최소 대기 시간 (초) */
    UPROPERTY(EditAnywhere, Category = "TestPlay|Reload")
    float ReloadCooldown = 2.0f;
};
