// Copyright Epic Games, Inc. All Rights Reserved.

#include "AI/BTS_TestPlaySetFocus.h"
#include "AI/TestPlayAIConstants.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTS_TestPlaySetFocus::UBTS_TestPlaySetFocus()
{
    NodeName = "TestPlay Set Focus";
    
    // 0.1초마다 체크 (시선 처리는 자주 업데이트 필요)
    Interval = 0.1f;
    RandomDeviation = 0.01f;
    
    bNotifyTick = true;
    bNotifyCeaseRelevant = true;
}

FString UBTS_TestPlaySetFocus::GetStaticDescription() const
{
    return TEXT("타겟을 향해 AI의 시선을 고정합니다.");
}

void UBTS_TestPlaySetFocus::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
    AAIController* AIC = OwnerComp.GetAIOwner();

    if (!BlackboardComp || !AIC)
    {
        UE_LOG(LogTemp, Warning, TEXT("[BTS_TestPlaySetFocus] BlackboardComponent 또는 AIController가 없습니다."));
        return;
    }

    // 1. 블랙보드에서 타겟 가져오기
    AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TestPlayAIKeys::TargetEnemy));

    // 2. 포커스 설정/해제
    if (IsValid(TargetActor))
    {
        // 타겟이 변경된 경우에만 로그 출력
        if (CurrentFocusTarget.Get() != TargetActor)
        {
            AIC->SetFocus(TargetActor);
            CurrentFocusTarget = TargetActor;
            UE_LOG(LogTemp, Log, TEXT("[BTS_TestPlaySetFocus] 포커스 설정: %s"), *TargetActor->GetName());
        }
    }
    else
    {
        // 포커스가 있었다면 해제
        if (CurrentFocusTarget.IsValid())
        {
            AIC->ClearFocus(EAIFocusPriority::Gameplay);
            CurrentFocusTarget.Reset();
            UE_LOG(LogTemp, Log, TEXT("[BTS_TestPlaySetFocus] 포커스 해제"));
        }
    }
}

void UBTS_TestPlaySetFocus::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    // 노드가 비활성화될 때 포커스 해제
    AAIController* AIC = OwnerComp.GetAIOwner();
    if (AIC && CurrentFocusTarget.IsValid())
    {
        AIC->ClearFocus(EAIFocusPriority::Gameplay);
        CurrentFocusTarget.Reset();
        UE_LOG(LogTemp, Log, TEXT("[BTS_TestPlaySetFocus] 노드 비활성화 - 포커스 해제"));
    }
    
    Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}
