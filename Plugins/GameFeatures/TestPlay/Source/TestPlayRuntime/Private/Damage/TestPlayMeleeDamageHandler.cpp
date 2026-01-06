// Copyright Epic Games, Inc. All Rights Reserved.

#include "Damage/TestPlayMeleeDamageHandler.h"
#include "AdvancedMeleeTraceComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/LyraGameplayEffectContext.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogTestPlayMeleeHandler, Log, All);

UTestPlayMeleeDamageHandler::UTestPlayMeleeDamageHandler()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTestPlayMeleeDamageHandler::BeginPlay()
{
	Super::BeginPlay();

	// BeginPlay에서 MeleeTraceComponent 찾아 바인딩
	BindToMeleeTraceComponent();
}

void UTestPlayMeleeDamageHandler::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromMeleeTraceComponent();
	Super::EndPlay(EndPlayReason);
}

void UTestPlayMeleeDamageHandler::BindToMeleeTraceComponent()
{
	if (!GetOwner()) return;

	UAdvancedMeleeTraceComponent* MeleeComp = GetOwner()->FindComponentByClass<UAdvancedMeleeTraceComponent>();
	if (MeleeComp)
	{
		MeleeComp->OnMeleeHit.AddDynamic(this, &UTestPlayMeleeDamageHandler::HandleMeleeHit);
		MeleeComp->OnMeleeHitBlocked.AddDynamic(this, &UTestPlayMeleeDamageHandler::HandleMeleeHitBlocked);
		CachedMeleeTraceComp = MeleeComp;
		UE_LOG(LogTestPlayMeleeHandler, Log, TEXT("MeleeTraceComponent에 OnMeleeHit/OnMeleeHitBlocked 바인딩 완료: %s"), *GetOwner()->GetName());
	}
	else
	{
		UE_LOG(LogTestPlayMeleeHandler, Warning, TEXT("MeleeTraceComponent를 찾을 수 없습니다: %s"), *GetOwner()->GetName());
	}
}

void UTestPlayMeleeDamageHandler::UnbindFromMeleeTraceComponent()
{
	if (CachedMeleeTraceComp.IsValid())
	{
		CachedMeleeTraceComp->OnMeleeHit.RemoveDynamic(this, &UTestPlayMeleeDamageHandler::HandleMeleeHit);
		CachedMeleeTraceComp->OnMeleeHitBlocked.RemoveDynamic(this, &UTestPlayMeleeDamageHandler::HandleMeleeHitBlocked);
		CachedMeleeTraceComp.Reset();
	}
}

void UTestPlayMeleeDamageHandler::HandleMeleeHit(AActor* HitActor, const FHitResult& HitResult)
{
	if (!HitActor)
	{
		UE_LOG(LogTestPlayMeleeHandler, Warning, TEXT("HandleMeleeHit: HitActor가 nullptr입니다."));
		return;
	}

	// 서버에서만 GE 적용
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	if (!MeleeDamageEffect)
	{
		UE_LOG(LogTestPlayMeleeHandler, Warning, TEXT("HandleMeleeHit: MeleeDamageEffect가 설정되지 않았습니다."));
		return;
	}

	// 공격자의 ASC 가져오기
	UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
	if (!SourceASC)
	{
		UE_LOG(LogTestPlayMeleeHandler, Warning, TEXT("HandleMeleeHit: Source ASC를 찾을 수 없습니다."));
		return;
	}

	// 타겟의 ASC 가져오기
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitActor);
	if (!TargetASC)
	{
		UE_LOG(LogTestPlayMeleeHandler, Log, TEXT("HandleMeleeHit: Target ASC가 없습니다 (비-캐릭터 히트): %s"), *HitActor->GetName());
		return;
	}

	// GameplayEffectContext 생성 및 HitResult 설정
	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(Owner);
	ContextHandle.AddHitResult(HitResult);

	// Lyra 커스텀 컨텍스트에 추가 정보 설정
	if (FLyraGameplayEffectContext* LyraContext = FLyraGameplayEffectContext::ExtractEffectContext(ContextHandle))
	{
		// SetAbilitySource는 LYRAGAME_API로 export되지 않아 호출 불가 (Linker Error)
		// LyraContext->SetAbilitySource(nullptr, 1.0f); 
	}

	// GameplayEffectSpec 생성
	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(MeleeDamageEffect, 1.0f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogTestPlayMeleeHandler, Warning, TEXT("HandleMeleeHit: GE Spec 생성 실패."));
		return;
	}

	// BaseDamage 설정 (SetByCallerMagnitude 대신 Attribute로 설정)
	// 참고: 실제로는 LyraCombatSet의 BaseDamage를 미리 설정해야 함
	// 여기서는 Spec에 직접 설정하는 방식 시도
	SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("SetByCaller.Damage")), BaseDamage);

	// 타겟에게 GE 적용
	FActiveGameplayEffectHandle ActiveGEHandle = SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data, TargetASC);

	if (ActiveGEHandle.IsValid() || true) // Instant GE는 Handle이 Invalid일 수 있음
	{
		UE_LOG(LogTestPlayMeleeHandler, Log, 
			TEXT("근접 대미지 적용: %s -> %s, Damage=%.1f"), 
			*Owner->GetName(), *HitActor->GetName(), BaseDamage);
	}
}

void UTestPlayMeleeDamageHandler::HandleMeleeHitBlocked(AActor* BlockingActor, const FHitResult& HitResult, FVector TraceDirection)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// 서버에서만 처리
	if (!Owner->HasAuthority())
	{
		return;
	}

	// 반동 방향 계산: 궤적 방향의 반대 (공격 방향의 반대)
	// TraceDirection이 유효하면 사용, 아니면 Fallback
	FVector RecoilDirection;
	if (!TraceDirection.IsNearlyZero())
	{
		// 궤적 방향의 반대 방향으로 반동
		RecoilDirection = -TraceDirection.GetSafeNormal2D();
	}
	else
	{
		// Fallback: 공격자 -> Block 지점의 반대 방향
		FVector AttackDirection = (HitResult.ImpactPoint - Owner->GetActorLocation()).GetSafeNormal2D();
		if (AttackDirection.IsNearlyZero())
		{
			AttackDirection = Owner->GetActorForwardVector();
		}
		RecoilDirection = -AttackDirection;
	}

	UE_LOG(LogTestPlayMeleeHandler, Log, 
		TEXT("HandleMeleeHitBlocked: BlockingActor=%s, TraceDir=(%0.2f, %0.2f, %0.2f), RecoilDir=(%0.2f, %0.2f, %0.2f), Distance=%.1f"),
		*GetNameSafe(BlockingActor),
		TraceDirection.X, TraceDirection.Y, TraceDirection.Z,
		RecoilDirection.X, RecoilDirection.Y, RecoilDirection.Z,
		BlockRecoilDistance);

	// 반동 거리가 0 이상이면 반동 이동 적용
	if (BlockRecoilDistance > 0.0f)
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
		{
			ApplyBlockRecoil(OwnerCharacter, RecoilDirection, BlockRecoilDistance);
		}
	}

	// === Physical Reaction (공격자에게 적용) ===
	// Block당한 공격자(Owner)에게 물리적 반동 적용
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(Owner))
	{
		// 공격자 입장에서 RecoilDirection은 이미 반동 방향 (공격 반대 방향)
		ApplyBlockPhysicalReaction(OwnerCharacter, RecoilDirection);
	}

	// Blueprint 델리게이트 브로드캐스트 (애니메이션, 사운드 등 추가 효과용)
	OnMeleeBlocked.Broadcast(BlockingActor, HitResult, RecoilDirection);
}

void UTestPlayMeleeDamageHandler::ApplyBlockRecoil(ACharacter* Character, const FVector& RecoilDirection, float Distance)
{
	if (!Character)
	{
		return;
	}

	UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
	if (!MovementComp)
	{
		UE_LOG(LogTestPlayMeleeHandler, Warning, 
			TEXT("ApplyBlockRecoil: CharacterMovementComponent를 찾을 수 없습니다: %s"),
			*Character->GetName());
		return;
	}

	// LaunchCharacter를 사용한 반동 이동
	// 향후 RootMotion, AddImpulse 등으로 확장 가능
	FVector LaunchVelocity = RecoilDirection * Distance;
	
	// AI/RootMotion 간섭 방지: MovementMode를 Falling으로 전환
	// LaunchCharacter는 자동으로 Falling으로 전환하지만, 일부 상황에서 무시될 수 있음
	MovementComp->SetMovementMode(MOVE_Falling);
	
	// XY 오버라이드 (기존 이동을 덮어씀), Z 오버라이드 안함 (점프 등 유지)
	Character->LaunchCharacter(LaunchVelocity, true, false);

	UE_LOG(LogTestPlayMeleeHandler, Log, 
		TEXT("ApplyBlockRecoil: Character=%s, LaunchVelocity=(%0.2f, %0.2f, %0.2f), MovementMode=%d"),
		*Character->GetName(),
		LaunchVelocity.X, LaunchVelocity.Y, LaunchVelocity.Z,
		(int32)MovementComp->MovementMode);

	// 디버그 시각화
	if (bShowDebug)
	{
		FVector StartPos = Character->GetActorLocation();
		FVector EndPos = StartPos + LaunchVelocity;
		
		// 반동 방향 화살표 (녹색)
		DrawDebugDirectionalArrow(
			Character->GetWorld(),
			StartPos,
			EndPos,
			50.0f,        // ArrowSize
			FColor::Green,
			false,        // bPersistentLines
			2.0f,         // LifeTime
			0,            // DepthPriority
			3.0f          // Thickness
		);
		
		// 시작 위치 구체 (노란색)
		DrawDebugSphere(
			Character->GetWorld(),
			StartPos,
			20.0f,
			12,
			FColor::Yellow,
			false,
			2.0f,
			0,
			2.0f
		);
		
		// 목표 위치 구체 (빨간색)
		DrawDebugSphere(
			Character->GetWorld(),
			EndPos,
			20.0f,
			12,
			FColor::Red,
			false,
			2.0f,
			0,
			2.0f
		);
	}
}

void UTestPlayMeleeDamageHandler::ApplyBlockPhysicalReaction(ACharacter* Character, const FVector& RecoilDirection)
{
	if (!Character)
	{
		return;
	}	

	// 서버에서 Multicast RPC 호출 (모든 클라이언트에서 시각 효과 실행)
	ApplyBlockPhysicalReaction_Multicast(Character, RecoilDirection);
}

void UTestPlayMeleeDamageHandler::ApplyBlockPhysicalReaction_Multicast_Implementation(ACharacter* Character, FVector RecoilDirection)
{
	if (!Character)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = Character->GetMesh();
	if (!Mesh)
	{
		return;
	}

	// === 1. 애니메이션 몽타주 중단 (Physics Asset 없이도 실행) ===
	if (bInterruptMontageOnBlock)
	{
		if (UAnimInstance* AnimInst = Mesh->GetAnimInstance())
		{
			AnimInst->Montage_Stop(0.1f);
			UE_LOG(LogTestPlayMeleeHandler, Log, 
				TEXT("ApplyBlockPhysicalReaction_Multicast: 몽타주 정지. Character=%s"),
				*Character->GetName());
		}
	}

	// === 2. 물리 반응 (Physics Asset 필요) ===
	if (PhysicalReactionBoneName.IsNone())
	{
		return;
	}

	// Physics Asset 유효성 체크
	if (!Mesh->GetPhysicsAsset())
	{
		UE_LOG(LogTestPlayMeleeHandler, Warning, 
			TEXT("ApplyBlockPhysicalReaction_Multicast: Physics Asset이 설정되지 않았습니다. Character=%s, Mesh=%s"),
			*Character->GetName(), *GetNameSafe(Mesh->GetSkeletalMeshAsset()));
		return;
	}

	// 본 이름 유효성 체크
	int32 BoneIndex = Mesh->GetBoneIndex(PhysicalReactionBoneName);
	if (BoneIndex == INDEX_NONE)
	{
		UE_LOG(LogTestPlayMeleeHandler, Warning, 
			TEXT("ApplyBlockPhysicalReaction_Multicast: 유효하지 않은 본 이름입니다. Character=%s, BoneName=%s"),
			*Character->GetName(), *PhysicalReactionBoneName.ToString());
		return;
	}

	// 물리 시뮬레이션 활성화 (부분)
	Mesh->SetAllBodiesBelowSimulatePhysics(PhysicalReactionBoneName, true, true);
	
	// 블렌드 웨이트 1.0 (물리 100%)
	Mesh->SetAllBodiesBelowPhysicsBlendWeight(PhysicalReactionBoneName, 1.0f);

	// 충격 가하기
	// RecoilDirection은 충격 방향 (Normalized expected)
	FVector Impulse = RecoilDirection * PhysicalReactionImpulseStrength;
	Mesh->AddImpulse(Impulse, PhysicalReactionBoneName, true); // bVelChange = true (질량 무시하고 속도 변화)

	UE_LOG(LogTestPlayMeleeHandler, Log, 
		TEXT("ApplyBlockPhysicalReaction_Multicast: Character=%s, Bone=%s, Impulse=(%0.2f, %0.2f, %0.2f)"),
		*Character->GetName(), *PhysicalReactionBoneName.ToString(),
		Impulse.X, Impulse.Y, Impulse.Z);

	// 복구 타이머 설정 (각 인스턴스에서 로컬로 처리)
	if (PhysicalReactionDuration > 0.0f)
	{
		FTimerDelegate TimerDel;
		TimerDel.BindUObject(this, &UTestPlayMeleeDamageHandler::ResetPhysicalReaction, Mesh, PhysicalReactionBoneName);
		
		GetWorld()->GetTimerManager().SetTimer(PhysicalReactionResetTimerHandle, TimerDel, PhysicalReactionDuration, false);
	}
}

void UTestPlayMeleeDamageHandler::ResetPhysicalReaction(USkeletalMeshComponent* Mesh, FName BoneName)
{
	if (!Mesh) return;

	// 블렌드 웨이트를 0으로 서서히 줄이거나 바로 끄기
	// 여기서는 즉시 0으로 설정 후 시뮬레이션 끄기 (자연스럽게 하려면 Tick에서 보간해야 하나, 간단히 구현)
	// 더 부드럽게 하려면 별도의 보간 로직이 필요하지만, 일단 기능 동작 확인 위주
	
	// 1. Blend Weight 0으로 (애니메이션 주도권 회복)
	Mesh->SetAllBodiesBelowPhysicsBlendWeight(BoneName, 0.0f);

	// 2. 시뮬레이션 끄기 (옵션: 끄면 포즈가 스냅될 수 있음. BlendWeight 0이면 시뮬레이션 켜져있어도 애니메이션 따라감)
	// 성능을 위해 끄는 것이 좋음.
	Mesh->SetAllBodiesBelowSimulatePhysics(BoneName, false, true);

	UE_LOG(LogTestPlayMeleeHandler, Verbose, TEXT("ResetPhysicalReaction: Completed for %s"), *GetNameSafe(Mesh->GetOwner()));
}
