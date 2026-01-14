// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Cosmetics/LyraControllerComponent_CharacterParts.h"
#include "TestPlayControllerComponent_CharacterParts.generated.h"

class AActor;
class APawn;
class ULyraPawnComponent_CharacterParts;
class UCustomizableSkeletalComponent;

/**
 * Mutable 파라미터 랜덤화 옵션
 */
USTRUCT(BlueprintType)
struct FRandomPartOptions
{
	GENERATED_BODY()

	/** 랜덤화 활성화 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mutable")
	bool bEnableRandomization = false;

	/** 랜덤화할 파라미터 이름 화이트리스트 (비어있으면 전체 대상) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mutable")
	TArray<FString> WhitelistParameterNames;

	/** 컬러 파라미터용 사전 정의 팔레트 (비어있으면 완전 랜덤) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mutable")
	TArray<FLinearColor> ColorPalette;
};

/**
 * 팀에 따라 다른 캐릭터 파트를 적용하는 ControllerComponent
 * 
 * - 로컬 플레이어: LocalPlayerPartClass 적용
 * - 같은 팀: SameTeamPartClasses에서 랜덤 선택
 * - 다른 팀: DifferentTeamPartClasses에서 랜덤 선택
 * - 배열이 비어있으면 LocalPlayerPartClass를 Fallback으로 사용
 * - Mutable 파라미터 랜덤화 지원 (파트 타입별 개별 설정 가능)
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

	//~ Mutable 랜덤화 옵션 (파트 타입별)
	
	/** 로컬 플레이어용 Mutable 랜덤 옵션 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestPlay|Mutable")
	FRandomPartOptions LocalPlayerRandomOptions;

	/** 같은 팀용 Mutable 랜덤 옵션 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestPlay|Mutable")
	FRandomPartOptions SameTeamRandomOptions;

	/** 다른 팀용 Mutable 랜덤 옵션 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TestPlay|Mutable")
	FRandomPartOptions DifferentTeamRandomOptions;

private:
	/** Pawn 소유 변경 시 호출 */
	UFUNCTION()
	void OnPossessedPawnChangedForTeam(APawn* OldPawn, APawn* NewPawn);

	/** 팀에 따라 적절한 파트 클래스를 선택하여 추가 */
	void ApplyCharacterPartByTeam(APawn* OwningPawn);

	/** PawnComponent의 파트 변경 델리게이트 핸들러 */
	UFUNCTION()
	void OnCharacterPartsSpawned(ULyraPawnComponent_CharacterParts* PawnCustomizer);

	/** 스폰된 액터의 Mutable 파라미터를 랜덤화 */
	void ApplyRandomMutableParameters(AActor* SpawnedPartActor, const FRandomPartOptions& Options);

	/** 파라미터 이름이 화이트리스트에 있는지 확인 */
	bool IsParameterInWhitelist(const FString& ParamName, const FRandomPartOptions& Options) const;

	/** PawnCustomizer 취득 (로컬 구현) */
	ULyraPawnComponent_CharacterParts* GetPawnCustomizerLocal() const;

	/** 파트가 이미 적용되었는지 여부 */
	bool bPartApplied = false;

	/** 현재 적용할 랜덤 옵션 (파트 추가 시 설정됨) */
	FRandomPartOptions CurrentRandomOptions;

	/** 델리게이트 바인딩 여부 */
	bool bDelegateBound = false;
};
