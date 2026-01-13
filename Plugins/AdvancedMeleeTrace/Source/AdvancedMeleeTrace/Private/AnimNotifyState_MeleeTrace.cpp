#include "AnimNotifyState_MeleeTrace.h"
#include "AdvancedMeleeTraceComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMeshSocket.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/Actor.h"

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

#if WITH_EDITOR
void UAnimNotifyState_MeleeTrace::EnsurePreviewWeaponSpawned(USkeletalMeshComponent* MeshComp)
{
	if (!MeshComp || !MeshComp->GetOwner()) return;
	
	UWorld* World = MeshComp->GetWorld();
	if (!World || !World->IsPreviewWorld()) return;
	if (WeaponAttachmentSocket.IsNone()) return;
	
	// 둘 다 없으면 리턴
	if (!PreviewWeaponMesh && !PreviewWeaponActorClass) return;

	// 현재 소스 결정 (ActorClass 우선, 없으면 Mesh 사용)
	UObject* CurrentSource = PreviewWeaponActorClass ? Cast<UObject>(PreviewWeaponActorClass) : PreviewWeaponMesh.Get();

	// 캐시 유효성 검사: 에셋이나 소켓이 변경되었으면 기존 프리뷰 제거
	bool bNeedsRespawn = false;
	if (CachedPreviewSourceAsset.Get() != CurrentSource || CachedPreviewSocket != WeaponAttachmentSocket)
	{
		bNeedsRespawn = true;
		// 기존 프리뷰 제거
		if (CachedPreviewActor.IsValid())
		{
			CachedPreviewActor->Destroy();
			CachedPreviewActor = nullptr;
		}
		if (CachedPreviewComponent.IsValid())
		{
			CachedPreviewComponent->DestroyComponent();
			CachedPreviewComponent = nullptr;
		}
	}

	// 이미 유효한 캐시가 있으면 스킵
	if (!bNeedsRespawn && (CachedPreviewComponent.IsValid() || CachedPreviewActor.IsValid()))
	{
		return;
	}

	// 스폰 로직 시작
	USceneComponent* SpawnedComp = nullptr;
	AActor* SpawnedActor = nullptr;

	// 1. Actor Class 우선 처리
	if (PreviewWeaponActorClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.ObjectFlags = RF_Transient;
		SpawnedActor = World->SpawnActor<AActor>(PreviewWeaponActorClass, FTransform::Identity, SpawnParams);
		if (SpawnedActor)
		{
			SpawnedActor->Tags.Add(TEXT("MeleeTracePreviewActor"));
			SpawnedActor->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachmentSocket);
		}
	}
	// 2. Mesh 처리 (ActorClass가 없을 때만)
	else if (PreviewWeaponMesh)
	{
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
	}

	if (SpawnedComp)
	{
		SpawnedComp->ComponentTags.Add(TEXT("MeleeTracePreviewWeapon"));
		SpawnedComp->RegisterComponent();
		SpawnedComp->AttachToComponent(MeshComp, FAttachmentTransformRules::SnapToTargetNotIncludingScale, WeaponAttachmentSocket);
		CachedPreviewComponent = SpawnedComp;
	}
	
	if (SpawnedActor)
	{
		CachedPreviewActor = SpawnedActor;
	}
	
	// 캐시 정보 업데이트
	CachedPreviewSourceAsset = CurrentSource;
	CachedPreviewSocket = WeaponAttachmentSocket;
}
#endif

void UAnimNotifyState_MeleeTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !MeshComp->GetOwner()) return;

#if WITH_EDITOR
	// 1. Setup Preview Weapon Mesh (if needed) - Must be done BEFORE StartTrace so sockets exist
	EnsurePreviewWeaponSpawned(MeshComp);
#endif

	// 2. Get or Spawn Component and Start Trace
	bool bSpawned = false;
	UAdvancedMeleeTraceComponent* TraceComp = GetOrSpawnTraceComponent(MeshComp, bSpawned);
	if (TraceComp)
	{
		// Always apply debug draw setting from Notify
		TraceComp->bDebugDraw = bDebugDraw;
		TraceComp->StartTrace(TraceInfos);
	}
}

void UAnimNotifyState_MeleeTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !MeshComp->GetOwner()) return;

#if WITH_EDITOR
	// 프레임 스크러빙 대응: 프리뷰가 없으면 생성
	EnsurePreviewWeaponSpawned(MeshComp);
#endif

	// Always use the component's logic. It handles previous frame locations correctly for smooth sweeps.
	bool bSpawned = false;
	UAdvancedMeleeTraceComponent* TraceComp = GetOrSpawnTraceComponent(MeshComp, bSpawned);
	
	if (TraceComp)
	{
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

	// NOTE: 프리뷰 무기 메시는 캐싱하므로 NotifyEnd에서 제거하지 않음.
	// 에셋이 변경되거나 애니메이션이 닫힐 때 GC가 처리함.
}
