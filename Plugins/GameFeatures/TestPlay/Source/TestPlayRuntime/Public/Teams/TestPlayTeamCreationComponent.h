// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/GameStateComponent.h"
#include "TestPlayTeamCreationComponent.generated.h"

class ULyraExperienceDefinition;
class ULyraTeamDisplayAsset;
class ALyraPlayerState;
class AGameModeBase;
class AController;

UCLASS(Blueprintable, Meta=(BlueprintSpawnableComponent))
class TESTPLAYRUNTIME_API UTestPlayTeamCreationComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UTestPlayTeamCreationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// List of teams to create (id to display asset mapping)
	// ULyraTeamDisplayAsset이 API로 노출되지 않을 수 있으므로 UDataAsset으로 받지만,
	// 실제로는 ULyraTeamDisplayAsset을 사용해야 합니다.
	UPROPERTY(EditDefaultsOnly, Category = Teams)
	TMap<uint8, TObjectPtr<UDataAsset>> TeamsToCreate;

	UPROPERTY(EditDefaultsOnly, Category=Teams)
	TSubclassOf<AActor> PublicTeamInfoClass;

	UPROPERTY(EditDefaultsOnly, Category=Teams)
	TSubclassOf<AActor> PrivateTeamInfoClass;

	// 아군(FriendlyTeamID) 팀에 봇이 배정될 확률 (0.0 ~ 100.0)
	// 0: 모든 봇이 적군(EnemyTeamID) - "나 vs 모두"
	// 100: 모든 봇이 아군(FriendlyTeamID) - "Team Co-op"
	UPROPERTY(EditDefaultsOnly, Category = Teams, meta=(ClampMin="0.0", ClampMax="100.0"))
	float FriendlyBotSpawnPercent = 50.0f;

	// 아군 팀 ID (기본값 1: Blue)
	UPROPERTY(EditDefaultsOnly, Category = Teams)
	int32 FriendlyTeamID = 1;

	// 적군 팀 ID (기본값 0: Red)
	UPROPERTY(EditDefaultsOnly, Category = Teams)
	int32 EnemyTeamID = 0;

#if WITH_SERVER_CODE
protected:
	//~UActorComponent interface
	virtual void BeginPlay() override;
	//~End of UActorComponent interface

	void OnExperienceLoaded(const ULyraExperienceDefinition* Experience);

	virtual void ServerCreateTeams();
	virtual void ServerAssignPlayersToTeams();
	virtual void ServerChooseTeamForPlayer(ALyraPlayerState* PS);

	void OnPlayerInitialized(AGameModeBase* GameMode, AController* NewPlayer);
	void ServerCreateTeam(int32 TeamId, UDataAsset* DisplayAsset);
#endif
};
