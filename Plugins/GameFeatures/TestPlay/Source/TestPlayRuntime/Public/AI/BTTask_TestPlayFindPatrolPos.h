// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_TestPlayFindPatrolPos.generated.h"

/**
 * AI 탐색을 위한 랜덤 위치 찾기 태스크
 * 현재 위치(또는 SelfActor)를 기준으로 지정된 반경 내의 Navigable한 랜덤 위치를 찾아
 * 블랙보드의 MoveGoal 키에 저장합니다.
 */
UCLASS()
class TESTPLAYRUNTIME_API UBTTask_TestPlayFindPatrolPos : public UBTTask_BlackboardBase
{
    GENERATED_BODY()

public:
    UBTTask_TestPlayFindPatrolPos();

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual FString GetStaticDescription() const override;

protected:
    /** 탐색 반경 (cm) */
    UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "500.0"))
    float SearchRadius;
};
