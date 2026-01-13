# Unreal Engine Python Bridge MCP

이 문서는 AI 에이전트 및 개발자가 외부에서 실행 중인 언리얼 에디터(Unreal Editor)를 제어하기 위해 구축된 **Python Bridge MCP** 시스템에 대해 설명합니다.

## 1. 개요 (Overview)
Python Bridge는 외부 프로세스(예: AI 에이전트, IDE, 스크립트)와 언리얼 에디터 내부의 Python VM 간의 통신 채널입니다. 이를 통해 에디터 프로세스 내부에서만 접근 가능한 `unreal` 모듈의 기능을 외부에서 호출할 수 있습니다.

### 아키텍처
*   **서버 (Unreal Side)**: `Content/Python/init_unreal.py`
    *   에디터 실행 시 자동 시작.
    *   **TCP 9999** 포트에서 수신 대기.
    *   요청을 받아 메인 스레드(Game Thread) 큐에 등록하고, `SlatePostTick`에서 안전하게 실행.
*   **클라이언트 (External Side)**: `Tools/MCP/lyra_bridge_mcp.py`
    *   JSON 형식의 명령을 TCP 소켓으로 전송.
    *   `execute` (코드 실행) 및 `evaluate` (표현식 평가) 모드 지원.

## 2. 파일 위치
*   **서버 스크립트**: `LyraStarterGame/Content/Python/init_unreal.py`
*   **클라이언트 도구**: `LyraStarterGame/Tools/MCP/lyra_bridge_mcp.py`

## 3. 사용 방법 (Usage)

### 3.1. 전제 조건
*   **PythonScriptPlugin**이 프로젝트에서 활성화되어 있어야 합니다.
*   언리얼 에디터가 실행 중이어야 합니다 (Output Log에 "Python Bridge Server initialized" 확인).

### 3.2. CLI 명령 예시
프로젝트 루트에서 다음 명령어를 실행하여 사용합니다. (언리얼에 내장된 Python 인터프리터 사용 권장)

**1. 표현식 평가 (Evaluate)**
값 확인, 상태 조회 등에 사용합니다.
```bash
# 엔진 버전 확인
c:\wz\UnrealEngine\Engine\Binaries\ThirdParty\Python3\Win64\python.exe Tools/MCP/lyra_bridge_mcp.py evaluate "str(unreal.SystemLibrary.get_engine_version())"

# 현재 열려있는 에셋 수 확인
c:\wz\UnrealEngine\Engine\Binaries\ThirdParty\Python3\Win64\python.exe Tools/MCP/lyra_bridge_mcp.py evaluate "len(unreal.EditorAssetLibrary.list_assets('/Game/'))"
```

**2. 코드 실행 (Execute)**
복잡한 로직, 에셋 변경, 저장 등에 사용합니다.
```bash
# 로그 출력
c:\wz\UnrealEngine\Engine\Binaries\ThirdParty\Python3\Win64\python.exe Tools/MCP/lyra_bridge_mcp.py execute "unreal.log('Hello from AI Agent!')"
```

## 4. 고급 사용법 (Advanced)
복잡한 자동화 스크립트를 작성할 때는 Python 코드를 문자열로 만들어 `execute` 명령으로 보냅니다. 줄바꿈을 포함한 긴 스크립트도 전송 가능합니다.

### 예시: 에셋 감사 스크립트 전송
```python
# 외부 파이썬 스크립트에서 호출 시
import json
import socket

code = """
import unreal
assets = unreal.EditorAssetLibrary.list_assets('/Game/MyFolder')
for asset in assets:
    unreal.log(asset)
"""

request = {"command": "execute", "code": code}
# ... 소켓 전송 로직 ...
```

## 5. 주의사항
*   **스레드 안전성**: 모든 명령은 `init_unreal.py` 내부의 큐 시스템을 통해 **메인 스레드(Game Thread)**에서 실행되므로, `unreal` API 호출 시 스레드 충돌 안전성이 보장됩니다.
*   **보안**: 이 브리지는 로컬호스트(`127.0.0.1`) 연결만 허용하도록 설계되었습니다. 외부 네트워크에 노출되지 않도록 주의하십시오.
