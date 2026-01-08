#include "AdvancedMeleeTraceComponent.h"
#include "Components/MeshComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"
#include "Engine/CollisionProfile.h"

DEFINE_LOG_CATEGORY(LogAdvancedMeleeTrace);

UAdvancedMeleeTraceComponent::UAdvancedMeleeTraceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bIsTracing = false;
	TraceChannel = ECC_Pawn;
}

void UAdvancedMeleeTraceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsTracing)
	{
		PerformTrace();
	}
}

void UAdvancedMeleeTraceComponent::StartTrace(const TArray<FMeleeTraceInfo>& TraceInfos)
{
	bIsTracing = true;
	CurrentTraceInfos = TraceInfos;
	
	// Try setup immediately
	SetupActiveTraces();
}

void UAdvancedMeleeTraceComponent::SetupActiveTraces()
{
	ActiveTraces.Empty();
	HitActors.Empty();
	BlockedActors.Empty();
	
	AActor* Owner = GetOwner();
	if (!Owner) return;

	// Radius Extension Direction (Owner Forward)
	FVector ExtensionDir = Owner->GetActorForwardVector();

	for (const auto& Info : CurrentTraceInfos)
	{
		// 소켓을 가진 메시 찾기 (Tag 우선, 없으면 SocketName 기준)
		UPrimitiveComponent* FoundMesh = FindMeshWithSocket(Info.StartSocketName, Info.MeshTag);
		if (!FoundMesh && Info.StartSocketName != Info.EndSocketName)
		{
			// StartSocket이 없으면 EndSocket으로도 찾아봄
			FoundMesh = FindMeshWithSocket(Info.EndSocketName, Info.MeshTag);
		}

		if (FoundMesh)
		{
			FActiveMeleeTrace NewTrace;
			NewTrace.Info = Info;
			NewTrace.MeshComponent = FoundMesh;
			
			// 초기 위치 계산
			FVector StartLoc = FoundMesh->GetSocketLocation(Info.StartSocketName);
			FVector EndLoc = FoundMesh->GetSocketLocation(Info.EndSocketName);
			
			// Radius 확장 (Owner Forward) - 초기값 설정
			FVector StartExt = StartLoc + (ExtensionDir * Info.Radius);
			FVector EndExt = (Info.StartSocketName != Info.EndSocketName) ? 
							 (EndLoc + (ExtensionDir * Info.Radius)) : StartExt;

			NewTrace.PrevPoints[0] = StartLoc;
			NewTrace.PrevPoints[1] = EndLoc;
			NewTrace.PrevPoints[2] = StartExt;
			NewTrace.PrevPoints[3] = EndExt;
			
			ActiveTraces.Add(NewTrace);
		}
	}

	if (ActiveTraces.Num() > 0)
	{
		UE_LOG(LogAdvancedMeleeTrace, Verbose, TEXT("StartTrace: Started with %d traces."), ActiveTraces.Num());
	}
}

void UAdvancedMeleeTraceComponent::EndTrace()
{
	// PDF Requirement: 마지막 프레임의 위치까지 트레이스 보장 (누락 방지)
	if (bIsTracing)
	{
		PerformTrace();
	}

	bIsTracing = false;
	HitActors.Empty();
	BlockedActors.Empty();
	ActiveTraces.Empty();
}

UPrimitiveComponent* UAdvancedMeleeTraceComponent::FindMeshWithSocket(FName SocketName, FName ComponentTag)
{
	if (SocketName.IsNone()) return nullptr;

	AActor* Owner = GetOwner();
	if (!Owner) return nullptr;

	auto CheckMesh = [&](UMeshComponent* Mesh) -> bool
	{
		if (!Mesh) return false;
		if (!ComponentTag.IsNone() && !Mesh->ComponentTags.Contains(ComponentTag))
		{
			return false;
		}
		return Mesh->DoesSocketExist(SocketName);
	};

	// 1. Owner Search
	TArray<UMeshComponent*> MeshComponents;
	Owner->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* Mesh : MeshComponents)
	{
		if (CheckMesh(Mesh)) return Mesh;
	}

	// 2. Attached Actors Search
	TArray<AActor*> AttachedActors;
	Owner->GetAttachedActors(AttachedActors, false, true); 

	for (AActor* AttachedActor : AttachedActors)
	{
		if (!AttachedActor) continue;
		TArray<UMeshComponent*> ChildMeshes;
		AttachedActor->GetComponents<UMeshComponent>(ChildMeshes);
		for (UMeshComponent* ChildMesh : ChildMeshes)
		{
			if (CheckMesh(ChildMesh)) return ChildMesh;
		}
	}

	return nullptr;
}

void UAdvancedMeleeTraceComponent::PerformTrace()
{
	// 1. Late Binding Check
	if (ActiveTraces.Num() == 0 && CurrentTraceInfos.Num() > 0 && bIsTracing)
	{
		SetupActiveTraces();
	}

	if (ActiveTraces.Num() == 0) return;

	for (auto& Trace : ActiveTraces)
	{
		FVector PrevCenter, CurrentCenter, Extent;
		FQuat Rotation;

		// 2. Trajectory & Shape Calculation
		if (UpdateTracePoints(Trace, PrevCenter, CurrentCenter, Rotation, Extent))
		{
			// 3. Collision Sweep
			TArray<FHitResult> Hits;
			PerformSweep(PrevCenter, CurrentCenter, Rotation, Extent, Hits);

			// 4. Hit Processing
			if (Hits.Num() > 0)
			{
				// BlockingChannel 우선 + 거리순 2차 정렬
			// BlockingChannel에 속한 객체는 항상 먼저 처리됨
			ECollisionChannel LocalBlockingChannel = BlockingChannel;
			Hits.Sort([LocalBlockingChannel](const FHitResult& A, const FHitResult& B)
			{
				if (LocalBlockingChannel != ECollisionChannel::ECC_MAX)
				{
					bool bAIsBlocking = A.Component.IsValid() && 
						A.Component->GetCollisionObjectType() == LocalBlockingChannel;
					bool bBIsBlocking = B.Component.IsValid() && 
						B.Component->GetCollisionObjectType() == LocalBlockingChannel;
					
					// BlockingChannel 객체 우선
					if (bAIsBlocking != bBIsBlocking)
					{
						return bAIsBlocking; // BlockingChannel이면 앞으로
					}
				}
				// 동일 타입 내에서는 거리순
				return A.Distance < B.Distance;
			});
			
			UE_LOG(LogAdvancedMeleeTrace, Verbose, TEXT("PerformTrace: Sorted %d hits (BlockingChannel priority: %s)"),
				Hits.Num(), 
				BlockingChannel != ECollisionChannel::ECC_MAX ? TEXT("Enabled") : TEXT("Disabled"));

				// 궤적 방향 계산 (이전 중심점 -> 현재 중심점)
				FVector TraceDirection = (CurrentCenter - PrevCenter).GetSafeNormal();

				for (const FHitResult& Hit : Hits)
				{
					EProcessHitResult Result = ProcessHit(Hit, TraceDirection);

					if (Result == EProcessHitResult::Blocked) break;
					if (Result == EProcessHitResult::Hit && !bProcessMultiHit) break;
				}
			}
		}
	}
}

bool UAdvancedMeleeTraceComponent::UpdateTracePoints(FActiveMeleeTrace& Trace, FVector& OutPrevCenter, FVector& OutCurrentCenter, FQuat& OutRot, FVector& OutExtent)
{
	UPrimitiveComponent* Mesh = Trace.MeshComponent.Get();
	if (!Mesh) return false;

	AActor* Owner = GetOwner();
	FVector ExtensionDir = Owner ? Owner->GetActorForwardVector() : FVector::ForwardVector;

	// 1. Calculate Current Points
	FVector StartLoc = Mesh->GetSocketLocation(Trace.Info.StartSocketName);
	FVector EndLoc = Mesh->GetSocketLocation(Trace.Info.EndSocketName);
	
	// Radius Extension (Owner Forward)
	FVector StartExt = StartLoc + (ExtensionDir * Trace.Info.Radius);
	FVector EndExt = (Trace.Info.StartSocketName != Trace.Info.EndSocketName) ? 
					 (EndLoc + (ExtensionDir * Trace.Info.Radius)) : StartExt; 
	
	if (Trace.Info.StartSocketName == Trace.Info.EndSocketName)
	{
		EndLoc = StartLoc;
		EndExt = StartExt;
	}

	// 2. Calculate Box Geometry
	FVector P0 = StartLoc;
	FVector P1 = EndLoc;
	FVector P2 = StartExt;
	FVector P3 = EndExt;

	OutCurrentCenter = (P0 + P1 + P2 + P3) * 0.25f;

	// Basis Vectors
	FVector AxisX = (P1 - P0); // Length
	float Length = AxisX.Size();
	if (Length > KINDA_SMALL_NUMBER)
	{
		AxisX /= Length;
	}
	else
	{
		// Single Socket Case: Use Radius Direction for orientation
		AxisX = (P2 - P0).GetSafeNormal(); 
		if (AxisX.IsNearlyZero()) AxisX = FVector::ForwardVector;
	}

	FVector AxisY = (P2 - P0); // Width
	float Width = AxisY.Size();
	
	AxisY = (AxisY - AxisX * (AxisY | AxisX)).GetSafeNormal();
	if (AxisY.IsNearlyZero()) AxisY = FVector::RightVector;

	FVector AxisZ = (AxisX ^ AxisY).GetSafeNormal();

	OutRot = FRotationMatrix::MakeFromXY(AxisX, AxisY).ToQuat();
	
	float FinalLength = (P1 - P0).Size();
	float FinalWidth = (P2 - P0).Size();

	OutExtent = FVector(FMath::Max(1.0f, FinalLength * 0.5f), FMath::Max(1.0f, FinalWidth * 0.5f), 5.0f);

	// 3. Calculate Prev Center
	OutPrevCenter = (Trace.PrevPoints[0] + Trace.PrevPoints[1] + Trace.PrevPoints[2] + Trace.PrevPoints[3]) * 0.25f;

	// 4. Update Prev Points for next frame
	Trace.PrevPoints[0] = StartLoc;
	Trace.PrevPoints[1] = EndLoc;
	Trace.PrevPoints[2] = StartExt;
	Trace.PrevPoints[3] = EndExt;

	return true;
}

void UAdvancedMeleeTraceComponent::PerformSweep(const FVector& PrevCenter, const FVector& CurrentCenter, const FQuat& Rot, const FVector& Extent, TArray<FHitResult>& OutHits)
{
	FCollisionShape BoxShape = FCollisionShape::MakeBox(Extent);
	
	TArray<AActor*> GlobalIgnoreList;
	GlobalIgnoreList.Add(GetOwner());
	for(auto& HitActor : HitActors)
	{
		if(HitActor) GlobalIgnoreList.Add(HitActor);
	}

	// === Multi-Object Type Support ===
	if (TraceObjectTypes.Num() > 0)
	{
		UKismetSystemLibrary::BoxTraceMultiForObjects(
			GetWorld(),
			PrevCenter,
			CurrentCenter,
			Extent,
			Rot.Rotator(),
			TraceObjectTypes,
			false, // bTraceComplex
			GlobalIgnoreList,
			bDebugDraw ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None,
			OutHits,
			true, // bIgnoreSelf
			FLinearColor::Green,
			FLinearColor::Red,
			1.0f
		);
	}
	else
	{
		// Native Sweep
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MeleeTrace), false, GetOwner());
		QueryParams.AddIgnoredActors(GlobalIgnoreList);

		GetWorld()->SweepMultiByChannel(OutHits, PrevCenter, CurrentCenter, Rot, TraceChannel, BoxShape, QueryParams);
		
		if (bDebugDraw)
		{
			DrawDebugBox(GetWorld(), CurrentCenter, Extent, Rot, FColor::Green, false, 1.0f);
			DrawDebugLine(GetWorld(), PrevCenter, CurrentCenter, FColor::Green, false, 1.0f);
		}
	}
}

void UAdvancedMeleeTraceComponent::ServerValidateHit_Implementation(AActor* HitActor, const FHitResult& HitResult)
{
    if (!HitActor) return;
    if (HitActors.Contains(HitActor)) return;
    HitActors.Add(HitActor);

	// === 필터링 로직 (서버 검증) ===
	bool bIsBlocked = false;
	
	
	// 콜리전 채널 확인 (프로필 기반)
	if (!bIsBlocked && HitResult.Component.IsValid() && BlockingChannel != ECollisionChannel::ECC_MAX)
	{
		const ECollisionChannel HitChannel = HitResult.Component->GetCollisionObjectType();
		if (HitChannel == BlockingChannel)
		{
			bIsBlocked = true;
		}
	}

	if (bIsBlocked)
	{
		OnMeleeHitBlocked.Broadcast(HitActor, HitResult, FVector::ZeroVector);
		return;
	}
	// === 필터링 로직 끝 ===
	
    OnMeleeHit.Broadcast(HitActor, HitResult);
}

bool UAdvancedMeleeTraceComponent::ServerValidateHit_Validate(AActor* HitActor, const FHitResult& HitResult)
{
    if (!HitActor) return false;
    
    AActor* Owner = GetOwner();
    if (!Owner) return false;

    // 거리 검증: 최대 공격 사거리 + 여유분 (5m = 500 units)
    constexpr float MaxValidDistance = 500.0f;
    float DistanceSq = FVector::DistSquared(Owner->GetActorLocation(), HitActor->GetActorLocation());
    
    if (DistanceSq > FMath::Square(MaxValidDistance))
    {
        UE_LOG(LogAdvancedMeleeTrace, Warning, TEXT("ServerValidateHit rejected: Target too far (%.1f > %.1f)"), 
            FMath::Sqrt(DistanceSq), MaxValidDistance);
        return false;
    }
    
    return true;
}

UAdvancedMeleeTraceComponent::EProcessHitResult UAdvancedMeleeTraceComponent::ProcessHit(const FHitResult& Hit, const FVector& TraceDirection)
{
	if (!Hit.GetActor()) return EProcessHitResult::Ignored;
	
	AActor* HitActor = Hit.GetActor();

	// 중복 체크: 이미 Valid Hit로 처리된 액터인가?
	if (HitActors.Contains(HitActor))
	{
		// UE_LOG(LogAdvancedMeleeTrace, Verbose, TEXT("ProcessHit: Ignored duplicate HitActor %s"), *GetNameSafe(HitActor));
		return EProcessHitResult::Ignored;
	}
	
	// 중복 체크: 이미 Blocked로 처리된 액터인가?
	if (BlockedActors.Contains(HitActor))
	{
		// UE_LOG(LogAdvancedMeleeTrace, Verbose, TEXT("ProcessHit: Ignored already blocked HitActor %s"), *GetNameSafe(HitActor));
		return EProcessHitResult::Blocked;
	}

	if (bDebugDraw)
	{
		DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10.0f, FColor::Red, false, 1.0f);
	}

	// === 필터링 로직 시작 ===
	bool bIsBlocked = false;	

	// 콜리전 채널 확인 (프로필 기반)
	if (!bIsBlocked && Hit.Component.IsValid() && BlockingChannel != ECollisionChannel::ECC_MAX)
	{
		// 히트된 채널이 BlockingChannel과 일치하는지 확인
		const ECollisionChannel HitChannel = Hit.Component->GetCollisionObjectType();		
		if (HitChannel == BlockingChannel)
		{
			bIsBlocked = true;			
		}
	}

	// 차단된 경우, 이벤트 발생 후 리턴 (OnMeleeHit 호출 안 함)
	if (bIsBlocked)
	{
		UE_LOG(LogAdvancedMeleeTrace, Log, TEXT("ProcessHit: BLOCKED by Actor: %s, Component: %s (Channel: %d), TraceDir=(%0.2f, %0.2f, %0.2f)"), 
			*GetNameSafe(HitActor), 
			*GetNameSafe(Hit.Component.Get()), 
			(int32)Hit.Component->GetCollisionObjectType(),
			TraceDirection.X, TraceDirection.Y, TraceDirection.Z);

		BlockedActors.Add(HitActor); // Block 처리된 액터 등록 (중복 처리 방지)

		if (bDebugDraw)
		{
			DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10.0f, FColor::Blue, false, 1.0f);
		}

		AActor* Owner = GetOwner();
		if (Owner && Owner->HasAuthority())
		{
			OnMeleeHitBlocked.Broadcast(HitActor, Hit, TraceDirection);
		}
		
		return EProcessHitResult::Blocked; 
	}
	// === 필터링 로직 끝 ===

	UE_LOG(LogAdvancedMeleeTrace, Log, TEXT("ProcessHit: Valid Hit on Actor: %s, Component: %s, Distance: %.2f"), 
		*GetNameSafe(HitActor), 
		*GetNameSafe(Hit.Component.Get()), 
		Hit.Distance);

    HitActors.Add(HitActor); // Hit 처리된 액터 등록

    AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority())
    {
        OnMeleeHit.Broadcast(HitActor, Hit);
    }
    else
    {
        ServerValidateHit(HitActor, Hit);
    }

	return EProcessHitResult::Hit;
}
