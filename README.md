# Lyra Modular Demo

Unreal Engine의 Lyra 샘플 프로젝트를 기반으로 GameFeature 시스템을 활용한 모듈형 게임플레이 데모입니다.  
MetaHuman으로 제작한 캐릭터를 Mutable 시스템으로 최적화하고 커스터마이징이 가능하도록 구현했습니다.

> **Note**: 본 리포지토리는 **소스코드 참고용** 및 **데모 시연용**입니다. 소스코드는 에셋이 제외되어 빌드가 불가능할 수 있으나, 아래 링크된 **데모 버전**을 통해 게임플레이를 **즉시 실행**해볼 수 있습니다.

---

## 데모 다운로드

GitHub Releases는 대용량 파일(LFS) 정책으로 인해 지원하지 않습니다. 아래 구글 드라이브 링크를 통해 최신 데모 버전을 다운로드하세요.

[🔗 데모 버전 다운로드 (Google Drive)](https://drive.google.com/drive/folders/1dKJwGT_gB_NFdVZ7NXoj9B8z4CLZC2T7?usp=sharing)

---

## 포함된 게임 모드
데모는 다음 두 가지 모드를 포함합니다:
1. **근접 전투 모드 (Melee Combat Mode)**: 자체 구현한 궤적 기반 근접 판정 시스템과 AI 전투를 체험할 수 있습니다.
2. **슈팅 모드 (Shooter Mode)**: Lyra 스타터 게임의 기본 슈팅 메커니즘을 경험할 수 있습니다.

---

## 기술 스택

| 영역 | 기술 |
|:---|:---|
| 아키텍처 | GameFeature 시스템 기반 모듈형 플러그인 설계 |
| 게임플레이 | Gameplay Tag Event, GAS(Gameplay Ability System), Enhanced Input 활용 |
| 네트워크 | Dedicated Server 지원 |
| 캐릭터 | MetaHuman 제작 + Mutable 최적화/커스터마이징 |
| 애니메이션 | Animation Retargeter를 통한 Lyra 애니메이션 연동 |
| 전투 시스템 | 궤적 기반 Box Sweep 근접 판정 + 블록 처리 |
| AI | Behavior Tree 기반 Bot 플레이어 |

---

## 실행 방법

1. 위 **구글 드라이브 링크**에서 최신 압축 파일(`LyraGame_Demo_Win64.zip`)을 다운로드합니다.
2. 압축을 해제한 후 `LyraGame.exe` (또는 `LyraStarterGame.exe`)를 실행합니다.
3. 메인 메뉴에서 **근접 전투 모드** 또는 **슈팅 모드**를 선택하여 플레이할 수 있습니다.

---

## 주요 구현 내역

### 근접 전투 시스템
- AGR 플러그인 분석을 기반으로 자체 구현
- 이전 프레임과 현재 프레임의 궤적을 계산하여 Sweep하는 방식 적용
- 히트 누락 방지 및 성능 최적화 달성

### AI Bot 시스템
- Behavior Tree 기반 근접 전투 AI 구현
- 적 탐색, 이동, 공격 패턴 처리

### 개발 환경
- AI 에이전트(Antigravity, VS Code Copilot) 활용
- Gemini, Claude, GPT 기반 프롬프트 엔지니어링

### 진행 상황 (WIP)
- 현재 근접 전투 시스템의 타격 및 피격 판정이 구현되어 있습니다.
- 콤보 시스템 및 스킬 연계 기능은 지속적으로 고도화 중입니다.
- AI 애니메이션 및 반응성 개선 작업이 진행 중입니다.

---

## 프로젝트 구조

```
Plugins/
├── GameFeatures/
│   └── TestPlay/              # 메인 게임플레이 로직
│       ├── AI/                # Bot Behavior Tree 노드
│       ├── Damage/            # 근접 대미지 처리
│       └── GameMode/          # 게임모드 컴포넌트
└── AdvancedMeleeTrace/        # 근접 판정 시스템
```

---

## 빌드 요구사항

- Unreal Engine 5.7 (Source Build)
- VS Code + Antigravity