// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTDecorator_TestPlayCheckCanMove.h"
#include "AI/TestPlayAIConstants.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
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
	
	// 대상 거리 체크 기본 설정
	bCheckTargetDistance = true;  // 대상 거리 체크 활성화
	AcceptanceRadius = 150.0f;    // 기본 거리 임계값 150cm
	
	// TargetKey 필터 설정 (Object/Actor 타입만 허용)
	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTDecorator_TestPlayCheckCanMove, TargetKey), AActor::StaticClass());
	TargetKey.SelectedKeyName = TestPlayAIKeys::TargetEnemy;
	
	// FlowAbortMode 설정 - 조건 변경 시 실행 중단 허용
	FlowAbortMode = EBTFlowAbortMode::Both;
}

void UBTDecorator_TestPlayCheckCanMove::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	
	// 블랙보드 에셋에서 키 ID를 초기화
	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetKey.ResolveSelectedKey(*BBAsset);
	}
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
		Description += TEXT("- PathFollowing 체크 활성화\n");
	}
	
	if (bCheckTargetDistance)
	{
		Description += FString::Printf(TEXT("- 대상 거리 체크: %.0fcm 이내 시 이동 불필요\n"), AcceptanceRadius);
		Description += FString::Printf(TEXT("- 대상 키: %s"), *TargetKey.SelectedKeyName.ToString());
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

	// 대상과의 거리 체크 (선택적)
	if (bCheckTargetDistance)
	{
		UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
		if (BlackboardComp && TargetKey.SelectedKeyType != nullptr)
		{
			// 블랙보드에서 대상 액터(TargetEnemy) 획득
			AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));
			if (TargetActor)
			{
				FVector TargetLocation = TargetActor->GetActorLocation();
				FVector AILocation = ControlledPawn->GetActorLocation();
				
				// 2D 거리 계산 (높이 무시 - Z축 차이는 무시)
				float Distance2D = FVector::DistXY(AILocation, TargetLocation);
				
				if (Distance2D <= AcceptanceRadius)
				{
					UE_LOG(LogTestPlayCheckCanMove, Verbose, 
						TEXT("CheckCanMove: 대상에 충분히 가까움 (거리: %.1f, 임계값: %.1f) - 이동 불필요"),
						Distance2D, AcceptanceRadius);
					return false;  // 이미 충분히 가까움 - MoveTo 실행 안 함
				}
				
				UE_LOG(LogTestPlayCheckCanMove, Verbose, 
					TEXT("CheckCanMove: 대상까지 거리 %.1f (임계값: %.1f) - 이동 필요"),
					Distance2D, AcceptanceRadius);
			}
			else
			{
				UE_LOG(LogTestPlayCheckCanMove, Verbose, 
					TEXT("CheckCanMove: 대상(TargetEnemy)이 없음 - 이동 가능 상태 유지"));
			}
		}
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
