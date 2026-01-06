#include "AnimNotifyState_MeleeTrace.h"
#include "AdvancedMeleeTraceComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/SkeletalMeshSocket.h"

// Helper to manage checking/creating components
UAdvancedMeleeTraceComponent* GetOrSpawnTraceComponent(USkeletalMeshComponent* MeshComp, bool& bWasSpawned)
{
	bWasSpawned = false;
	if (!MeshComp || !MeshComp->GetOwner()) return nullptr;

	AActor* Owner = MeshComp->GetOwner();
	UAdvancedMeleeTraceComponent* TraceComp = Owner->FindComponentByClass<UAdvancedMeleeTraceComponent>();

#if WITH_EDITOR
	UWorld* World = MeshComp->GetWorld();
	if (World && World->IsPreviewWorld())
	{
		// 만약 FindComponentByClass가 못 찾았다면, 태그로 명시적 검색 (Transient 컴포넌트 이슈 대비)
		if (!TraceComp)
		{
			TArray<UActorComponent*> Comps;
			Owner->GetComponents(UAdvancedMeleeTraceComponent::StaticClass(), Comps);
			for (UActorComponent* C : Comps)
			{
				if (C && C->ComponentTags.Contains(TEXT("TempEditorMeleeComponent")))
				{
					TraceComp = Cast<UAdvancedMeleeTraceComponent>(C);
					break;
				}
			}
		}

		// 그래도 없으면 생성
		if (!TraceComp)
		{
			TraceComp = NewObject<UAdvancedMeleeTraceComponent>(Owner, NAME_None, RF_Transient);
			TraceComp->ComponentTags.Add(TEXT("TempEditorMeleeComponent"));
			TraceComp->RegisterComponent();
			bWasSpawned = true;
		}
	}
#endif
	return TraceComp;
}

void UAnimNotifyState_MeleeTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !MeshComp->GetOwner()) return;

#if WITH_EDITOR
	// 1. Setup Preview Weapon Mesh (if needed) - Must be done BEFORE StartTrace so sockets exist
	UWorld* World = MeshComp->GetWorld();
	if (World && World->IsPreviewWorld() && PreviewWeaponMesh && !WeaponAttachmentSocket.IsNone())
	{
		// Cleanup existing first
		TArray<USceneComponent*> Children;
		MeshComp->GetChildrenComponents(true, Children);
		for (USceneComponent* Child : Children)
		{
			if (Child && Child->ComponentTags.Contains(TEXT("MeleeTracePreviewWeapon")))
			{
				Child->DestroyComponent();
			}
		}

		USceneComponent* SpawnedComp = nullptr;
		if (UStaticMesh* SM = Cast<UStaticMesh>(PreviewWeaponMesh))
		{
			UStaticMeshComponent* SMC = NewObject<UStaticMeshComponent>(MeshComp->GetOwner(), NAME_None, RF_Transient);
			SMC->SetStaticMesh(SM);
			SpawnedComp = SMC;
		}
		else if (USkeletalMesh* SKM = Cast<USkeletalMesh>(PreviewWeaponMesh))
		{
			USkeletalMeshComponent* SKMC = NewObject<USkeletalMeshComponent>(MeshComp->GetOwner(), NAME_None, RF_Transient);
			SKMC->SetSkeletalMeshAsset(SKM);
			SpawnedComp = SKMC;
		}

		if (SpawnedComp)
		{
			SpawnedComp->ComponentTags.Add(TEXT("MeleeTracePreviewWeapon"));
			SpawnedComp->RegisterComponent();
			SpawnedComp->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachmentSocket);
		}
	}
#endif

	// 2. Get or Spawn Component and Start Trace
	bool bSpawned = false;
	UAdvancedMeleeTraceComponent* TraceComp = GetOrSpawnTraceComponent(MeshComp, bSpawned);
	if (TraceComp)
	{
		// Sync DebugDraw setting from Notify to Component
#if WITH_EDITOR
		if (World && World->IsPreviewWorld())
		{
			// Preview World logic if needed
		}
#endif
		// Always apply debug draw setting from Notify
		TraceComp->bDebugDraw = bDebugDraw;
		TraceComp->StartTrace(TraceInfos);
	}
}

void UAnimNotifyState_MeleeTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !MeshComp->GetOwner()) return;

	// Always use the component's logic. It handles previous frame locations correctly for smooth sweeps.
	bool bSpawned = false;
	UAdvancedMeleeTraceComponent* TraceComp = GetOrSpawnTraceComponent(MeshComp, bSpawned);
	
	if (TraceComp)
	{
		// Ensure DebugDraw matches (e.g. if toggled during playback)
#if WITH_EDITOR
		UWorld* World = MeshComp->GetWorld();
		if (World && World->IsPreviewWorld())
		{
			// Preview specific logic
		}
#endif
		// Always apply debug draw setting from Notify
		TraceComp->bDebugDraw = bDebugDraw;
		TraceComp->PerformTrace();
	}
}

void UAnimNotifyState_MeleeTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !MeshComp->GetOwner()) return;

	UAdvancedMeleeTraceComponent* TraceComp = MeshComp->GetOwner()->FindComponentByClass<UAdvancedMeleeTraceComponent>();
	if (TraceComp)
	{
		TraceComp->EndTrace();

#if WITH_EDITOR
		// Cleanup Temporary Component
		if (TraceComp->ComponentTags.Contains(TEXT("TempEditorMeleeComponent")))
		{
			TraceComp->DestroyComponent();
		}
#endif
	}

#if WITH_EDITOR
	// Cleanup Preview Weapon Mesh
	if (MeshComp->GetWorld() && MeshComp->GetWorld()->IsPreviewWorld())
	{
		TArray<USceneComponent*> Children;
		MeshComp->GetChildrenComponents(true, Children);
		for (USceneComponent* Child : Children)
		{
			if (Child && Child->ComponentTags.Contains(TEXT("MeleeTracePreviewWeapon")))
			{
				Child->DestroyComponent();
			}
		}
	}
#endif
}
