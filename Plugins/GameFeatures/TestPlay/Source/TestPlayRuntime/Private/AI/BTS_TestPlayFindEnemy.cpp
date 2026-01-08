// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTS_TestPlayFindEnemy.h"
#include "AI/TestPlayAIConstants.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Teams/LyraTeamSubsystem.h"
#include "Character/LyraCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"

UBTS_TestPlayFindEnemy::UBTS_TestPlayFindEnemy()
{
    NodeName = "TestPlay Find Enemy";
    SearchRadius = 3000.0f; // 기본 탐색 반경 30m
    
    // 0.5초마다 체크 (너무 자주하면 성능 부하)
    Interval = 0.5f;
    RandomDeviation = 0.1f;
    
    // 기본적으로 TargetEnemy 키를 업데이트하도록 필터링
    BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTS_TestPlayFindEnemy, BlackboardKey), AActor::StaticClass());
    BlackboardKey.SelectedKeyName = TestPlayAIKeys::TargetEnemy;
}

void UBTS_TestPlayFindEnemy::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    if (!BlackboardComp)
    {
        return;
    }

    AAIController* MyController = OwnerComp.GetAIOwner();
    if (!MyController)
    {
        return;
    }

    APawn* MyPawn = MyController->GetPawn();
    if (!MyPawn)
    {
        return;
    }

    // Lyra Team Subsystem 가져오기
    ULyraTeamSubsystem* TeamSubsystem = MyPawn->GetWorld()->GetSubsystem<ULyraTeamSubsystem>();
    if (!TeamSubsystem)
    {
        return;
    }

    // 현재 저장된 타겟이 유효하고 범위 내에 있으며 살아있는지 확인
    AActor* CurrentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(BlackboardKey.SelectedKeyName));
    if (IsValid(CurrentTarget))
    {
        float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), CurrentTarget->GetActorLocation());
        bool bIsInRange = DistSq <= (SearchRadius * SearchRadius);
        
        // 시야 체크 등은 PerceptionSystem에서 하는게 좋지만, 여기서는 간단히 거리와 생존 여부만 체크
        bool bIsAlive = true; // 기본적으로 액터가 유효하면 살아있다고 가정 (HealthComp 체크 추가 가능)
        
        /* HealthComponent를 통한 사망 체크 (선택 사항)
        if (ULyraHealthComponent* HealthComp = ULyraHealthComponent::FindHealthComponent(CurrentTarget))
        {
            if (HealthComp->GetHealth() <= 0.0f) bIsAlive = false;
        }
        */

        if (bIsInRange && bIsAlive)
        {
            // 아직 유효하므로 유지
            return;
        }
        else
        {
            // 조건 불만족 시 타겟 해제
            BlackboardComp->SetValueAsObject(BlackboardKey.SelectedKeyName, nullptr);
            CurrentTarget = nullptr;
        }
    }

    	// 새로운 적 탐색 (Sphere Overlap으로 최적화 - GetAllActorsOfClass 대체)
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FindEnemy), false, MyPawn);
	
	GetWorld()->OverlapMultiByObjectType(
		Overlaps,
		MyPawn->GetActorLocation(),
		FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(SearchRadius),
		QueryParams
	);

	AActor* BestTarget = nullptr;
	float BestDistSq = SearchRadius * SearchRadius;

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Actor = Overlap.GetActor();
		if (!Actor || Actor == MyPawn)
		{
			continue;
		}

		// LyraCharacter만 대상으로 함
		if (!Actor->IsA(ALyraCharacter::StaticClass()))
		{
			continue;
		}

		// 1. 살아있는지 확인 (간단히 액터 유효성만 체크하거나 HealthComponent 확인)
		// (프로토타입 단계이므로 생략, 죽으면 Destroy되거나 Ragdoll이 됨을 가정)

		// 2. 적대 관계인지 확인
		ELyraTeamComparison TeamRelationship = TeamSubsystem->CompareTeams(MyPawn, Actor);
		if (TeamRelationship != ELyraTeamComparison::DifferentTeams)
		{
			continue; // 같은 팀이거나 중립이면 무시
		}

		// 3. 거리 확인 (더 가까운 적 찾기)
		float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), Actor->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestTarget = Actor;
		}
	}
    
    if (BestTarget)
    {
        BlackboardComp->SetValueAsObject(BlackboardKey.SelectedKeyName, BestTarget);
    }
}

FString UBTS_TestPlayFindEnemy::GetStaticDescription() const
{
    return FString::Printf(TEXT("반경 %.0f 내의 적대적인(Different Team) 캐릭터를 찾아 TargetEnemy에 설정합니다."), SearchRadius);
}
