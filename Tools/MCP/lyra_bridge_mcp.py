import sys
import socket
import json
import argparse

# Configuration
UNREAL_HOST = '127.0.0.1'
UNREAL_PORT = 9999
BUFFER_SIZE = 4096

def send_request(command, data_key, data_value):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((UNREAL_HOST, UNREAL_PORT))
            request = {"command": command, data_key: data_value}
            s.sendall(json.dumps(request).encode('utf-8'))
            
            response_data = b""
            while True:
                chunk = s.recv(BUFFER_SIZE)
                if not chunk:
                    break
                response_data += chunk
            
            return json.loads(response_data.decode('utf-8'))
    except ConnectionRefusedError:
        return {"status": "error", "message": "Connection refused. Is Unreal Editor running with init_unreal.py?"}
    except Exception as e:
        return {"status": "error", "message": str(e)}

def main():
    parser = argparse.ArgumentParser(description="MCP Bridge for Unreal Engine")
    subparsers = parser.add_subparsers(dest="mode", help="Mode of operation")

    # Execute mode
    exec_parser = subparsers.add_parser("execute", help="Execute Python code")
    exec_parser.add_argument("code", help="Python code to execute")

    # Evaluate mode
    eval_parser = subparsers.add_parser("evaluate", help="Evaluate Python expression")
    eval_parser.add_argument("expression", help="Python expression to evaluate")

    args = parser.parse_args()

    if args.mode == "execute":
        result = send_request("execute", "code", args.code)
        print(json.dumps(result, indent=2))
    elif args.mode == "evaluate":
        result = send_request("evaluate", "expression", args.expression)
        print(json.dumps(result, indent=2))
    else:
        # Default behavior: Simple interactive loop or MCP server mode (future)
        # For now, just print help
        parser.print_help()

if __name__ == "__main__":
    main()
