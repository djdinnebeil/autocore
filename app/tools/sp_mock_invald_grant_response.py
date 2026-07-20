from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json


HOST = "127.0.0.1"
PORT = 8786


class SpotifyMockHandler(BaseHTTPRequestHandler):
    def do_POST(self) -> None:
        if self.path != "/api/token":
            self.send_json_response(
                status_code=404,
                body={
                    "error": "not_found",
                    "error_description": "Mock endpoint not found",
                },
            )
            return

        content_length = int(self.headers.get("Content-Length", "0"))
        request_body = self.rfile.read(content_length).decode(
            "utf-8",
            errors="replace",
        )

        print("\nReceived Spotify token request")
        print(f"Path: {self.path}")
        print(f"Authorization: {self.headers.get('Authorization')}")
        print(f"Content-Type: {self.headers.get('Content-Type')}")
        print(f"Body: {request_body}")

        self.send_json_response(
            status_code=400,
            body={
                "error": "invalid_grant",
                "error_description": "Refresh token expired",
            },
        )

    def send_json_response(
        self,
        status_code: int,
        body: dict[str, str],
    ) -> None:
        response_body = json.dumps(body).encode("utf-8")

        self.send_response(status_code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(response_body)))
        self.end_headers()

        self.wfile.write(response_body)

    def log_message(self, format: str, *args: object) -> None:
        print(f"[HTTP] {format % args}")


def main() -> None:
    server = ThreadingHTTPServer(
        (HOST, PORT),
        SpotifyMockHandler,
    )

    print("Spotify mock server is running.")
    print(f"Token endpoint: http://{HOST}:{PORT}/api/token")
    print("Every POST request to /api/token returns HTTP 400 invalid_grant.")
    print("Press Ctrl+C to stop the server.")

    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping Spotify mock server.")
    finally:
        server.server_close()


if __name__ == "__main__":
    main()