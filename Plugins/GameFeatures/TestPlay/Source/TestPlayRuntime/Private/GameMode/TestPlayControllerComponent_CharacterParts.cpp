// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameMode/TestPlayControllerComponent_CharacterParts.h"

#include "Cosmetics/LyraCharacterPartTypes.h"
#include "Teams/LyraTeamSubsystem.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TestPlayControllerComponent_CharacterParts)

DEFINE_LOG_CATEGORY_STATIC(LogTestPlayCharacterParts, Log, All);

UTestPlayControllerComponent_CharacterParts::UTestPlayControllerComponent_CharacterParts(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTestPlayControllerComponent_CharacterParts::BeginPlay()
{
	// 부모 BeginPlay 먼저 호출
	Super::BeginPlay();

	// Pawn 소유 변경 이벤트 추가 바인딩
	if (HasAuthority())
	{
		if (AController* OwningController = GetController<AController>())
		{
			// 추가 델리게이트 바인딩 (팀 기반 파트 적용용)
			OwningController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessedPawnChangedForTeam);

			// 이미 Pawn이 있으면 즉시 적용
			if (APawn* ControlledPawn = GetPawn<APawn>())
			{
				OnPossessedPawnChangedForTeam(nullptr, ControlledPawn);
			}
		}
	}
}

void UTestPlayControllerComponent_CharacterParts::OnPossessedPawnChangedForTeam(APawn* OldPawn, APawn* NewPawn)
{
	if (!bPartApplied && NewPawn)
	{
		ApplyCharacterPartByTeam(NewPawn);
	}
}

void UTestPlayControllerComponent_CharacterParts::ApplyCharacterPartByTeam(APawn* OwningPawn)
{
	// 유효성 검사
	if (!LocalPlayerPartClass)
	{
		UE_LOG(LogTestPlayCharacterParts, Warning, TEXT("LocalPlayerPartClass is not set!"));
		return;
	}

	if (!OwningPawn)
	{
		UE_LOG(LogTestPlayCharacterParts, Warning, TEXT("OwningPawn is null!"));
		return;
	}

	UE_LOG(LogTestPlayCharacterParts, Log, TEXT("ApplyCharacterPartByTeam - OwningPawn: %s"), *OwningPawn->GetName());

	// 헬퍼 람다: 클래스를 FLyraCharacterPart로 변환하여 추가
	auto AddPartFromClass = [this](TSubclassOf<AActor> PartClass, const FString& Reason)
	{
		if (PartClass)
		{
			FLyraCharacterPart NewPart;
			NewPart.PartClass = PartClass;
			AddCharacterPart(NewPart);
			bPartApplied = true;
			UE_LOG(LogTestPlayCharacterParts, Log, TEXT("Added CharacterPart: %s (Reason: %s)"), 
				*PartClass->GetName(), *Reason);
		}
	};

	// 로컬 플레이어 찾기 (팀 비교 기준)
	UWorld* World = GetWorld();
	if (!World)
	{
		AddPartFromClass(LocalPlayerPartClass, TEXT("No World"));
		return;
	}

	APlayerController* LocalPC = World->GetFirstPlayerController();
	APawn* LocalPlayerPawn = LocalPC ? LocalPC->GetPawn() : nullptr;

	UE_LOG(LogTestPlayCharacterParts, Log, TEXT("LocalPlayerPawn: %s"), 
		LocalPlayerPawn ? *LocalPlayerPawn->GetName() : TEXT("NULL"));

	// 로컬 플레이어인 경우
	if (OwningPawn == LocalPlayerPawn)
	{
		AddPartFromClass(LocalPlayerPartClass, TEXT("Is LocalPlayer"));
		return;
	}

	// 팀 비교
	ULyraTeamSubsystem* TeamSubsystem = World->GetSubsystem<ULyraTeamSubsystem>();
	if (!TeamSubsystem)
	{
		UE_LOG(LogTestPlayCharacterParts, Warning, TEXT("TeamSubsystem is null"));
		AddPartFromClass(LocalPlayerPartClass, TEXT("No TeamSubsystem"));
		return;
	}

	if (!LocalPlayerPawn)
	{
		UE_LOG(LogTestPlayCharacterParts, Warning, TEXT("LocalPlayerPawn is null"));
		AddPartFromClass(LocalPlayerPartClass, TEXT("No LocalPlayerPawn"));
		return;
	}

	int32 OwningTeamId = INDEX_NONE;
	int32 LocalTeamId = INDEX_NONE;
	ELyraTeamComparison TeamComparison = TeamSubsystem->CompareTeams(OwningPawn, LocalPlayerPawn, OwningTeamId, LocalTeamId);

	UE_LOG(LogTestPlayCharacterParts, Log, TEXT("TeamComparison: %d (OwningTeamId: %d, LocalTeamId: %d)"), 
		(int32)TeamComparison, OwningTeamId, LocalTeamId);

	// 팀에 따라 파트 클래스 배열 선택
	const TArray<TSubclassOf<AActor>>* PartClassesArray = nullptr;
	FString TeamReason;
	
	if (TeamComparison == ELyraTeamComparison::OnSameTeam)
	{
		PartClassesArray = &SameTeamPartClasses;
		TeamReason = TEXT("SameTeam");
	}
	else if (TeamComparison == ELyraTeamComparison::DifferentTeams)
	{
		PartClassesArray = &DifferentTeamPartClasses;
		TeamReason = TEXT("DifferentTeam");
	}
	else
	{
		TeamReason = TEXT("InvalidArgument");
	}

	UE_LOG(LogTestPlayCharacterParts, Log, TEXT("TeamReason: %s, ArraySize: %d"), 
		*TeamReason, PartClassesArray ? PartClassesArray->Num() : -1);

	// 배열에서 랜덤 선택 또는 Fallback
	if (PartClassesArray && PartClassesArray->Num() > 0)
	{
		int32 RandomIndex = FMath::RandRange(0, PartClassesArray->Num() - 1);
		TSubclassOf<AActor> SelectedClass = (*PartClassesArray)[RandomIndex];
		
		if (SelectedClass)
		{
			AddPartFromClass(SelectedClass, FString::Printf(TEXT("%s[%d]"), *TeamReason, RandomIndex));
		}
		else
		{
			AddPartFromClass(LocalPlayerPartClass, TEXT("SelectedClass is null"));
		}
	}
	else
	{
		// Fallback: 배열이 비어있거나 팀 비교 결과가 InvalidArgument인 경우
		AddPartFromClass(LocalPlayerPartClass, FString::Printf(TEXT("Fallback (%s)"), *TeamReason));
	}
}
