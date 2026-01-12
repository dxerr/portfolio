// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameMode/TestPlayBotCreationComponent.h"
#include "GameModes/LyraGameMode.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/LyraExperienceManagerComponent.h"
#include "Development/LyraDeveloperSettings.h"
#include "Character/LyraPawnExtensionComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Character/LyraHealthComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/PlayerStart.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TestPlayBotCreationComponent)

UTestPlayBotCreationComponent::UTestPlayBotCreationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTestPlayBotCreationComponent::BeginPlay()
{
	Super::BeginPlay();

	// Listen for the experience load to complete
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	ULyraExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<ULyraExperienceManagerComponent>();
	check(ExperienceComponent);
	ExperienceComponent->CallOrRegister_OnExperienceLoaded_LowPriority(FOnLyraExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
}

void UTestPlayBotCreationComponent::OnExperienceLoaded(const ULyraExperienceDefinition* Experience)
{
#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		ServerCreateBots();
	}
#endif
}

#if WITH_SERVER_CODE

void UTestPlayBotCreationComponent::ServerCreateBots_Implementation()
{
	if (BotControllerClass == nullptr)
	{
		return;
	}

	RemainingBotNames = RandomBotNames;

	// Determine how many bots to spawn
	int32 EffectiveBotCount = NumBotsToCreate;

	// Give the developer settings a chance to override it
	if (GIsEditor)
	{
		const ULyraDeveloperSettings* DeveloperSettings = GetDefault<ULyraDeveloperSettings>();
		
		if (DeveloperSettings->bOverrideBotCount)
		{
			EffectiveBotCount = DeveloperSettings->OverrideNumPlayerBotsToSpawn;
		}
	}

	// Give the URL a chance to override it
	if (AGameModeBase* GameModeBase = GetGameMode<AGameModeBase>())
	{
		EffectiveBotCount = UGameplayStatics::GetIntOption(GameModeBase->OptionsString, TEXT("NumBots"), EffectiveBotCount);
	}

	// Create them
	for (int32 Count = 0; Count < EffectiveBotCount; ++Count)
	{
		SpawnOneBot();
	}
}

FString UTestPlayBotCreationComponent::CreateBotName(int32 PlayerIndex)
{
	FString Result;
	if (RemainingBotNames.Num() > 0)
	{
		const int32 NameIndex = FMath::RandRange(0, RemainingBotNames.Num() - 1);
		Result = RemainingBotNames[NameIndex];
		RemainingBotNames.RemoveAtSwap(NameIndex);
	}
	else
	{
		//@TODO: PlayerId is only being initialized for players right now
		PlayerIndex = FMath::RandRange(260, 260+100);
		Result = FString::Printf(TEXT("Bot %d"), PlayerIndex);
	}
	return Result;
}

FVector UTestPlayBotCreationComponent::GetReferenceLocationForValidation() const
{
	// 1순위: 첫 번째 플레이어 Pawn 위치
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (APawn* PlayerPawn = PC->GetPawn())
				{
					return PlayerPawn->GetActorLocation();
				}
			}
		}
	}

	// 2순위: 첫 번째 PlayerStart 위치
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	if (PlayerStarts.Num() > 0)
	{
		return PlayerStarts[0]->GetActorLocation();
	}

	// 3순위: 맵 중심 (폴백)
	return FVector::ZeroVector;
}

bool UTestPlayBotCreationComponent::IsLocationReachableFromReference(const FVector& TestLocation) const
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		// 네비게이션 시스템이 없으면 검증 스킵 (true 반환)
		return true;
	}

	FVector ReferenceLocation = GetReferenceLocationForValidation();
	
	// 기준 위치가 유효하지 않으면 검증 스킵
	if (ReferenceLocation.IsZero())
	{
		return true;
	}

	// 기준 위치를 NavMesh에 투영 (PlayerStart가 NavMesh에 정확히 위치하지 않을 수 있음)
	FNavLocation ProjectedReference;
	if (!NavSys->ProjectPointToNavigation(ReferenceLocation, ProjectedReference, FVector(500.f, 500.f, 500.f)))
	{
		// 기준 위치 자체가 NavMesh에 투영되지 않으면 검증 스킵
		return true;
	}

	// 투영된 기준 위치에서 테스트 위치까지 경로 찾기
	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(GetWorld(), ProjectedReference.Location, TestLocation);
	
	// 경로가 유효하면 OK (부분 경로라도 도달 가능성이 있으면 허용)
	// IsPartial() 검사 제거 - 완전히 다른 NavMesh 영역일 때만 IsValid()가 false
	if (NavPath && NavPath->IsValid())
	{
		// 추가 검증: 경로 길이가 0이면 (시작점과 끝점이 같은 영역에 없음) 거부
		if (NavPath->GetPathLength() > 0.0f)
		{
			return true;
		}
	}

	return false;
}

void UTestPlayBotCreationComponent::SpawnOneBot()
{
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnInfo.OverrideLevel = GetComponentLevel();
	SpawnInfo.ObjectFlags |= RF_Transient;
	AAIController* NewController = GetWorld()->SpawnActor<AAIController>(BotControllerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);

	if (NewController != nullptr)
	{
		ALyraGameMode* GameMode = GetGameMode<ALyraGameMode>();
		check(GameMode);

		if (NewController->PlayerState != nullptr)
		{
			NewController->PlayerState->SetPlayerName(CreateBotName(NewController->PlayerState->GetPlayerId()));
		}

		GameMode->GenericPlayerInitialization(NewController);
		GameMode->RestartPlayer(NewController);

		if (NewController->GetPawn() != nullptr)
		{
			bool bTeleported = false;
			FVector SpawnLocation = FVector::ZeroVector;
			
			// Get map bounds for reference
			FBox MapBounds(ForceInit);
			UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
			
			if (NavSys && NavSys->GetDefaultNavDataInstance())
			{
				MapBounds = NavSys->GetDefaultNavDataInstance()->GetBounds();
			}
			else
			{
				// Calculate map bounds from all static mesh actors if NavSystem is unavailable
				TArray<AActor*> AllActors;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), AllActors);
				
				for (AActor* Actor : AllActors)
				{
					if (Actor && Actor->IsRootComponentStatic())
					{
						MapBounds += Actor->GetComponentsBoundingBox(true);
					}
				}
				
				// Fallback: use a default reasonable bounds if nothing found
				if (!MapBounds.IsValid || MapBounds.GetVolume() < 1.0f)
				{
					MapBounds = FBox(FVector(-10000, -10000, 0), FVector(10000, 10000, 1000));
				}
			}

			// Try to find a valid navigable location
			if (NavSys)
			{
				FNavLocation RandomLocation;
				bool bFoundValidLocation = false;
				
				for (int32 i = 0; i < 30; ++i)
				{
					ANavigationData* NavData = NavSys->GetDefaultNavDataInstance();
					bool bResult = NavSys->GetRandomPoint(RandomLocation, NavData);

					if (!bResult && NavData)
					{
						// Use map bounds center as reference point
						FVector CenterPoint = MapBounds.GetCenter();
						FVector RandomOffset = FVector(
							FMath::RandRange(-MapBounds.GetExtent().X, MapBounds.GetExtent().X),
							FMath::RandRange(-MapBounds.GetExtent().Y, MapBounds.GetExtent().Y),
							FMath::RandRange(-MapBounds.GetExtent().Z * 0.5f, MapBounds.GetExtent().Z * 0.5f)
						);
						FVector ManualRandomPos = CenterPoint + RandomOffset;
						
						bResult = NavSys->ProjectPointToNavigation(ManualRandomPos, RandomLocation, 
							FVector(1000.f, 1000.f, 2000.f), NavData);
					}

					if (bResult)
					{
						bool bTooClose = false;
						for (AAIController* ExistingBot : SpawnedBotList)
						{
							if (ExistingBot && ExistingBot->GetPawn())
							{
								float DistSq = FVector::DistSquared(ExistingBot->GetPawn()->GetActorLocation(), RandomLocation.Location);
								if (DistSq < MinDistBetweenBots * MinDistBetweenBots)
								{
									bTooClose = true;
									break;
								}
							}
						}

						if (!bTooClose)
						{
							// 갇힌 공간 검증: 기준 위치로부터 도달 가능한지 확인
							if (!IsLocationReachableFromReference(RandomLocation.Location))
							{
								continue; // 도달 불가능하면 재시도
							}

							bFoundValidLocation = true;
							SpawnLocation = RandomLocation.Location;
							break;
						}
					}
				}

				// Fallback 1: Use map center with ground trace
				if (!bFoundValidLocation)
				{
					FVector CenterPoint = MapBounds.GetCenter();
					FVector RandomOffset = FVector(
						FMath::RandRange(-MapBounds.GetExtent().X * 0.8f, MapBounds.GetExtent().X * 0.8f),
						FMath::RandRange(-MapBounds.GetExtent().Y * 0.8f, MapBounds.GetExtent().Y * 0.8f),
						0
					);
					
					FVector TestLocation = CenterPoint + RandomOffset;
					FVector TraceStart = TestLocation + FVector(0.f, 0.f, MapBounds.GetExtent().Z);
					FVector TraceEnd = TestLocation - FVector(0.f, 0.f, MapBounds.GetExtent().Z);
					
					FHitResult HitResult;
					if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic))
					{
						SpawnLocation = HitResult.Location;
						bFoundValidLocation = true;
					}
				}
				
				if (bFoundValidLocation)
				{
					FRotator RandomRotation(0, FMath::RandRange(0.0f, 360.0f), 0);
					NewController->GetPawn()->TeleportTo(SpawnLocation + FVector(0, 0, 100.0f), RandomRotation);
					bTeleported = true;
				}
			}

			// Fallback 2: Use PlayerStarts if navigation completely failed
			if (!bTeleported)
			{
				TArray<AActor*> PlayerStarts;
				UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);

				if (PlayerStarts.Num() > 0)
				{
					int32 RandomIndex = FMath::RandRange(0, PlayerStarts.Num() - 1);
					AActor* SelectedStart = PlayerStarts[RandomIndex];
					
					if (SelectedStart)
					{
						// Add random offset from player start
						FVector Offset = FVector(
							FMath::RandRange(-500.0f, 500.0f),
							FMath::RandRange(-500.0f, 500.0f),
							0.0f
						);
						
						SpawnLocation = SelectedStart->GetActorLocation() + Offset;
						
						// Trace down to find ground
						FVector TraceStart = SpawnLocation + FVector(0.f, 0.f, 500.f);
						FVector TraceEnd = SpawnLocation - FVector(0.f, 0.f, 1000.f);
						FHitResult HitResult;
						
						if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic))
						{
							SpawnLocation = HitResult.Location;
						}
						else
						{
							SpawnLocation = SelectedStart->GetActorLocation();
						}
						
						NewController->GetPawn()->TeleportTo(SpawnLocation + FVector(0, 0, 100.0f), SelectedStart->GetActorRotation());
						bTeleported = true;
					}
				}
			}

			// Final fallback: Use map center
			if (!bTeleported)
			{
				SpawnLocation = MapBounds.GetCenter();
				FVector TraceStart = SpawnLocation + FVector(0.f, 0.f, MapBounds.GetExtent().Z);
				FVector TraceEnd = SpawnLocation - FVector(0.f, 0.f, MapBounds.GetExtent().Z);
				FHitResult HitResult;
				
				if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic))
				{
					SpawnLocation = HitResult.Location;
				}
				
				FRotator RandomRotation(0, FMath::RandRange(0.0f, 360.0f), 0);
				NewController->GetPawn()->TeleportTo(SpawnLocation + FVector(0, 0, 100.0f), RandomRotation);
			}

			if (ULyraPawnExtensionComponent* PawnExtComponent = NewController->GetPawn()->FindComponentByClass<ULyraPawnExtensionComponent>())
			{
				PawnExtComponent->CheckDefaultInitialization();
			}
		}

		SpawnedBotList.Add(NewController);
	}
}

void UTestPlayBotCreationComponent::RemoveOneBot()
{
	if (SpawnedBotList.Num() > 0)
	{
		const int32 BotToRemoveIndex = FMath::RandRange(0, SpawnedBotList.Num() - 1);

		AAIController* BotToRemove = SpawnedBotList[BotToRemoveIndex];
		SpawnedBotList.RemoveAtSwap(BotToRemoveIndex);

		if (BotToRemove)
		{
			if (APawn* ControlledPawn = BotToRemove->GetPawn())
			{
				if (ULyraHealthComponent* HealthComponent = ULyraHealthComponent::FindHealthComponent(ControlledPawn))
				{
					HealthComponent->DamageSelfDestruct();
				}
				else
				{
					ControlledPawn->Destroy();
				}
			}
			BotToRemove->Destroy();
		}
	}
}

#else // !WITH_SERVER_CODE

void UTestPlayBotCreationComponent::ServerCreateBots_Implementation()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in LyraClient!"));
}

void UTestPlayBotCreationComponent::SpawnOneBot()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in LyraClient!"));
}

void UTestPlayBotCreationComponent::RemoveOneBot()
{
	ensureMsgf(0, TEXT("Bot functions do not exist in LyraClient!"));
}

#endif
