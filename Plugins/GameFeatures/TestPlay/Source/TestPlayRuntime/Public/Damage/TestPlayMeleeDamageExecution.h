// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "TestPlayMeleeDamageExecution.generated.h"

class ULyraHealthSet;
class ULyraCombatSet;

/**
 * UTestPlayMeleeDamageExecution
 *
 * ULyraDamageExecution을 기반으로 한 근접 공격 전용 대미지 계산 클래스입니다.
 * UAdvancedMeleeTraceComponent의 OnMeleeHit 델리게이트와 연동하여 사용됩니다.
 *
 * 주요 기능:
 * 1. BaseDamage 캡처 (LyraCombatSet)
 * 2. 팀 대미지 허용 체크 (LyraTeamSubsystem)
 * 3. 거리/물리 재질 감쇠 (선택적)
 * 4. 최종 대미지를 Health Damage 속성에 적용
 *
 * 사용법:
 * 1. GameplayEffect 에셋에서 이 클래스를 Execution으로 설정
 * 2. UAdvancedMeleeTraceComponent::OnMeleeHit 발생 시 해당 GE 적용
 */
UCLASS()
class TESTPLAYRUNTIME_API UTestPlayMeleeDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UTestPlayMeleeDamageExecution();

protected:
	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput
	) const override;
};
