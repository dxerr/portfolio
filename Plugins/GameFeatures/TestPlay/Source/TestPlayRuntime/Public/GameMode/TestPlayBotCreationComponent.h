// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"
#include "TestPlayBotCreationComponent.generated.h"

class ULyraExperienceDefinition;
class ULyraPawnData;
class AAIController;

/**
 * UTestPlayBotCreationComponent
 * 
 * An improved bot creation component that handles spawning bots at random locations
 * while ensuring they don't overlap with each other.
 * 
 * Logic adapted from ULyraBotCreationComponent but with location randomization.
 */
UCLASS(Blueprintable, Abstract)
class UTestPlayBotCreationComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UTestPlayBotCreationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void BeginPlay() override;
	//~End of UActorComponent interface

private:
	void OnExperienceLoaded(const ULyraExperienceDefinition* Experience);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Gameplay)
	int32 NumBotsToCreate = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Gameplay)
	TSubclassOf<AAIController> BotControllerClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Gameplay)
	TArray<FString> RandomBotNames;

	// Minimum distance between spawned bots to avoid overlap
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Gameplay)
	float MinDistBetweenBots = 200.0f;

	TArray<FString> RemainingBotNames;

protected:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AAIController>> SpawnedBotList;

	/** 주어진 위치에서 기준 위치(PlayerStart 또는 플레이어)로 네비게이션 경로가 존재하는지 검증 */
	bool IsLocationReachableFromReference(const FVector& TestLocation) const;

	/** 유효한 기준 위치(PlayerStart 또는 첫 번째 플레이어 위치) 반환 */
	FVector GetReferenceLocationForValidation() const;

	/** Always creates a single bot */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Gameplay)
	virtual void SpawnOneBot();

	/** Deletes the last created bot if possible */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Gameplay)
	virtual void RemoveOneBot();

	/** Spawns bots up to NumBotsToCreate */
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category=Gameplay)
	void ServerCreateBots();

#if WITH_SERVER_CODE
public:
	void Cheat_AddBot() { SpawnOneBot(); }
	void Cheat_RemoveBot() { RemoveOneBot(); }

	FString CreateBotName(int32 PlayerIndex);
#endif
};
