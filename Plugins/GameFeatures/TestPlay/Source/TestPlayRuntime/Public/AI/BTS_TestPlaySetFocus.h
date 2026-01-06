// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTS_TestPlaySetFocus.generated.h"

/**
 * UBTS_TestPlaySetFocus
 * 
 * 포커스 설정 서비스: AI가 항상 타겟을 바라보도록 설정합니다.
 * 
 * 로직:
 * 1. Blackboard의 TargetEnemy 액터를 가져옴
 * 2. 유효한 타겟이 있으면 AIController의 SetFocus() 호출
 * 3. 타겟이 없으면 ClearFocus() 호출
 */
UCLASS()
class TESTPLAYRUNTIME_API UBTS_TestPlaySetFocus : public UBTService
{
    GENERATED_BODY()

public:
    UBTS_TestPlaySetFocus();

    /** 노드에 대한 설명 반환 */
    virtual FString GetStaticDescription() const override;

protected:
    /** 매 틱마다 호출되어 포커스를 업데이트 */
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    
    /** 노드가 비활성화될 때 호출되어 포커스를 해제 */
    virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
    /** 현재 포커스 중인 타겟 (변경 감지용) */
    TWeakObjectPtr<AActor> CurrentFocusTarget;
};
