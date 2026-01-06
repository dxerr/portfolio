// Copyright Epic Games, Inc. All Rights Reserved.

#include "Damage/TestPlayMeleeDamageExecution.h"
#include "AbilitySystem/Attributes/LyraHealthSet.h"
#include "AbilitySystem/Attributes/LyraCombatSet.h"
#include "AbilitySystem/LyraGameplayEffectContext.h"
#include "AbilitySystem/LyraAbilitySourceInterface.h"
#include "Teams/LyraTeamSubsystem.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogTestPlayMeleeDamage, Log, All);

// 대미지 관련 속성 캡처 정의
struct FMeleeDamageStatics
{
	FGameplayEffectAttributeCaptureDefinition BaseDamageDef;

	FMeleeDamageStatics()
	{
		// Source(공격자)의 BaseDamage 속성 캡처
		BaseDamageDef = FGameplayEffectAttributeCaptureDefinition(
			ULyraCombatSet::GetBaseDamageAttribute(),
			EGameplayEffectAttributeCaptureSource::Source,
			true
		);
	}
};

static FMeleeDamageStatics& MeleeDamageStatics()
{
	static FMeleeDamageStatics Statics;
	return Statics;
}

UTestPlayMeleeDamageExecution::UTestPlayMeleeDamageExecution()
{
	// 캡처할 속성 등록
	RelevantAttributesToCapture.Add(MeleeDamageStatics().BaseDamageDef);
}

void UTestPlayMeleeDamageExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput
) const
{
#if WITH_SERVER_CODE
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	FLyraGameplayEffectContext* TypedContext = FLyraGameplayEffectContext::ExtractEffectContext(Spec.GetContext());
	
	if (!TypedContext)
	{
		UE_LOG(LogTestPlayMeleeDamage, Warning, TEXT("Execute_Implementation: TypedContext가 없습니다."));
		return;
	}

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	// 1. 기본 대미지 캡처
	float BaseDamage = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(
		MeleeDamageStatics().BaseDamageDef,
		EvaluateParameters,
		BaseDamage
	);

	// 2. 히트 정보 가져오기 (UAdvancedMeleeTraceComponent에서 설정한 HitResult)
	const AActor* EffectCauser = TypedContext->GetEffectCauser();
	const FHitResult* HitActorResult = TypedContext->GetHitResult();

	AActor* HitActor = nullptr;
	FVector ImpactLocation = FVector::ZeroVector;

	if (HitActorResult)
	{
		const FHitResult& CurHitResult = *HitActorResult;
		HitActor = CurHitResult.HitObjectHandle.FetchActor();
		if (HitActor)
		{
			ImpactLocation = CurHitResult.ImpactPoint;
		}
	}

	// HitResult가 없는 경우 Target ASC에서 가져오기
	UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	if (!HitActor)
	{
		HitActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor_Direct() : nullptr;
		if (HitActor)
		{
			ImpactLocation = HitActor->GetActorLocation();
		}
	}

	// 3. 팀 대미지 허용 체크
	float DamageInteractionAllowedMultiplier = 0.0f;
	if (HitActor)
	{
		ULyraTeamSubsystem* TeamSubsystem = HitActor->GetWorld()->GetSubsystem<ULyraTeamSubsystem>();
		if (TeamSubsystem)
		{
			// 같은 팀이면 0, 적대 팀이면 1
			DamageInteractionAllowedMultiplier = TeamSubsystem->CanCauseDamage(EffectCauser, HitActor) ? 1.0f : 0.0f;
		}
		else
		{
			// TeamSubsystem이 없으면 기본적으로 대미지 허용
			DamageInteractionAllowedMultiplier = 1.0f;
		}
	}

	// 4. 거리 계산 (근접 공격은 일반적으로 거리 감쇠가 없지만, 필요 시 적용 가능)
	double Distance = 0.0;
	if (TypedContext->HasOrigin())
	{
		Distance = FVector::Dist(TypedContext->GetOrigin(), ImpactLocation);
	}
	else if (EffectCauser)
	{
		Distance = FVector::Dist(EffectCauser->GetActorLocation(), ImpactLocation);
	}

	// 5. 물리 재질 감쇠 (선택적)
	float PhysicalMaterialAttenuation = 1.0f;
	float DistanceAttenuation = 1.0f;

	// GetAbilitySource, GetPhysicalMaterial은 export되지 않아 호출 시 Linker Error 발생
	// 근접 공격에서는 이 기능들을 제외하고 기본값 적용
	/*
	if (const ILyraAbilitySourceInterface* AbilitySource = TypedContext->GetAbilitySource())
	{
		if (const UPhysicalMaterial* PhysMat = TypedContext->GetPhysicalMaterial())
		{
			PhysicalMaterialAttenuation = AbilitySource->GetPhysicalMaterialAttenuation(PhysMat, SourceTags, TargetTags);
		}
	}
	*/

	// 6. 최종 대미지 계산
	const float DamageDone = FMath::Max(
		BaseDamage * DistanceAttenuation * PhysicalMaterialAttenuation * DamageInteractionAllowedMultiplier,
		0.0f
	);

	UE_LOG(LogTestPlayMeleeDamage, Log, 
		TEXT("근접 대미지 계산: Base=%.1f, PhysMat=%.2f, TeamMult=%.1f, Final=%.1f, Target=%s"),
		BaseDamage, PhysicalMaterialAttenuation, DamageInteractionAllowedMultiplier, DamageDone,
		HitActor ? *HitActor->GetName() : TEXT("None"));

	// 7. Health Damage 속성에 대미지 적용
	if (DamageDone > 0.0f)
	{
		OutExecutionOutput.AddOutputModifier(
			FGameplayModifierEvaluatedData(
				ULyraHealthSet::GetDamageAttribute(),
				EGameplayModOp::Additive,
				DamageDone
			)
		);
	}
#endif // #if WITH_SERVER_CODE
}
