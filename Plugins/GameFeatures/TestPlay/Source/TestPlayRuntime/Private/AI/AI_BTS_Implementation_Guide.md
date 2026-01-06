# Lyra AI Behavior Tree Services C++ 구현 가이드

이 문서는 Lyra 프로젝트에서 AI Behavior Tree Services를 C++로 구현하는 방법을 설명합니다.
다른 AI 에이전트가 이 문서를 참고하여 동일한 로직을 구현할 수 있도록 작성되었습니다.

---

## 목차

1. [개요](#개요)
2. [프로젝트 구조](#프로젝트-구조)
3. [핵심 개념](#핵심-개념)
4. [구현된 BTS 클래스](#구현된-bts-클래스)
5. [중요한 트러블슈팅](#중요한-트러블슈팅)
6. [Behavior Tree 설정](#behavior-tree-설정)
7. [테스트 및 디버깅](#테스트-및-디버깅)

---

## 개요

### 목적
블루프린트로 구현된 AI Behavior Tree Services를 Native C++로 포팅하여 성능 최적화 및 유지보수성 향상.

### 구현된 서비스
| 서비스 | 역할 |
|--------|------|
| `UBTS_TestPlayShoot` | 타겟이 유효하면 사격 어빌리티 활성화 |
| `UBTS_TestPlayReloadWeapon` | 탄약 부족 시 재장전 어빌리티 활성화 |
| `UBTS_TestPlayCheckAmmo` | 현재 무기 탄약 확인 및 블랙보드 업데이트 |
| `UBTS_TestPlaySetFocus` | 타겟을 향해 AI 시선 고정 |

---

## 프로젝트 구조

```
LyraStarterGame/
└── Plugins/GameFeatures/TestPlay/
    └── Source/TestPlayRuntime/
        ├── Public/AI/
        │   ├── TestPlayAIConstants.h    # 블랙보드 키 & 태그 상수
        │   ├── BTS_TestPlayShoot.h
        │   ├── BTS_TestPlayReloadWeapon.h
        │   ├── BTS_TestPlayCheckAmmo.h
        │   └── BTS_TestPlaySetFocus.h
        └── Private/AI/
            ├── BTS_TestPlayShoot.cpp
            ├── BTS_TestPlayReloadWeapon.cpp
            ├── BTS_TestPlayCheckAmmo.cpp
            └── BTS_TestPlaySetFocus.cpp
```

### Build.cs 필수 의존성

```cpp
PublicDependencyModuleNames.AddRange(new string[] {
    "AIModule",
    "GameplayAbilities",
    "GameplayTags",
    "GameplayTasks",
    "LyraGame"
});
```

---

## 핵심 개념

### 1. 블랙보드 키 상수화

하드코딩을 피하고 오타를 방지하기 위해 상수 헤더 파일 사용:

```cpp
// TestPlayAIConstants.h
namespace TestPlayAIKeys
{
    static const FName TargetEnemy = FName("TargetEnemy");
    static const FName OutOfAmmo = FName("OutOfAmmo");
    static const FName SelfActor = FName("SelfActor");
    static const FName MoveGoal = FName("MoveGoal");
}
```

### 2. 어빌리티 태그 vs 입력 태그

> [!CAUTION]
> **가장 중요한 개념**: `TryActivateAbilitiesByTag`는 **AbilityTags**를 기준으로 검색합니다.
> InputTag가 아닙니다!

```cpp
// ❌ 잘못된 사용 - InputTag로 검색 시도
static const FName InputTag_Weapon_Fire = FName("InputTag.Weapon.Fire");

// ✅ 올바른 사용 - AbilityTags로 검색
static const FName AbilityTag_Weapon_Fire = FName("Ability.Type.Action.WeaponFire");
```

어빌리티 블루프린트에서 확인할 수 있는 태그:
- **어빌리티 트리거 및 캐시에드 태그** (AbilityTags) ← 이것을 사용!
- DynamicSpecSourceTags (InputTag) ← TryActivateAbilitiesByTag에서 사용 불가

### 3. Lyra 컴포넌트 접근 패턴

```cpp
// AI Controller에서 시작
AAIController* AIC = OwnerComp.GetAIOwner();
APawn* Pawn = AIC->GetPawn();

// LyraCharacter로 캐스팅
ALyraCharacter* LyraChar = Cast<ALyraCharacter>(Pawn);

// Lyra ASC 획득 (핵심!)
ULyraAbilitySystemComponent* LyraASC = LyraChar->GetLyraAbilitySystemComponent();

// 장비 매니저로 무기 확인
ULyraEquipmentManagerComponent* EquipmentComp = 
    LyraChar->FindComponentByClass<ULyraEquipmentManagerComponent>();
ULyraRangedWeaponInstance* RangedWeapon = 
    EquipmentComp->GetFirstInstanceOfType<ULyraRangedWeaponInstance>();
```

---

## 구현된 BTS 클래스

### UBTS_TestPlayShoot (사격 서비스)

**역할**: 타겟이 유효하고 무기가 있을 때 사격 어빌리티 활성화

**핵심 로직**:
```cpp
void UBTS_TestPlayShoot::TickNode(...)
{
    // 1. 블랙보드에서 타겟 확인
    AActor* TargetActor = Cast<AActor>(
        BlackboardComp->GetValueAsObject(TestPlayAIKeys::TargetEnemy));
    
    // 2. 무기 확인
    ULyraRangedWeaponInstance* RangedWeapon = 
        EquipmentComp->GetFirstInstanceOfType<ULyraRangedWeaponInstance>();
    
    // 3. 사격 어빌리티 활성화 (AbilityTags 사용!)
    FGameplayTag FireTag = FGameplayTag::RequestGameplayTag(
        FName("Ability.Type.Action.WeaponFire"), false);
    
    FGameplayTagContainer FireTags;
    FireTags.AddTag(FireTag);
    LyraASC->TryActivateAbilitiesByTag(FireTags);
}
```

**설정 값**:
- `Interval`: 0.2초 (사격은 빠른 반응 필요)
- `bNotifyTick`: true
- `bNotifyCeaseRelevant`: true

### UBTS_TestPlaySetFocus (포커스 서비스)

**역할**: AI가 타겟을 바라보도록 설정

**핵심 로직**:
```cpp
void UBTS_TestPlaySetFocus::TickNode(...)
{
    AActor* TargetActor = GetTargetFromBlackboard();
    
    if (IsValid(TargetActor))
    {
        AIController->SetFocus(TargetActor);
    }
    else
    {
        AIController->ClearFocus(EAIFocusPriority::Gameplay);
    }
}
```

---

## 구현된 BTDecorator 클래스

### BTDecorator_TestPlayCheckCanMove (이동 가능 체크)

**역할**: MoveTo 태스크 실행 전 캐릭터의 이동 가능 상태 확인

**체크 항목**:
- `CharacterMovementComponent.MovementMode != MOVE_None`
- `PathFollowingComponent` 유효성 및 상태

**핵심 로직**:
```cpp
bool UBTDecorator_TestPlayCheckCanMove::CalculateRawConditionValue(...) const
{
    // CharacterMovementComponent 획득
    UCharacterMovementComponent* MovementComp = Character->GetCharacterMovement();
    
    // MovementMode 체크 - MOVE_None이면 이동 불가
    if (MovementComp->MovementMode == MOVE_None)
    {
        return false;
    }
    
    // PathFollowing 상태 체크 (선택적)
    if (bCheckPathFollowing)
    {
        UPathFollowingComponent* PathFollowingComp = AIController->GetPathFollowingComponent();
        if (PathFollowingComp->GetStatus() == EPathFollowingStatus::Paused)
        {
            return false;
        }
    }
    
    return true;
}
```

**설정 옵션**:
| 옵션 | 기본값 | 설명 |
|------|--------|------|
| `bRequireWalkingMode` | false | true면 Walking 모드만 허용 |
| `bCheckPathFollowing` | true | PathFollowing 컴포넌트 상태 체크 |

**사용 예시**:
```
Exploration Sequence
├── Decorator: Check Can Move  ← 추가
├── Find Patrol Pos
└── Move To
```

---

## 중요한 트러블슈팅

### 문제 1: ProcessAbilityInput 배열 순회 충돌

**증상**: `Array has changed during ranged-for iteration!` 에러

**원인**: BT Service에서 `ProcessAbilityInput` 직접 호출

**해결**: `TryActivateAbilitiesByTag` 사용 (안전한 어빌리티 활성화)

```cpp
// ❌ 문제 발생
LyraASC->AbilityInputTagPressed(FireTag);
LyraASC->ProcessAbilityInput(DeltaSeconds, false);

// ✅ 해결
FGameplayTagContainer FireTags;
FireTags.AddTag(FireTag);
LyraASC->TryActivateAbilitiesByTag(FireTags);
```

### 문제 2: 스폰 보호 태그로 사격 불가

**증상**: `TryActivateAbilitiesByTag 실패` + ASC에 `Status.SpawningIn` 태그 존재

**원인**: 스폰 보호 GameplayEffect가 적용되어 있음

**해결**: Experience에 `AbilitySet_Elimination` 추가하여 스폰 보호 종료 로직 활성화

### 문제 3: 잘못된 태그로 어빌리티 검색

**증상**: Fire 어빌리티가 있지만 활성화되지 않음

**원인**: `TryActivateAbilitiesByTag`에 `InputTag` 사용 (AbilityTags 필요)

**해결**: 
```cpp
// 어빌리티 블루프린트의 AbilityTags 확인 후 동일한 태그 사용
FName("Ability.Type.Action.WeaponFire")
```

---

## Behavior Tree 설정

### 권장 구조

```
Root
└── Selector
    └── Sequence (사격 분기)
        ├── Decorator: Has Valid Target
        ├── Service: BTS_TestPlaySetFocus
        ├── Service: BTS_TestPlayShoot
        └── Task: Wait (3-5초)  ← 중요! 서비스 활성 유지
```

> [!IMPORTANT]
> **Wait 태스크 필수**: 서비스 노드는 부모 Task가 실행 중일 때만 활성화됩니다.
> Task가 없거나 빠르게 완료되면 서비스가 계속 활성화/비활성화를 반복합니다.

### 블랙보드 키 설정

| 키 이름 | 타입 | 설명 |
|---------|------|------|
| TargetEnemy | Object (Actor) | 현재 타겟 적 |
| OutOfAmmo | Bool | 탄약 부족 상태 |
| SelfActor | Object (Actor) | AI 자신 |
| MoveGoal | Vector | 이동 목표 위치 |

---

## 테스트 및 디버깅

### 로그 카테고리 정의

```cpp
DEFINE_LOG_CATEGORY_STATIC(LogTestPlayShoot, Log, All);
```

### 유용한 디버그 로그

```cpp
// 어빌리티 존재 확인
for (const FGameplayAbilitySpec& Spec : LyraASC->GetActivatableAbilities())
{
    if (Spec.Ability && Spec.AbilityTags.HasTag(FireTag))
    {
        UE_LOG(LogTestPlayShoot, Log, TEXT("어빌리티: %s, 활성: %s"),
            *Spec.Ability->GetClass()->GetName(),
            Spec.IsActive() ? TEXT("Yes") : TEXT("No"));
    }
}

// 현재 ASC 태그 출력
FGameplayTagContainer OwnedTags;
LyraASC->GetOwnedGameplayTags(OwnedTags);
UE_LOG(LogTestPlayShoot, Log, TEXT("ASC 태그: %s"), *OwnedTags.ToString());
```

### Output Log 필터링

```
LogTestPlayShoot
```

---

## 요약: AI가 기억해야 할 핵심 사항

1. **TryActivateAbilitiesByTag**는 **AbilityTags**를 사용 (InputTag X)
2. **ProcessAbilityInput** 직접 호출 금지 (배열 충돌)
3. **Wait 태스크**로 서비스 활성 상태 유지
4. **스폰 보호 태그** 확인 (`Status.SpawningIn`)
5. **LyraAbilitySystemComponent** 사용 (`UAbilitySystemComponent` 아님)
6. 어빌리티 블루프린트에서 **실제 AbilityTags 값 확인** 필수
7. **MoveTo 실행 전** `BTDecorator_TestPlayCheckCanMove`로 이동 가능 상태 체크

---

*문서 작성일: 2024-12-21*
*Lyra 버전: UE5 Lyra Starter Game*
