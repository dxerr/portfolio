# 언리얼 에디터 Python Bridge MCP 구축 가이드

## 1. 개요
이 문서는 외부 AI 에이전트(Cursor, Claude Desktop 등)가 실행 중인 언리얼 에디터(Unreal Editor)를 제어할 수 있도록 **TCP 소켓 기반의 MCP 브리지**를 구축하는 방법을 설명합니다. 이 가이드를 통해 다른 환경에서도 동일한 시스템을 재구축할 수 있습니다.

## 2. 아키텍처

```mermaid
graph LR
    User[사용자 / AI Agent] -- MCP Protocol --> MCPClient[lyra_bridge_mcp.py (FastMCP)]
    MCPClient -- TCP Socket (JSON) --> UnrealServer[init_unreal.py (Editor Process)]
    UnrealServer -- GameThread Task --> UnrealEngine[Unreal Python VM]
```

*   **Unreal Server**: 에디터 내부에서 실행되는 TCP 서버. 메인 스레드 틱(Tick)에 동기화하여 Python 코드를 실행합니다.
*   **MCP Client**: 외부에서 실행되는 MCP 서버. 사용자의 요청을 받아 언리얼 서버로 전달합니다.

---

## 3. 구축 단계 (Step-by-Step)

### 단계 1: Unreal Internal Server 설치
언리얼 에디터가 실행될 때 자동으로 로드되는 서버 스크립트입니다.

*   **파일 경로**: `[ProjectRoot]/Content/Python/init_unreal.py`
    *   *참고: `Content/Python` 폴더가 없다면 생성하세요.*
*   **핵심 기능**:
    *   TCP 9999 포트 바인딩 (Localhost Only)
    *   `unreal.register_slate_post_tick_callback`을 사용한 메인 스레드 실행 보장
    *   `exec()` 함수로 전달받은 Python 코드 실행 및 Stdout/Stderr 캡처

### 단계 2: MCP Bridge Client 설치
외부 AI와 언리얼을 연결하는 매개체입니다.

*   **파일 경로**: `[ProjectRoot]/Tools/MCP/lyra_bridge_mcp.py`
*   **필수 라이브러리 설치**:
    언리얼 엔진에 내장된 Python 인터프리터를 사용하여 `mcp` 패키지를 설치해야 합니다.
    ```powershell
    # 언리얼 내장 Python 경로 (Win64 기준)
    c:\wz\UnrealEngine\Engine\Binaries\ThirdParty\Python3\Win64\python.exe -m pip install mcp
    ```
*   **핵심 기능**:
    *   `FastMCP` 서버 초기화
    *   `execute_unreal_python` 도구 노출
    *   소켓 통신 (타임아웃 5초)

### 단계 3: MCP 서버 설정 등록
AI 에이전트(Antigravity, Cursor 등)가 이 MCP 서버를 인식하도록 설정 파일(`settings.json` 등)에 등록합니다.

*   **Command**: 언리얼 내장 Python 실행 파일 절대 경로
*   **Args**: `lyra_bridge_mcp.py`의 절대 경로

**설정 예시 (JSON):**
```json
"unreal-python-bridge": {
    "command": "c:/wz/UnrealEngine/Engine/Binaries/ThirdParty/Python3/Win64/python.exe",
    "args": [
        "d:/wz/LyraStarterGame/Tools/MCP/lyra_bridge_mcp.py"
    ]
}
```

---

## 4. 검증 방법

1.  **에디터 실행**: 언리얼 에디터를 실행(또는 재시작)합니다.
2.  **로그 확인**: Output Log 패널에서 `Python Bridge Server listening on 127.0.0.1:9999` 메시지를 확인합니다.
3.  **MCP 연결 테스트**: AI 에이전트에게 "언리얼에서 현재 레벨의 모든 액터 이름을 출력해줘"라고 요청하여 `execute_unreal_python` 도구가 호출되는지 확인합니다.

## 5. 보안 유의사항
*   **Localhost 바인딩**: `init_unreal.py`는 반드시 `127.0.0.1`에 바인딩하여 외부 접근을 차단해야 합니다.
*   **임의 코드 실행**: 이 시스템은 임의의 Python 코드를 실행할 수 있으므로, 신뢰할 수 있는 사용자/에이전트만 접근해야 합니다.

