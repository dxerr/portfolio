// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTTask_TestPlayFindPatrolPos.h"
#include "AI/TestPlayAIConstants.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"

UBTTask_TestPlayFindPatrolPos::UBTTask_TestPlayFindPatrolPos()
{
    NodeName = "Find Patrol Pos";
    SearchRadius = 1500.0f; // 기본 반경 설정

    // BlackboardKey는 Vector 타입만 허용하도록 필터링
    BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_TestPlayFindPatrolPos, BlackboardKey));
    
    // 기본적으로 MoveGoal 키가 선택되도록 설정
    BlackboardKey.SelectedKeyName = TestPlayAIKeys::MoveGoal;
}

EBTNodeResult::Type UBTTask_TestPlayFindPatrolPos::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIC = OwnerComp.GetAIOwner();
    if (!AIC)
    {
        return EBTNodeResult::Failed;
    }

    APawn* Pawn = AIC->GetPawn();
    if (!Pawn)
    {
        return EBTNodeResult::Failed;
    }

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys)
    {
        return EBTNodeResult::Failed;
    }

    // 탐색 원점 설정 (현재 Pawn의 위치)
    FVector Origin = Pawn->GetActorLocation();
    FNavLocation ResultLocation;

    // 랜덤 위치 찾기 (NavMesh 위에서)
    bool bSuccess = NavSys->GetRandomReachablePointInRadius(Origin, SearchRadius, ResultLocation);

    if (bSuccess)
    {
        UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
        if (BlackboardComp)
        {
            // 찾은 위치를 블랙보드에 저장 (BlackboardKey는 에디터에서 MoveGoal로 설정됨을 가정)
            // 기본 생성자에서 필터를 걸어줬으므로, 사용자가 선택한 키에 값을 넣습니다.
            BlackboardComp->SetValueAsVector(BlackboardKey.SelectedKeyName, ResultLocation.Location);
            return EBTNodeResult::Succeeded;
        }
    }

    return EBTNodeResult::Failed;
}

FString UBTTask_TestPlayFindPatrolPos::GetStaticDescription() const
{
    return FString::Printf(TEXT("반경 %.0f 내의 무작위 순찰 위치를 찾아 MoveGoal에 저장합니다."), SearchRadius);
}
