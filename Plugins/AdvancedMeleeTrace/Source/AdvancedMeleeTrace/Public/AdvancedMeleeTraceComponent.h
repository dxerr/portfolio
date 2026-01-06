#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "AdvancedMeleeTraceComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAdvancedMeleeTrace, Log, All);



USTRUCT(BlueprintType)
struct FMeleeTraceInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	FName StartSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	FName EndSocketName;

	/** 소켓의 전방(X) 방향으로 뻗어나가는 길이 (검의 날 너비 등) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	float Radius = 30.0f;

	/** (Optional) 찾고자 하는 메시 컴포넌트의 Component Tag. 비어있으면 소켓 이름으로 자동 검색. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee")
	FName MeshTag;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMeleeHit, AActor*, HitActor, const FHitResult&, HitResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMeleeHitBlocked, AActor*, HitActor, const FHitResult&, HitResult, FVector, TraceDirection);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDMELEETRACE_API UAdvancedMeleeTraceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAdvancedMeleeTraceComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** 트레이스 시작 */
	UFUNCTION(BlueprintCallable, Category = "Melee Trace")
	void StartTrace(const TArray<FMeleeTraceInfo>& TraceInfos);

	/** 트레이스 종료 */
	UFUNCTION(BlueprintCallable, Category = "Melee Trace")
	void EndTrace();

	/** 매 틱(혹은 NotifyTick)마다 호출되어 트레이스를 수행 */
	UFUNCTION(BlueprintCallable, Category = "Melee Trace")
	void PerformTrace();

	/** 적중 시 이벤트 (서버/클라 모두 발생 가능, 로직에 따라 다름) */
	UPROPERTY(BlueprintAssignable, Category = "Melee Trace")
	FOnMeleeHit OnMeleeHit;

protected:
	bool bIsTracing;

	// 내부적으로 트레이스 상태를 관리하는 구조체
	struct FActiveMeleeTrace
	{
		FMeleeTraceInfo Info;
		TWeakObjectPtr<UPrimitiveComponent> MeshComponent;
		
		// 4개의 트레이스 포인트 (이전 프레임 위치)
		// 0: Start, 1: End, 2: Start+Ext, 3: End+Ext
		FVector PrevPoints[4]; 
	};
	
	/**
	 * 다중 히트 처리 여부.
	 * True: 한 번의 스윙으로 여러 적을 타격 (Default)
	 * False: 한 번의 스윙으로 가장 먼저 맞은 적 하나만 타격
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace")
	bool bProcessMultiHit = true;

	/** 검 길이 방향(StartSocket -> EndSocket)의 트레이스 분할 개수 (Deprecated: Box Sweep 사용으로 인해 미사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Optimization", meta = (ClampMin = "1", DeprecatedProperty, DeprecationMessage="No longer used with Box Sweep logic"))
	int32 TraceResolutionLength = 3;

	/** 검 너비 방향(Radius)의 트레이스 분할 개수 (Deprecated: Box Sweep 사용으로 인해 미사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace|Optimization", meta = (ClampMin = "1", DeprecatedProperty, DeprecationMessage="No longer used with Box Sweep logic"))
	int32 TraceResolutionWidth = 2;

protected:
	UPROPERTY()
	TArray<TObjectPtr<AActor>> HitActors;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> BlockedActors;

	// 현재 활성화된 트레이스 목록
	TArray<FActiveMeleeTrace> ActiveTraces;

	// 트레이스 설정 정보 저장 (Late Binding 지원용)
	UPROPERTY()
	TArray<FMeleeTraceInfo> CurrentTraceInfos;

	// 내부 로직 분리: 트레이스 활성화/바인딩 시도
	void SetupActiveTraces();

	// 내부 로직 분리: 궤적 및 형상 계산
	bool UpdateTracePoints(FActiveMeleeTrace& Trace, FVector& OutPrevCenter, FVector& OutCurrentCenter, FQuat& OutRot, FVector& OutExtent);

	// 내부 로직 분리: 충돌 스윕 수행
	void PerformSweep(const FVector& PrevCenter, const FVector& CurrentCenter, const FQuat& Rot, const FVector& Extent, TArray<FHitResult>& OutHits);

	// 소켓 이름을 가진 메시 컴포넌트를 재귀적으로 찾음 (MeshTag가 있으면 우선 검색)
	UPrimitiveComponent* FindMeshWithSocket(FName SocketName, FName MeshTag = NAME_None);

public:
	UPROPERTY(EditAnywhere, Category = "Melee Trace")
	bool bDebugDraw = false;

protected:

	UPROPERTY(EditAnywhere, Category = "Melee Trace")	
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	/**
	 * 트레이스에 사용할 오브젝트 타입 목록입니다.
	 * 이 배열에 요소가 하나라도 있다면, TraceChannel 대신 이 목록에 있는 모든 오브젝트 타입을 감지합니다.
	 * (예: Pawn, WorldDynamic, PhysicsBody 등)
	 * 
	 * Shield와 같은 특정 오브젝트가 Pawn 채널을 블록하지 않으면서도(이동 방해 X)
	 * 공격 트레이스에는 감지되게 하려면, Shield의 오브젝트 타입을 여기에 추가하세요.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melee Trace")
	TArray<TEnumAsByte<EObjectTypeQuery>> TraceObjectTypes;
	

	/**
	 * 선택된 콜리전 채널을 사용하여 충돌을 필터링합니다.	 
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Melee Trace|Filtering")
	TEnumAsByte<ECollisionChannel> BlockingChannel = ECollisionChannel::ECC_MAX;

public:
	/**
	 * 필터링에 의해 차단된 히트 발생 시 호출되는 이벤트
	 */
	UPROPERTY(BlueprintAssignable, Category = "Melee Trace")
	FOnMeleeHitBlocked OnMeleeHitBlocked;

protected:

	/** 서버 RPC: 클라이언트에서 감지한 히트를 서버 검증 */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerValidateHit(AActor* HitActor, const FHitResult& HitResult);

	enum class EProcessHitResult : uint8
	{
		Ignored,
		Hit,
		Blocked
	};

	EProcessHitResult ProcessHit(const FHitResult& Hit, const FVector& TraceDirection);
};
