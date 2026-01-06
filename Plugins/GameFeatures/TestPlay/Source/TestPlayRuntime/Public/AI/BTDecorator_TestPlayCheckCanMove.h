// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_TestPlayCheckCanMove.generated.h"

/**
 * AI 캐릭터가 이동 가능한 상태인지 확인하는 Decorator
 * 
 * CharacterMovementComponent의 MovementMode가 None인 경우 등
 * 외부 요인에 의해 이동이 제한된 상황을 감지하여
 * MoveTo 태스크 실행을 방지합니다.
 * 
 * 체크 항목:
 * - CharacterMovementComponent 유효성
 * - MovementMode != MOVE_None
 * - (선택) 추가 이동 가능 조건
 */
UCLASS()
class TESTPLAYRUNTIME_API UBTDecorator_TestPlayCheckCanMove : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_TestPlayCheckCanMove();

	virtual FString GetStaticDescription() const override;

protected:
	/** 이동 가능 조건 계산 */
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	/** 
	 * MOVE_Walking 상태만 허용할지 여부
	 * false이면 MOVE_Falling, MOVE_Swimming 등도 이동 가능으로 판정
	 */
	UPROPERTY(EditAnywhere, Category = "Movement")
	bool bRequireWalkingMode;

	/**
	 * PathFollowingComponent 유효성도 함께 체크할지 여부
	 */
	UPROPERTY(EditAnywhere, Category = "Movement")
	bool bCheckPathFollowing;
};
