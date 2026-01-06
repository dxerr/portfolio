// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_TestPlayCheckAmmo.generated.h"

/**
 * UBTS_TestPlayCheckAmmo
 * 
 * 탄약 확인 서비스: 현재 무기의 탄약을 주기적으로 확인하여 블랙보드를 업데이트합니다.
 * 
 * 로직:
 * 1. AI 캐릭터가 현재 장착한 무기를 가져옴
 * 2. 무기의 현재 탄약량을 확인
 * 3. 탄약이 0 이하라면 Blackboard의 OutOfAmmo 키를 true로 설정
 * 
 * Note: Lyra에서는 탄약을 LyraRangedWeaponInstance 또는 GAS Attribute로 관리할 수 있음
 */
UCLASS()
class TESTPLAYRUNTIME_API UBTS_TestPlayCheckAmmo : public UBTService
{
    GENERATED_BODY()

public:
    UBTS_TestPlayCheckAmmo();

    /** 노드에 대한 설명 반환 */
    virtual FString GetStaticDescription() const override;

protected:
    /** 매 틱마다 호출되어 탄약 상태를 확인 */
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
    /**
     * 현재 장착된 무기의 탄약량을 확인
     * @param Pawn - AI 폰
     * @return 현재 탄약량 (무기가 없으면 -1 반환)
     */
    int32 GetCurrentAmmo(APawn* Pawn) const;
};
