# Unreal Engine Python Bridge MCP 시스템 문서

## 개요
**Python Bridge**는 외부의 AI 에이전트나 도구가 실행 중인 언리얼 에디터(Unreal Editor)의 Python 환경에 직접 접근하여 코드를 실행할 수 있게 해주는 시스템입니다. 이를 통해 자동화 작업, 에셋 관리, 상태 쿼리 등을 자연어 명령으로 수행할 수 있습니다.

## 시스템 구성 요소

### 1. MCP Bridge Server (`lyra_bridge_mcp.py`)
AI 에이전트와 직접 통신하는 MCP(Model Context Protocol) 서버입니다.
*   **위치**: `Tools/MCP/lyra_bridge_mcp.py`
*   **역할**:
    *   MCP `execute_unreal_python` 도구 제공.
    *   사용자의 Python 코드 요청을 JSON 패킷으로 포장.
    *   TCP 소켓(Localhost:9999)을 통해 언리얼 에디터로 전송.
    *   응답(Stdout/Stderr)을 받아 사용자에게 반환.

### 2. Unreal Internal Server (`init_unreal.py`)
언리얼 에디터 내부에서 동작하는 TCP 수신 서버입니다.
*   **위치**: `Content/Python/init_unreal.py`
*   **역할**:
    *   에디터 시작 시 포트 **9999** 바인딩.
    *   백그라운드 스레드에서 요청 대기.
    *   **Main Thread Queueing**: 받은 코드를 `SlatePostTick` 콜백 큐에 넣어, 메인 스레드(GameThread)에서 안전하게 실행되도록 보장.
    *   실행 결과를 캡처하여 클라이언트로 반환.

---

## 사용법 (Available Tools)

### `execute_unreal_python`
언리얼 에디터 컨텍스트에서 임의의 Python 코드를 실행합니다.

*   **Arguments**:
    *   `code` (string): 실행할 유효한 Python 코드 블록. `unreal` 모듈은 이미 임포트되어 있습니다.
*   **Returns**:
    *   실행 결과(Stdout 출력값) 또는 에러 메시지.

**예시 요청:**
```python
# 에디터의 모든 액터 개수 세기
import unreal
actors = unreal.EditorLevelLibrary.get_all_level_actors()
print(f"Total Actors: {len(actors)}")
```

**예시 응답:**
```text
Total Actors: 42
```

---

## 통신 프로토콜 (Internal)

MCP 브리지와 언리얼 서버 간의 저수준 TCP 통신 포맷입니다.

**Request (JSON):**
```json
{
    "code": "print('Hello')"
}
```

**Response (JSON):
```json
{
    "status": "ok",   // 또는 "error"
    "output": "Hello\n",
    "error": ""       // 에러 발생 시 상세 메시지
}
```

## 트러블슈팅

### 1. "Connection Refused" 에러
*   **원인**: 언리얼 에디터가 실행 중이 아니거나, `init_unreal.py`가 로드되지 않았습니다.
*   **해결**:
    *   에디터를 실행하세요.
    *   Output Log에서 "Python Bridge Server listening..." 메시지가 있는지 확인하세요.
    *   Port 9999가 방화벽이나 다른 프로세스에 의해 차단되지 않았는지 확인하세요.

### 2. "Python was not found" 에러 (MCP 서버 실행 시)
*   **원인**: 시스템 Python 경로 설정 문제.
*   **해결**: `settings.json` 설정에서 `unreal-python-bridge`의 명령어를 언리얼 내장 Python 경로(`.../Binaries/ThirdParty/Python3/Win64/python.exe`)로 지정하세요.

### 3. 언리얼 에디터 멈춤 (Freeze)
*   **원인**: 실행한 Python 코드가 무한 루프에 빠지거나 너무 무거운 작업을 수행함.
*   **주의**: 모든 코드는 에디터의 메인 스레드에서 실행되므로, 긴 작업은 에디터 UI를 멈추게 할 수 있습니다. 복잡한 작업은 짧게 나누어 실행하세요.

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
