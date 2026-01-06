#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AdvancedMeleeTraceComponent.h"
#include "AnimNotifyState_MeleeTrace.generated.h"

/**
 * PDF Requirement: Core Class `UAnimNotifyState_MeleeTrace`
 */
UCLASS()
class ADVANCEDMELEETRACE_API UAnimNotifyState_MeleeTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	TArray<FMeleeTraceInfo> TraceInfos;

	/** 에디터 미리보기(페르소나 등)에서 디버그 라인을 표시할지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bDebugDraw = true;

	/** 에디터 미리보기를 위한 무기 메시 (StaticMesh 또는 SkeletalMesh) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug", meta = (AllowedClasses = "/Script/Engine.StaticMesh,/Script/Engine.SkeletalMesh"))
	TObjectPtr<UObject> PreviewWeaponMesh;

	/** 무기가 부착될 캐릭터의 소켓 이름 (예: hand_r) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	FName WeaponAttachmentSocket;
};
