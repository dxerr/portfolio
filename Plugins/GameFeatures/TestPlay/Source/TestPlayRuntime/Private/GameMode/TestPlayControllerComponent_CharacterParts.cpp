// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameMode/TestPlayControllerComponent_CharacterParts.h"

#include "Cosmetics/LyraCharacterPartTypes.h"
#include "Cosmetics/LyraPawnComponent_CharacterParts.h"
#include "Teams/LyraTeamSubsystem.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/ChildActorComponent.h"

// Mutable 관련 Include
#include "MuCO/CustomizableSkeletalComponent.h"
#include "MuCO/CustomizableObjectInstance.h"
#include "MuCO/CustomizableObject.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TestPlayControllerComponent_CharacterParts)

DEFINE_LOG_CATEGORY_STATIC(LogTestPlayCharacterParts, Log, All);

UTestPlayControllerComponent_CharacterParts::UTestPlayControllerComponent_CharacterParts(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UTestPlayControllerComponent_CharacterParts::BeginPlay()
{
	Super::BeginPlay();

	// 서버에서만 파트 적용 (Authority 체크)
	if (!HasAuthority())
	{
		return;
	}

	AController* OwningController = GetController<AController>();
	if (!OwningController)
	{
		return;
	}

	// 팀 기반 파트 적용을 위한 델리게이트 바인딩
	OwningController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::OnPossessedPawnChangedForTeam);

	// 이미 Pawn이 있으면 즉시 적용
	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		OnPossessedPawnChangedForTeam(nullptr, ControlledPawn);
	}
}

void UTestPlayControllerComponent_CharacterParts::OnPossessedPawnChangedForTeam(APawn* OldPawn, APawn* NewPawn)
{
	if (!bPartApplied && NewPawn)
	{
		ApplyCharacterPartByTeam(NewPawn);
	}
}

//////////////////////////////////////////////////////////////////////////
// Mutable 랜덤화 함수 구현

ULyraPawnComponent_CharacterParts* UTestPlayControllerComponent_CharacterParts::GetPawnCustomizerLocal() const
{
	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		return ControlledPawn->FindComponentByClass<ULyraPawnComponent_CharacterParts>();
	}
	return nullptr;
}

void UTestPlayControllerComponent_CharacterParts::OnCharacterPartsSpawned(ULyraPawnComponent_CharacterParts* PawnCustomizer)
{
	if (!PawnCustomizer || !CurrentRandomOptions.bEnableRandomization)
	{
		return;
	}

	// PawnCustomizer의 Owner(Pawn)에서 ChildActorComponent를 탐색
	APawn* OwnerPawn = Cast<APawn>(PawnCustomizer->GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	TArray<UChildActorComponent*> ChildActorComponents;
	OwnerPawn->GetComponents<UChildActorComponent>(ChildActorComponents);

	for (UChildActorComponent* ChildActorComp : ChildActorComponents)
	{
		if (ChildActorComp)
		{
			if (AActor* PartActor = ChildActorComp->GetChildActor())
			{
				ApplyRandomMutableParameters(PartActor, CurrentRandomOptions);
			}
		}
	}
}

void UTestPlayControllerComponent_CharacterParts::ApplyRandomMutableParameters(AActor* SpawnedPartActor, const FRandomPartOptions& Options)
{
	if (!Options.bEnableRandomization || !SpawnedPartActor)
	{
		return;
	}

	// UCustomizableSkeletalComponent 찾기
	UCustomizableSkeletalComponent* MutableComp = SpawnedPartActor->FindComponentByClass<UCustomizableSkeletalComponent>();
	if (!MutableComp)
	{
		return;
	}

	UCustomizableObjectInstance* OriginalInstance = MutableComp->GetCustomizableObjectInstance();
	if (!OriginalInstance)
	{
		return;
	}

	// 공유된 Instance를 Clone하여 독립적인 인스턴스 생성
	// Clone()은 transient 인스턴스를 생성하므로 각 액터가 독립적인 파라미터를 가짐
	UCustomizableObjectInstance* ClonedInstance = OriginalInstance->Clone();
	if (!ClonedInstance)
	{
		UE_LOG(LogTestPlayCharacterParts, Warning, TEXT("[%s] Instance Clone 실패"), *SpawnedPartActor->GetName());
		return;
	}

	// 새 인스턴스를 컴포넌트에 설정 (GC에 안전한 setter 사용)
	MutableComp->SetCustomizableObjectInstance(ClonedInstance);

	UCustomizableObject* CO = ClonedInstance->GetCustomizableObject();
	if (!CO)
	{
		return;
	}

	// CO가 컴파일되지 않은 경우 스킵
	if (!CO->IsCompiled())
	{
		UE_LOG(LogTestPlayCharacterParts, Warning, TEXT("[%s] CO '%s' not compiled - randomization skipped"), 
			*SpawnedPartActor->GetName(), *CO->GetName());
		return;
	}

	const int32 ParamCount = CO->GetParameterCount();
	if (ParamCount == 0)
	{
		return;
	}

	UE_LOG(LogTestPlayCharacterParts, Log, TEXT("Mutable 랜덤화 적용 - Actor: %s, CO: %s, Params: %d"), 
		*SpawnedPartActor->GetName(), *CO->GetName(), ParamCount);

	// 파라미터 순회 및 랜덤화
	for (int32 i = 0; i < ParamCount; ++i)
	{
		const FString ParamName = CO->GetParameterName(i);
		const EMutableParameterType ParamType = CO->GetParameterTypeByName(ParamName);

		// 타입별 필터링: Color는 ColorPalette가 있으면 자동 허용, 나머지는 화이트리스트로
		bool bShouldProcess = false;
		if (ParamType == EMutableParameterType::Color)
		{
			bShouldProcess = (Options.ColorPalette.Num() > 0) || IsParameterInWhitelist(ParamName, Options);
		}
		else
		{
			bShouldProcess = IsParameterInWhitelist(ParamName, Options);
		}

		if (!bShouldProcess)
		{
			continue;
		}

		// 타입별 랜덤화 처리
		switch (ParamType)
		{
		case EMutableParameterType::Int:
			{
				const int32 OptionCount = CO->GetEnumParameterNumValues(ParamName);
				if (OptionCount > 0)
				{
					const int32 RandIdx = FMath::RandRange(0, OptionCount - 1);
					const FString& OptionName = CO->GetEnumParameterValue(ParamName, RandIdx);
					ClonedInstance->SetIntParameterSelectedOption(ParamName, OptionName);
					UE_LOG(LogTestPlayCharacterParts, Log, TEXT("  [Int] %s = %s"), *ParamName, *OptionName);
				}
			}
			break;

		case EMutableParameterType::Bool:
			{
				const bool bRandValue = FMath::RandBool();
				ClonedInstance->SetBoolParameterSelectedOption(ParamName, bRandValue);
				UE_LOG(LogTestPlayCharacterParts, Log, TEXT("  [Bool] %s = %s"), *ParamName, bRandValue ? TEXT("true") : TEXT("false"));
			}
			break;

		case EMutableParameterType::Color:
			{
				FLinearColor RandColor;
				if (Options.ColorPalette.Num() > 0)
				{
					RandColor = Options.ColorPalette[FMath::RandRange(0, Options.ColorPalette.Num() - 1)];
				}
				else
				{
					RandColor = FLinearColor::MakeRandomColor();
				}
				ClonedInstance->SetColorParameterSelectedOption(ParamName, RandColor);
				UE_LOG(LogTestPlayCharacterParts, Log, TEXT("  [Color] %s = (%.2f, %.2f, %.2f)"), 
					*ParamName, RandColor.R, RandColor.G, RandColor.B);
			}
			break;

		default:
			// Float, Vector, Projector 등은 필요시 추가 구현
			break;
		}
	}

	// 변경 적용 (비동기)
	MutableComp->UpdateSkeletalMeshAsync();
}

bool UTestPlayControllerComponent_CharacterParts::IsParameterInWhitelist(const FString& ParamName, const FRandomPartOptions& Options) const
{
	// 화이트리스트가 비어있으면 전체 허용
	if (Options.WhitelistParameterNames.Num() == 0)
	{
		return true;
	}
	return Options.WhitelistParameterNames.Contains(ParamName);
}

//////////////////////////////////////////////////////////////////////////
// 팀 기반 파트 적용 로직

void UTestPlayControllerComponent_CharacterParts::ApplyCharacterPartByTeam(APawn* OwningPawn)
{
	if (!LocalPlayerPartClass || !OwningPawn)
	{
		UE_LOG(LogTestPlayCharacterParts, Warning, TEXT("ApplyCharacterPartByTeam - LocalPlayerPartClass or OwningPawn is null"));
		return;
	}

	// 헬퍼 람다: 파트 추가 및 랜덤화 델리게이트 바인딩
	auto AddPartFromClass = [this](TSubclassOf<AActor> PartClass, const FRandomPartOptions& RandomOpts)
	{
		if (!PartClass)
		{
			return;
		}

		// 랜덤 옵션 저장 (델리게이트 콜백에서 사용)
		CurrentRandomOptions = RandomOpts;

		// 랜덤화가 필요하면 델리게이트 바인딩 (한 번만)
		if (!bDelegateBound && RandomOpts.bEnableRandomization)
		{
			if (ULyraPawnComponent_CharacterParts* PawnCustomizer = GetPawnCustomizerLocal())
			{
				PawnCustomizer->OnCharacterPartsChanged.AddDynamic(this, &ThisClass::OnCharacterPartsSpawned);
				bDelegateBound = true;
			}
		}

		// 파트 추가
		FLyraCharacterPart NewPart;
		NewPart.PartClass = PartClass;
		AddCharacterPart(NewPart);
		bPartApplied = true;

		UE_LOG(LogTestPlayCharacterParts, Log, TEXT("Added CharacterPart: %s (Randomization: %s)"), 
			*PartClass->GetName(), RandomOpts.bEnableRandomization ? TEXT("ON") : TEXT("OFF"));
	};

	// 로컬 플레이어 찾기
	UWorld* World = GetWorld();
	if (!World)
	{
		AddPartFromClass(LocalPlayerPartClass, LocalPlayerRandomOptions);
		return;
	}

	APlayerController* LocalPC = World->GetFirstPlayerController();
	APawn* LocalPlayerPawn = LocalPC ? LocalPC->GetPawn() : nullptr;

	// 로컬 플레이어인 경우
	if (OwningPawn == LocalPlayerPawn)
	{
		AddPartFromClass(LocalPlayerPartClass, LocalPlayerRandomOptions);
		return;
	}

	// 팀 비교
	ULyraTeamSubsystem* TeamSubsystem = World->GetSubsystem<ULyraTeamSubsystem>();
	if (!TeamSubsystem || !LocalPlayerPawn)
	{
		AddPartFromClass(LocalPlayerPartClass, LocalPlayerRandomOptions);
		return;
	}

	int32 OwningTeamId = INDEX_NONE;
	int32 LocalTeamId = INDEX_NONE;
	const ELyraTeamComparison TeamComparison = TeamSubsystem->CompareTeams(OwningPawn, LocalPlayerPawn, OwningTeamId, LocalTeamId);

	// 팀에 따라 파트 선택
	const TArray<TSubclassOf<AActor>>* PartClassesArray = nullptr;
	const FRandomPartOptions* TeamRandomOptions = nullptr;
	
	if (TeamComparison == ELyraTeamComparison::OnSameTeam)
	{
		PartClassesArray = &SameTeamPartClasses;
		TeamRandomOptions = &SameTeamRandomOptions;
	}
	else if (TeamComparison == ELyraTeamComparison::DifferentTeams)
	{
		PartClassesArray = &DifferentTeamPartClasses;
		TeamRandomOptions = &DifferentTeamRandomOptions;
	}
	else
	{
		// InvalidArgument 등의 경우 로컬 플레이어 파트 사용
		AddPartFromClass(LocalPlayerPartClass, LocalPlayerRandomOptions);
		return;
	}

	// 배열에서 랜덤 선택
	if (PartClassesArray && PartClassesArray->Num() > 0)
	{
		const int32 RandomIndex = FMath::RandRange(0, PartClassesArray->Num() - 1);
		TSubclassOf<AActor> SelectedClass = (*PartClassesArray)[RandomIndex];
		
		if (SelectedClass)
		{
			AddPartFromClass(SelectedClass, *TeamRandomOptions);
		}
		else
		{
			AddPartFromClass(LocalPlayerPartClass, LocalPlayerRandomOptions);
		}
	}
	else
	{
		// Fallback: 배열이 비어있는 경우
		AddPartFromClass(LocalPlayerPartClass, *TeamRandomOptions);
	}
}
