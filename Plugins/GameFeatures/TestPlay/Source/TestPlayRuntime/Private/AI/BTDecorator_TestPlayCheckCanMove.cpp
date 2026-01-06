// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTDecorator_TestPlayCheckCanMove.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogTestPlayCheckCanMove, Log, All);

UBTDecorator_TestPlayCheckCanMove::UBTDecorator_TestPlayCheckCanMove()
{
	NodeName = "Check Can Move";
	
	// 기본 설정
	bRequireWalkingMode = false;  // 기본적으로 Walking 외 모드도 허용
	bCheckPathFollowing = true;   // PathFollowing 컴포넌트 체크 활성화
	
	// FlowAbortMode 설정 - 조건 변경 시 실행 중단 허용
	FlowAbortMode = EBTFlowAbortMode::Both;
}

FString UBTDecorator_TestPlayCheckCanMove::GetStaticDescription() const
{
	FString Description = TEXT("이동 가능 상태 체크\n");
	
	if (bRequireWalkingMode)
	{
		Description += TEXT("- Walking 모드만 허용\n");
	}
	else
	{
		Description += TEXT("- MOVE_None 이외 모드 허용\n");
	}
	
	if (bCheckPathFollowing)
	{
		Description += TEXT("- PathFollowing 체크 활성화");
	}
	
	return Description;
}

bool UBTDecorator_TestPlayCheckCanMove::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	// AI Controller 획득
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		UE_LOG(LogTestPlayCheckCanMove, Warning, TEXT("CheckCanMove: AIController가 유효하지 않음"));
		return false;
	}

	// Pawn 획득
	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		UE_LOG(LogTestPlayCheckCanMove, Warning, TEXT("CheckCanMove: ControlledPawn이 유효하지 않음"));
		return false;
	}

	// Character로 캐스팅 (CharacterMovementComponent 접근용)
	ACharacter* Character = Cast<ACharacter>(ControlledPawn);
	if (!Character)
	{
		UE_LOG(LogTestPlayCheckCanMove, Warning, TEXT("CheckCanMove: Pawn이 Character가 아님"));
		return false;
	}

	// CharacterMovementComponent 획득
	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (!MovementComp)
	{
		UE_LOG(LogTestPlayCheckCanMove, Warning, TEXT("CheckCanMove: CharacterMovementComponent가 없음"));
		return false;
	}

	// MovementMode 체크
	EMovementMode CurrentMode = MovementComp->MovementMode;
	
	if (CurrentMode == MOVE_None)
	{
		UE_LOG(LogTestPlayCheckCanMove, Verbose, TEXT("CheckCanMove: MovementMode가 MOVE_None - 이동 불가"));
		return false;
	}

	if (bRequireWalkingMode)
	{
		// Walking 모드만 허용하는 경우
		if (CurrentMode != MOVE_Walking)
		{
			UE_LOG(LogTestPlayCheckCanMove, Verbose, TEXT("CheckCanMove: MovementMode가 Walking이 아님 (%d) - 이동 불가"), 
				static_cast<int32>(CurrentMode));
			return false;
		}
	}
	else
	{
		// MOVE_None이 아닌 모든 모드 허용
		// 이미 위에서 MOVE_None 체크했으므로 여기까지 왔으면 OK
	}

	// PathFollowingComponent 체크 (선택적)
	if (bCheckPathFollowing)
	{
		UPathFollowingComponent* PathFollowingComp = AIController->GetPathFollowingComponent();
		if (!PathFollowingComp)
		{
			UE_LOG(LogTestPlayCheckCanMove, Warning, TEXT("CheckCanMove: PathFollowingComponent가 없음"));
			return false;
		}

		// PathFollowing 상태 확인 - Idle이거나 Moving이면 OK
		EPathFollowingStatus::Type PathStatus = PathFollowingComp->GetStatus();
		if (PathStatus == EPathFollowingStatus::Paused)
		{
			UE_LOG(LogTestPlayCheckCanMove, Verbose, TEXT("CheckCanMove: PathFollowing이 일시정지됨 - 이동 불가"));
			return false;
		}
	}

	UE_LOG(LogTestPlayCheckCanMove, Verbose, TEXT("CheckCanMove: 이동 가능 상태 확인됨 (MovementMode: %d)"), 
		static_cast<int32>(CurrentMode));
	
	return true;
}
