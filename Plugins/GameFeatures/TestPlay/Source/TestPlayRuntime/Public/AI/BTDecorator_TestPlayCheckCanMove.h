// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BlackboardComponent.h"
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
 * - (선택) 대상과의 거리 체크 (이미 가까이 있으면 이동 불필요)
 * - (선택) PathFollowing 상태
 */
UCLASS()
class TESTPLAYRUNTIME_API UBTDecorator_TestPlayCheckCanMove : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_TestPlayCheckCanMove();

	virtual FString GetStaticDescription() const override;
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

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

	/**
	 * 대상과의 거리를 체크할지 여부
	 * true이면 TargetKey(대상 액터) 위치와의 거리가 AcceptanceRadius 이내일 때 이동 불필요로 판정
	 */
	UPROPERTY(EditAnywhere, Category = "Target Distance Check")
	bool bCheckTargetDistance;

	/**
	 * 거리 체크 대상 액터를 저장하는 블랙보드 키
	 * Object(Actor) 타입이어야 합니다
	 */
	UPROPERTY(EditAnywhere, Category = "Target Distance Check")
	FBlackboardKeySelector TargetKey;

	/**
	 * 충분히 가까운 것으로 판정하는 거리 임계값 (cm)
	 * 대상까지의 거리가 이 값 이내이면 이동 불필요로 판정
	 */
	UPROPERTY(EditAnywhere, Category = "Target Distance Check", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AcceptanceRadius;
};
