// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTS_TestPlayFindEnemy.generated.h"

/**
 * 주변의 적을 탐색하여 블랙보드의 TargetEnemy 키에 할당하는 서비스
 * LyraTeamSubsystem을 사용하여 적대 관계(Hostile)를 판단합니다.
 */
UCLASS()
class TESTPLAYRUNTIME_API UBTS_TestPlayFindEnemy : public UBTService_BlackboardBase
{
    GENERATED_BODY()

public:
    UBTS_TestPlayFindEnemy();

    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
    virtual FString GetStaticDescription() const override;

protected:
    /** 탐색 반경 (cm) */
    UPROPERTY(EditAnywhere, Category = "AI", meta = (ClampMin = "500.0"))
    float SearchRadius;

    /** 자신이 데미지를 입었을 때 즉시 반응할지 여부 (향후 확장용) */
    UPROPERTY(EditAnywhere, Category = "AI")
    bool bCheckSensedActors;
};
