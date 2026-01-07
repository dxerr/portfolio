// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TestPlayAIBlueprintLibrary.generated.h"

/**
 * AI 로직 보조용 블루프린트 함수 라이브러리
 */
UCLASS()
class UTestPlayAIBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 현재 장착된 무기의 사거리(MaxDamageRange) 내에 타겟이 있는지 확인합니다.
	 * * @param ShooterPawn       사격을 하는 폰 (AI 캐릭터)
	 * @param TargetActor       공격 대상 액터
	 * @param RangeTolerance    사거리 오차 허용 범위 (기본값 50.0cm)
	 * @return                  사거리 내에 있으면 true, 아니면 false
	 */
	UFUNCTION(BlueprintPure, Category = "TestPlay|AI", meta = (DefaultToSelf = "ShooterPawn"))
	static bool IsTargetInWeaponRange(APawn* ShooterPawn, AActor* TargetActor, float RangeTolerance = 50.0f);

    /**
     * (선택사항) 현재 무기의 최대 사거리를 반환합니다. 디버깅용.
     */
    UFUNCTION(BlueprintPure, Category = "TestPlay|AI", meta = (DefaultToSelf = "ShooterPawn"))
    static float GetCurrentWeaponMaxRange(APawn* ShooterPawn);
};