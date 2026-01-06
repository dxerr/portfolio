// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "TestPlayMeleeDamageHandler.generated.h"

class UAdvancedMeleeTraceComponent;
class UAbilitySystemComponent;
class UGameplayEffect;
class ACharacter;

/**
 * Block 이벤트 델리게이트 (Blueprint에서 애니메이션, 사운드 등 추가 효과 구현용)
 * @param BlockingActor - 공격을 막은 액터
 * @param HitResult - 히트 정보
 * @param RecoilDirection - 반동 방향 (공격 반대 방향)
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMeleeBlocked, AActor*, BlockingActor, const FHitResult&, HitResult, FVector, RecoilDirection);

/**
 * UTestPlayMeleeDamageHandler
 *
 * UAdvancedMeleeTraceComponent의 OnMeleeHit/OnMeleeHitBlocked 델리게이트를 받아
 * 대미지 및 Block 처리를 담당하는 핸들러 컴포넌트입니다.
 *
 * 사용법:
 * 1. 캐릭터에 이 컴포넌트 추가
 * 2. MeleeDamageEffect에 대미지 GE 설정 (Execution: UTestPlayMeleeDamageExecution)
 * 3. UAdvancedMeleeTraceComponent::OnMeleeHit 발생 시 자동으로 GE 적용
 * 4. Block 시 반동(Recoil) 효과 자동 적용
 *
 * 특징:
 * - OnMeleeHit/OnMeleeHitBlocked 델리게이트를 자동 바인딩
 * - HitResult를 GameplayEffectContext에 전달
 * - 서버에서만 GE/반동 적용 (네트워크 안전)
 * - OnMeleeBlocked 델리게이트로 추가 효과(애니메이션, 사운드) Blueprint 연동 가능
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESTPLAYRUNTIME_API UTestPlayMeleeDamageHandler : public UActorComponent
{
	GENERATED_BODY()

public:
	UTestPlayMeleeDamageHandler();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ============================================================
	// Damage Settings
	// ============================================================

	/**
	 * 근접 공격 시 적용할 GameplayEffect 클래스
	 * Execution으로 UTestPlayMeleeDamageExecution을 사용해야 함
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Damage")
	TSubclassOf<UGameplayEffect> MeleeDamageEffect;

	/**
	 * 기본 대미지 값 (GE에서 BaseDamage로 캡처됨)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Damage")
	float BaseDamage = 50.0f;

	// ============================================================
	// Block Settings
	// ============================================================

	/**
	 * Block 시 공격자에게 적용할 반동 거리 (0 = 무시)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Block")
	float BlockRecoilDistance = 100.0f;

	/**
	 * Block 이벤트 델리게이트 (Blueprint에서 추가 효과 구현용)
	 * 애니메이션, 사운드, VFX 등을 Blueprint에서 바인딩하여 사용
	 */
	UPROPERTY(BlueprintAssignable, Category = "Melee Block")
	FOnMeleeBlocked OnMeleeBlocked;

	// ============================================================
	// Physical Reaction Settings (Block 시 물리 반응)
	// ============================================================

	/**
	 * 물리 반응 활성화 여부
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Block|Physical Reaction")
	bool bEnablePhysicalReaction = false;

	/**
	 * 물리 시뮬레이션을 적용할 시작 본 이름 (이 본 이하가 물리 반응)
	 * 예: "RightHand", "RightArm", "Spine" 등
	 */
	/**
	 * 물리 시뮬레이션을 적용할 시작 본 이름 (이 본 이하가 물리 반응)
	 * 예: "RightHand", "RightArm", "Spine" 등
	 * 설정하지 않으면 물리 반응이 비활성화됩니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Block|Physical Reaction")
	FName PhysicalReactionBoneName = NAME_None;

	/**
	 * Impulse 강도 (물리적 밀림 정도)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Block|Physical Reaction", meta = (ClampMin = "0"))
	float PhysicalReactionImpulseStrength = 500.0f;

	/**
	 * Physics Blend Weight가 1.0으로 유지되는 시간 (초)
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Block|Physical Reaction", meta = (ClampMin = "0.1"))
	float PhysicalReactionDuration = 0.3f;

	/**
	 * Block 시 기존 애니메이션 몽타주 중단 여부
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Block|Physical Reaction")
	bool bInterruptMontageOnBlock = true;

	// ============================================================
	// Debug Settings
	// ============================================================

	/**
	 * 디버그 시각화 활성화 (Block 반동 방향 표시)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bShowDebug = false;

protected:
	/**
	 * OnMeleeHit 델리게이트 핸들러
	 * @param HitActor - 맞은 액터
	 * @param HitResult - 히트 정보
	 */
	UFUNCTION()
	void HandleMeleeHit(AActor* HitActor, const FHitResult& HitResult);

	/**
	 * OnMeleeHitBlocked 델리게이트 핸들러
	 * @param BlockingActor - 공격을 막은 액터
	 * @param HitResult - 히트 정보
	 * @param TraceDirection - 공격 궤적 방향
	 */
	UFUNCTION()
	void HandleMeleeHitBlocked(AActor* BlockingActor, const FHitResult& HitResult, FVector TraceDirection);

	/**
	 * 반동 이동 적용 (가상 함수 - 향후 RootMotion 등 확장 가능)
	 * @param Character - 반동을 적용할 캐릭터
	 * @param RecoilDirection - 반동 방향 (정규화됨)
	 * @param Distance - 반동 거리
	 */
	virtual void ApplyBlockRecoil(ACharacter* Character, const FVector& RecoilDirection, float Distance);

	/**
	 * 물리 반응(Partial Ragdoll) 적용
	 * @param Character - 반응을 적용할 캐릭터
	 * @param RecoilDirection - 충격 방향
	 */
	virtual void ApplyBlockPhysicalReaction(ACharacter* Character, const FVector& RecoilDirection);

	/**
	 * 물리 반응을 모든 클라이언트에 브로드캐스트 (시각 효과용)
	 * @param Character - 반응을 적용할 캐릭터
	 * @param RecoilDirection - 충격 방향
	 */
	UFUNCTION(NetMulticast, Reliable)
	void ApplyBlockPhysicalReaction_Multicast(ACharacter* Character, FVector RecoilDirection);

	/**
	 * 물리 반응 복구 (PhysicsBlendWeight를 0으로)
	 */
	void ResetPhysicalReaction(USkeletalMeshComponent* Mesh, FName BoneName);

	/**
	 * UAdvancedMeleeTraceComponent를 찾아 델리게이트 바인딩
	 */
	void BindToMeleeTraceComponent();

	/**
	 * 델리게이트 언바인딩
	 */
	void UnbindFromMeleeTraceComponent();

private:
	UPROPERTY()
	TWeakObjectPtr<UAdvancedMeleeTraceComponent> CachedMeleeTraceComp;

	/** 물리 반응 복구 타이머 핸들 */
	FTimerHandle PhysicalReactionResetTimerHandle;
};
