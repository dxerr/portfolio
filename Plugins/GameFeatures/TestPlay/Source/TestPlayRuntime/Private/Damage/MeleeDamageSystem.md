# TestPlay 근접 공격 대미지 시스템 (Melee Damage System)

LyraStarterGame의 GameplayAbilitySystem(GAS)과 연동하여 근접 공격(Melee)의 판정 및 대미지 처리를 수행하는 시스템입니다.

## 1. 시스템 개요

이 시스템은 애니메이션 노티파이를 통해 트레이스(Trace)를 수행하고, 충돌이 감지되면 자동으로 GameplayEffect(GE)를 적용하여 대미지를 입힙니다. Lyra의 `ULyraDamageExecution`을 기반으로 하지만, 근접 공격에 불필요한 거리 감쇠 등을 제외하고 커스텀화되었습니다.

### 주요 구성 요소

| 클래스 | 경로 | 역할 |
|---|---|---|
| **UAdvancedMeleeTraceComponent** | `Plugins/AdvancedMeleeTrace` | 소켓/메시 기반 트레이스 수행, `OnMeleeHit` 델리게이트 송출 |
| **UTestPlayMeleeDamageHandler** | `TestPlayRuntime/Private/Damage` | `OnMeleeHit` 이벤트를 받아 대미지 GE를 타겟에게 적용 |
| **UTestPlayMeleeDamageExecution** | `TestPlayRuntime/Private/Damage` | 실제 대미지 계산 로직 (BaseDamage, 팀 판정 등) |

---

## 2. 동작 흐름 (Data Flow)

1. **Animation Montage**: `UAnimNotifyState_MeleeTrace` 실행
2. **Trace Component**: `PerformTrace()`로 충돌 감지 → `OnMeleeHit` 델리게이트 브로드캐스트
3. **Damage Handler**: `HandleMeleeHit()`에서 이벤트 수신
    - 공격자(Source)와 피격자(Target)의 AbilitySystemComponent(ASC) 확보
    - `GameplayEffectContext` 생성 (HitResult 포함)
    - 지정된 `GameplayEffect` Spec 생성 및 `ApplyGameplayEffectSpecToTarget` 호출
4. **Damage Execution**: `UTestPlayMeleeDamageExecution` 실행
    - BaseDamage 캡처
    - 팀 식별 (아군 오인 사격 방지)
    - 최종 대미지를 `LyraHealthSet.Damage` 속성에 적용

---

## 3. 사용 방법 (Setup Guide)

### 3.1. 캐릭터 설정 (Blueprint)
캐릭터 블루프린트에 다음 두 컴포넌트를 추가합니다:

1. **AdvancedMeleeTraceComponent**: 트레이스 설정 (소켓 이름, 반경 등)
2. **TestPlayMeleeDamageHandler**: 대미지 핸들링 설정
    - **Melee Damage Effect**: 적용할 GE 선택 (아래 3.2 참고)
    - **Base Damage**: 기본 대미지 값 설정

### 3.2. GameplayEffect (GE) 생성
1. `GameplayEffect`를 상속받는 블루프린트 생성 (예: `GE_MeleeDamage`)
2. **Duration Policy**: `Instant`
3. **Executions** 섹션 추가:
    - **Calculation Class**: `UTestPlayMeleeDamageExecution`
4. 그 외 GameplayTag 등 필요한 설정 추가

---

## 4. 구현 상세 및 주의사항

### 4.1. Lyra API Export 제한 우회
Lyra의 일부 코어 클래스(`FLyraGameplayEffectContext` 등)의 멤버 함수가 `LYRAGAME_API`로 export되지 않아 외부 모듈에서 호출 시 **Linker Error (LNK2019)**가 발생합니다.
이를 해결하기 위해 다음 기능을 제한적으로 사용하거나 우회했습니다:

- `SetAbilitySource`: 호출 생략 (기본값 사용)
- `GetAbilitySource`, `GetPhysicalMaterial`: 호출 생략 (거리 감쇠 및 물리기반 대미지 계산 제외)

> **참고**: 근접 공격은 보통 거리 감쇠가 필요 없으므로 게임플레이에 미치는 영향은 미미합니다.

### 4.2. 팀 판정 (Team Check)
`ULyraTeamSubsystem`을 사용하여 공격자와 피격자가 같은 팀인지 확인합니다. 같은 팀일 경우 대미지 계수가 0이 되어 피해를 입지 않습니다.

### 4.3. 네트워크 (Network)
- **Trace**: 클라이언트/서버 모두 수행되지만, 대미지 적용(`HandleMeleeHit`)은 **서버(Authority)**에서만 실행되도록 `HasAuthority()` 체크가 되어 있습니다.
- **Validation**: `UAdvancedMeleeTraceComponent`에는 클라이언트 히트를 서버가 검증하는 로직이 포함되어 있습니다.

### 4.4. 히트 필터링 및 차단 (Hit Filtering & Blocking)
`UAdvancedMeleeTraceComponent`에는 특정 오브젝트와의 충돌 시 대미지 처리를 차단하고 별도 이벤트를 발생시키는 필터링 기능이 포함되어 있습니다. **블로킹 오브젝트와 충돌 시 해당 트레이스는 즉시 중단됩니다.**

- **BlockingTags**: 이 태그를 가진 액터나 컴포넌트와 충돌하면 대미지를 입히지 않고 트레이스를 중단합니다.
- **BlockingCollisionChannels**: 이 콜리전 채널(Object Type)을 가진 컴포넌트와 충돌하면 대미지를 입히지 않고 트레이스를 중단합니다.
- **OnMeleeHitBlocked**: 차단된 히트 발생 시 호출되는 델리게이트입니다. 반동 효과(Recoil) 등을 구현하는 데 사용됩니다.

> **작동 원리**: `LineTraceMulti`로 검출된 히트 목록을 순회하며, 블로킹 조건에 해당하는 오브젝트가 발견되면 즉시 처리를 중단합니다. 따라서 블로킹 오브젝트 뒤에 있는 적은 타격되지 않습니다.

### 4.5. 다중 히트 제어 (Multi-Hit Control)
`UAdvancedMeleeTraceComponent`의 `bProcessMultiHit` 속성을 통해 한 번의 공격으로 여러 대상을 타격할지 결정할 수 있습니다.

- **True (Default)**: 검출된 모든 유효 타겟에 대해 이벤트를 발생시킵니다. (광역 공격)
- **False**: 검출된 타겟 중 가장 먼저 맞은(가장 가까운) 하나만 처리하고 트레이스를 중단합니다. (단일 공격)
