# 언리얼 에디터 Python Bridge MCP 구현 계획

## 목표
AI 에이전트가 MCP(Model Context Protocol)를 통해 실행 중인 **언리얼 에디터 환경 내부**에서 Python 명령과 스크립트를 직접 실행할 수 있는 통신 브리지를 구축합니다.

## 배경
사용자는 리소스 사용량 최적화, 에셋 감사 등의 작업을 자동화하기 위해 언리얼 에디터를 제어하고자 합니다. 언리얼 엔진의 Python API(`unreal` 모듈)는 에디터 프로세스 내부에서만 접근이 가능하므로, 외부 에이전트와 통신하기 위한 브리지(Bridge)가 필요합니다.

## 아키텍처

```mermaid
graph LR
    Agent[AI 에이전트 / Cursor] -- MCP (표준 입출력) --> MCPServer[외부 MCP 서버 (Python)]
    MCPServer -- TCP 소켓 (포트 9999) --> UnrealServer[언리얼 내부 소켓 서버]
    UnrealServer -- unreal.py --> Editor[언리얼 엔진 에디터]
```

## 변경 제안 사항

### 1. 언리얼 엔진 측 (TCP 서버)
*   **파일**: `LyraStarterGame/Content/Python/init_unreal.py` (없을 경우 생성)
*   **기능**:
    *   에디터 로딩 시 포트 **9999**번에서 수신 대기하는 TCP 서버 스레드를 자동으로 시작합니다.
    *   JSON 형식의 요청을 수신합니다: `{"command": "execute", "code": "..."}` 또는 `{"command": "evaluate", "expression": "..."}`.
    *   메인 스레드(Game Thread)에서 안전하게 코드를 실행하기 위해 `unreal.register_slate_post_tick_callback` 또는 유사한 메커니즘을 활용하여 큐에 작업을 등록하고 실행합니다.
    *   실행 결과(Output) 또는 반환 값을 JSON으로 직렬화하여 소켓을 통해 반환합니다.

### 2. 외부 MCP 측 (클라이언트)
*   **파일**: `LyraStarterGame/Tools/MCP/lyra_bridge_mcp.py`
*   **기능**:
    *   `mcp` Python 패키지 또는 표준 입출력(Stdio) 방식을 사용하여 MCP 프로토콜을 구현합니다.
    *   제공 도구(Tools):
        *   `execute_unreal_python(code: str)`: 임의의 Python 코드를 언리얼 에디터에서 실행.
        *   `get_unreal_objects(class_name: str)`: 특정 클래스의 객체 목록 조회 (편의 기능).
    *   `localhost:9999`에 접속하여 명령을 전송하고 응답을 받습니다.

## 검증 계획
1.  프로젝트에서 `PythonScriptPlugin`이 활성화되어 있는지 확인합니다 (이미 활성화됨).
2.  `init_unreal.py` 파일을 작성하여 배치합니다.
3.  언리얼 에디터를 재시작하여 "Python Socket Server Listening on 9999" 로그가 출력되는지 확인합니다 (사용자 수행).
4.  `lyra_bridge_mcp.py`를 수동으로 실행하여 연결 및 명령 실행(예: `print("Hello from MCP")`)을 테스트합니다.
5.  사용자의 IDE(Cursor 등) 설정에 MCP 서버를 등록합니다 (사용자 수행).

## 보안 유의사항
*   이 소켓 서버는 원격 코드 실행(RCE)이 가능하므로, 반드시 로컬호스트(`127.0.0.1`)에서의 연결만 허용해야 합니다.
