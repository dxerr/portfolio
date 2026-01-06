// Copyright Epic Games, Inc. All Rights Reserved.

#include "Teams/TestPlayTeamCreationComponent.h"
#include "GameModes/LyraExperienceManagerComponent.h"
#include "Player/LyraPlayerState.h"
#include "Engine/World.h"
#include "GameModes/LyraGameMode.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TestPlayTeamCreationComponent)

UTestPlayTeamCreationComponent::UTestPlayTeamCreationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

#if WITH_SERVER_CODE
void UTestPlayTeamCreationComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for the experience load to complete
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	ULyraExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<ULyraExperienceManagerComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded_HighPriority(FOnLyraExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void UTestPlayTeamCreationComponent::OnExperienceLoaded(const ULyraExperienceDefinition* Experience)
{
	if (HasAuthority())
	{
		ServerCreateTeams();
		ServerAssignPlayersToTeams();
	}
}

void UTestPlayTeamCreationComponent::ServerCreateTeams()
{
	for (const auto& KVP : TeamsToCreate)
	{
		const int32 TeamId = KVP.Key;
		ServerCreateTeam(TeamId, KVP.Value);
	}
}

void UTestPlayTeamCreationComponent::ServerAssignPlayersToTeams()
{
	// Assign players that already exist to teams
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (ALyraPlayerState* LyraPS = Cast<ALyraPlayerState>(PS))
		{
			ServerChooseTeamForPlayer(LyraPS);
		}
	}

	// Listen for new players logging in
	ALyraGameMode* GameMode = Cast<ALyraGameMode>(GameState->AuthorityGameMode);
	check(GameMode);

	GameMode->OnGameModePlayerInitialized.AddUObject(this, &ThisClass::OnPlayerInitialized);
}

void UTestPlayTeamCreationComponent::ServerChooseTeamForPlayer(ALyraPlayerState* PS)
{
	if (PS->IsOnlyASpectator())
	{
		PS->SetGenericTeamId(FGenericTeamId::NoTeam);
	}
	else
	{
		// 봇인지 확인
		if (PS->IsABot())
		{
			// 설정된 확률에 따라 아군/적군 배정
			const float RandomValue = FMath::RandRange(0.0f, 100.0f);
			if (RandomValue <= FriendlyBotSpawnPercent)
			{
				PS->SetGenericTeamId(FGenericTeamId(FriendlyTeamID));
			}
			else
			{
				PS->SetGenericTeamId(FGenericTeamId(EnemyTeamID));
			}
		}
		else
		{
			// 사람은 무조건 아군 팀(FriendlyTeamID)으로 배정
			// (그래야 봇 비율 설정의 기준이 되는 '나'의 위치가 확정됨)
			PS->SetGenericTeamId(FGenericTeamId(FriendlyTeamID));
		}
	}
}

void UTestPlayTeamCreationComponent::OnPlayerInitialized(AGameModeBase* GameMode, AController* NewPlayer)
{
	check(NewPlayer);
	check(NewPlayer->PlayerState);
	if (ALyraPlayerState* LyraPS = Cast<ALyraPlayerState>(NewPlayer->PlayerState))
	{
		ServerChooseTeamForPlayer(LyraPS);
	}
}

void UTestPlayTeamCreationComponent::ServerCreateTeam(int32 TeamId, UDataAsset* DisplayAsset)
{
	check(HasAuthority());

	UWorld* World = GetWorld();
	check(World);

	// Reflection을 사용하여 ALyraTeamPublicInfo 및 PrivateInfo 생성 및 값 설정
	// SpawnActorDeferred를 사용하여 BeginPlay 호출 전에 TeamId를 설정해야 함
	if (PublicTeamInfoClass)
	{
		AActor* NewTeamPublicInfo = World->SpawnActorDeferred<AActor>(PublicTeamInfoClass, FTransform::Identity, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (NewTeamPublicInfo)
		{
			// SetTeamId (int32) - Protected property in base class
			if (FProperty* TeamIdProp = NewTeamPublicInfo->GetClass()->FindPropertyByName(TEXT("TeamId")))
			{
				if (FIntProperty* IntProp = CastField<FIntProperty>(TeamIdProp))
				{
					IntProp->SetPropertyValue_InContainer(NewTeamPublicInfo, TeamId);
				}
			}

			// SetTeamDisplayAsset (TObjectPtr<ULyraTeamDisplayAsset>) - Private property
			if (DisplayAsset)
			{
				if (FProperty* DisplayAssetProp = NewTeamPublicInfo->GetClass()->FindPropertyByName(TEXT("TeamDisplayAsset")))
				{
					if (FObjectProperty* ObjProp = CastField<FObjectProperty>(DisplayAssetProp))
					{
						ObjProp->SetObjectPropertyValue_InContainer(NewTeamPublicInfo, DisplayAsset);
					}
				}
			}

			// Finish Spawning (Calls BeginPlay -> Registers with Subsystem)
			UGameplayStatics::FinishSpawningActor(NewTeamPublicInfo, FTransform::Identity);
		}
	}

	if (PrivateTeamInfoClass)
	{
		AActor* NewTeamPrivateInfo = World->SpawnActorDeferred<AActor>(PrivateTeamInfoClass, FTransform::Identity, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (NewTeamPrivateInfo)
		{
			// SetTeamId (int32)
			if (FProperty* TeamIdProp = NewTeamPrivateInfo->GetClass()->FindPropertyByName(TEXT("TeamId")))
			{
				if (FIntProperty* IntProp = CastField<FIntProperty>(TeamIdProp))
				{
					IntProp->SetPropertyValue_InContainer(NewTeamPrivateInfo, TeamId);
				}
			}

			// Finish Spawning
			UGameplayStatics::FinishSpawningActor(NewTeamPrivateInfo, FTransform::Identity);
		}
	}
}

#endif // WITH_SERVER_CODE
