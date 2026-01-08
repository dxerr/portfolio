#include "TestPlayWeaponBlock.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"

ATestPlayWeaponBlock::ATestPlayWeaponBlock()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowDebug = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ATestPlayWeaponBlock::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}

void ATestPlayWeaponBlock::BeginPlay()
{
	Super::BeginPlay();

	TargetCollisionComponent = nullptr;

	if (!CollisionComponentName.IsNone())
	{
		// Find the component
		TArray<UActorComponent*> Components;
		GetComponents(Components);

		for (UActorComponent* Comp : Components)
		{
			if (Comp->GetFName() == CollisionComponentName)
			{
				if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Comp))
				{
					TargetCollisionComponent = PrimComp;
					
					// Cache original settings
					OriginalCollisionProfileName = PrimComp->GetCollisionProfileName();
					OriginalCollisionEnabled = PrimComp->GetCollisionEnabled();

					// Initial state: Deactivate collision if needed, or user might want it active?
					// Usually weapon blocks wait for usage. Let's disable it initially.
					PrimComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					// Removed SetCollisionProfileName to prevent resetting custom collision settings (e.g. Ignore Pawn)

					break;
				}
			}
		}
	}

	// 기본 비활성화
	SetActive(false);
}

void ATestPlayWeaponBlock::SetActive(bool bIsActive)
{
	if (UPrimitiveComponent* PrimComp = TargetCollisionComponent.Get())
	{
		AActor* OwnerActor = GetOwner();

		if (bIsActive)
		{
			// Restore original settings
			// PrimComp->SetCollisionProfileName(OriginalCollisionProfileName); // Removed to keep Custom settings
			PrimComp->SetCollisionEnabled(OriginalCollisionEnabled);

			// Ignore collision with Owner to prevent movement interference
			// Ignore collision with Owner to prevent movement interference
			if (OwnerActor)
			{
				if (UPrimitiveComponent* OwnerRoot = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent()))
				{
					OwnerRoot->IgnoreActorWhenMoving(this, true);
				}
				PrimComp->IgnoreActorWhenMoving(OwnerActor, true);
			}
		}
		else
		{
			// Disable
			PrimComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			// PrimComp->SetCollisionProfileName(FName("NoCollision")); // Removed

			// Restore collision with Owner (cleanup)
			// Restore collision with Owner (cleanup)
			if (OwnerActor)
			{
				if (UPrimitiveComponent* OwnerRoot = Cast<UPrimitiveComponent>(OwnerActor->GetRootComponent()))
				{
					OwnerRoot->IgnoreActorWhenMoving(this, false);
				}
				PrimComp->IgnoreActorWhenMoving(OwnerActor, false);
			}
		}
	}
}

#if WITH_EDITOR
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#endif

TArray<FString> ATestPlayWeaponBlock::GetCollisionComponentNames() const
{
	TArray<FString> Names;
	
	// 1. Native Components (Always available)
	for (UActorComponent* Comp : GetComponents())
	{
		if (Cast<UPrimitiveComponent>(Comp))
		{
			Names.Add(Comp->GetName());
		}
	}

#if WITH_EDITOR
	// 2. Blueprint Components (SCS) - Only when editing BP Defaults (CDO context)
	if (const UBlueprintGeneratedClass* BPGC = Cast<UBlueprintGeneratedClass>(GetClass()))
	{
		// Check SimpleConstructionScript for components added in Blueprint Editor
		if (const USimpleConstructionScript* SCS = BPGC->SimpleConstructionScript)
		{
			for (const USCS_Node* Node : SCS->GetAllNodes())
			{
				if (Node->ComponentClass && Node->ComponentClass->IsChildOf(UPrimitiveComponent::StaticClass()))
				{
					// Use variable name as it maps to the component name people see in BP
					Names.Add(Node->GetVariableName().ToString());
				}
			}
		}
	}
#endif

	// TSet으로 중복 제거 및 정렬
	TSet<FString> UniqueSet(Names);
	TArray<FString> Result = UniqueSet.Array();
	Result.Sort();
	return Result;
}

ATestPlayWeaponBlock* ATestPlayWeaponBlock::FindWeaponBlock(AActor* OwnerActor)
{
	if (!OwnerActor)
	{
		return nullptr;
	}

	TArray<AActor*> AttachedActors;
	OwnerActor->GetAttachedActors(AttachedActors);

	for (AActor* Actor : AttachedActors)
	{
		if (ATestPlayWeaponBlock* Block = Cast<ATestPlayWeaponBlock>(Actor))
		{
			return Block;
		}
	}

	return nullptr;
}

void ATestPlayWeaponBlock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bShowDebug && TargetCollisionComponent.IsValid())
	{
		UPrimitiveComponent* PrimComp = TargetCollisionComponent.Get();
		FTransform ComponentTransform = PrimComp->GetComponentTransform();
		FVector Center = ComponentTransform.GetLocation();
		FQuat Rotation = ComponentTransform.GetRotation(); // Box, Capsule use rotation
		FColor DebugColor = FColor::Green;

		if (UBoxComponent* BoxComp = Cast<UBoxComponent>(PrimComp))
		{
			DrawDebugBox(GetWorld(), Center, BoxComp->GetScaledBoxExtent(), Rotation, DebugColor, false, -1.0f, 0, 1.0f);
		}
		else if (USphereComponent* SphereComp = Cast<USphereComponent>(PrimComp))
		{
			DrawDebugSphere(GetWorld(), Center, SphereComp->GetScaledSphereRadius(), 32, DebugColor, false, -1.0f, 0, 1.0f);
		}
		else if (UCapsuleComponent* CapsuleComp = Cast<UCapsuleComponent>(PrimComp))
		{
			DrawDebugCapsule(GetWorld(), Center, CapsuleComp->GetScaledCapsuleHalfHeight(), CapsuleComp->GetScaledCapsuleRadius(), Rotation, DebugColor, false, -1.0f, 0, 1.0f);
		}
		else
		{
			// Fallback: Use Bounds (Rotation ignored for AABB, but bounds are world aligned so ok)
			DrawDebugBox(GetWorld(), PrimComp->Bounds.Origin, PrimComp->Bounds.BoxExtent, FQuat::Identity, DebugColor, false, -1.0f, 0, 1.0f);
		}
	}
}
