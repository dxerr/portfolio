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

	/** 에디터 미리보기를 위한 무기 Actor 블루프린트 클래스 (PreviewWeaponMesh 대신 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	TSubclassOf<AActor> PreviewWeaponActorClass;

	/** 무기가 부착될 캐릭터의 소켓 이름 (예: hand_r) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	FName WeaponAttachmentSocket;

#if WITH_EDITORONLY_DATA
protected:
	/** 프리뷰 무기 스폰/캐싱을 처리하는 헬퍼 함수 */
	void EnsurePreviewWeaponSpawned(USkeletalMeshComponent* MeshComp);

	/** 캐싱된 프리뷰 컴포넌트 또는 액터 참조 */
	UPROPERTY(Transient)
	TWeakObjectPtr<USceneComponent> CachedPreviewComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CachedPreviewActor;

	/** 캐싱된 프리뷰의 원본 에셋 (변경 감지용) */
	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> CachedPreviewSourceAsset;

	/** 캐싱된 프리뷰의 소켓 (변경 감지용) */
	UPROPERTY(Transient)
	FName CachedPreviewSocket;
#endif
};
