// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Cosmetics/LyraControllerComponent_CharacterParts.h"
#include "TestPlayControllerComponent_CharacterParts.generated.h"

class AActor;
class APawn;

/**
 * 팀에 따라 다른 캐릭터 파트를 적용하는 ControllerComponent
 * 
 * - 로컬 플레이어: LocalPlayerPartClass 적용
 * - 같은 팀: SameTeamPartClasses에서 랜덤 선택
 * - 다른 팀: DifferentTeamPartClasses에서 랜덤 선택
 * - 배열이 비어있으면 LocalPlayerPartClass를 Fallback으로 사용
 */
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class TESTPLAYRUNTIME_API UTestPlayControllerComponent_CharacterParts : public ULyraControllerComponent_CharacterParts
{
	GENERATED_BODY()

public:
	UTestPlayControllerComponent_CharacterParts(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void BeginPlay() override;
	//~End of UActorComponent interface

protected:
	/** 로컬 플레이어에게 적용할 파트 액터 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestPlay|CharacterParts")
	TSubclassOf<AActor> LocalPlayerPartClass;

	/** 같은 팀에게 적용할 파트 액터 클래스 배열 (랜덤 선택) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestPlay|CharacterParts")
	TArray<TSubclassOf<AActor>> SameTeamPartClasses;

	/** 다른 팀에게 적용할 파트 액터 클래스 배열 (랜덤 선택) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestPlay|CharacterParts")
	TArray<TSubclassOf<AActor>> DifferentTeamPartClasses;

private:
	/** Pawn 소유 변경 시 호출 */
	UFUNCTION()
	void OnPossessedPawnChangedForTeam(APawn* OldPawn, APawn* NewPawn);

	/** 팀에 따라 적절한 파트 클래스를 선택하여 추가 */
	void ApplyCharacterPartByTeam(APawn* OwningPawn);

	/** 파트가 이미 적용되었는지 여부 */
	bool bPartApplied = false;
};
