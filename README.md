# Lyra Modular Demo

Unreal Engine의 Lyra 샘플 프로젝트를 기반으로 GameFeature 시스템을 활용한 모듈형 게임플레이 데모입니다.  
MetaHuman으로 제작한 캐릭터를 Mutable 시스템으로 최적화하고 커스터마이징이 가능하도록 구현했습니다.

---

## 기술 스택

| 영역 | 기술 |
|:---|:---|
| 아키텍처 | GameFeature 시스템 기반 모듈형 플러그인 설계 |
| 네트워크 | Dedicated Server 지원 |
| 캐릭터 | MetaHuman 제작 + Mutable 최적화/커스터마이징 |
| 애니메이션 | Animation Retargeter를 통한 Lyra 애니메이션 연동 |
| 전투 시스템 | 궤적 기반 Box Sweep 근접 판정 + 블록 처리 |
| AI | Behavior Tree 기반 Bot 플레이어 |

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