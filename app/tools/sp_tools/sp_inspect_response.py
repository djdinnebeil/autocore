import base64
import json
import sys
from pathlib import Path

import requests


TOKEN_ENDPOINT = "https://accounts.spotify.com/api/token"

CODES_PATH = Path("sp_codes.rc")
TOKENS_PATH = Path("sp_tokens.rc")


def read_lines(path: Path, required_lines: int) -> list[str]:
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except FileNotFoundError:
        raise RuntimeError(f"File not found: {path}")
    except OSError as error:
        raise RuntimeError(f"Unable to read {path}: {error}") from error

    if len(lines) < required_lines:
        raise RuntimeError(
            f"{path} must contain at least {required_lines} lines."
        )

    return lines


def main() -> int:
    print("Spotify access-token refresh test")
    print("---------------------------------\n")

    try:
        code_lines = read_lines(CODES_PATH, 2)
        token_lines = read_lines(TOKENS_PATH, 2)

        client_id = code_lines[0].strip()
        client_secret = code_lines[1].strip()
        old_refresh_token = token_lines[1].strip()

        if not client_id:
            raise RuntimeError(
                f"Client ID is missing from line 1 of {CODES_PATH}."
            )

        if not client_secret:
            raise RuntimeError(
                f"Client secret is missing from line 2 of {CODES_PATH}."
            )

        if not old_refresh_token:
            raise RuntimeError(
                f"Refresh token is missing from line 2 of {TOKENS_PATH}."
            )

    except RuntimeError as error:
        print(f"Configuration error: {error}", file=sys.stderr)
        return 1

    credentials = f"{client_id}:{client_secret}"

    encoded_credentials = base64.b64encode(
        credentials.encode("utf-8")
    ).decode("ascii")

    headers = {
        "Authorization": f"Basic {encoded_credentials}",
        "Content-Type": "application/x-www-form-urlencoded",
    }

    form_data = {
        "grant_type": "refresh_token",
        "refresh_token": old_refresh_token,
    }

    try:
        response = requests.post(
            TOKEN_ENDPOINT,
            headers=headers,
            data=form_data,
            timeout=30,
        )
    except requests.RequestException as error:
        print(f"Request failed: {error}", file=sys.stderr)
        return 1

    print(f"HTTP status: {response.status_code}")
    print(
        "Content-Type: "
        f"{response.headers.get('Content-Type', 'unknown')}"
    )
    print("\nSpotify response:")

    try:
        response_json = response.json()
    except requests.exceptions.JSONDecodeError:
        print(response.text)
        return 1

    print(json.dumps(response_json, indent=4))

    if response.status_code != 200:
        return 1

    print("\nResponse analysis:")

    print(
        "Access token returned: "
        f"{response_json.get('access_token') is not None}"
    )

    new_refresh_token = response_json.get("refresh_token")

    if new_refresh_token is None:
        print("Replacement refresh token returned: False")
        print("Continue using the existing refresh token.")
    else:
        print("Replacement refresh token returned: True")
        print(
            "Refresh token changed: "
            f"{new_refresh_token != old_refresh_token}"
        )

    print(
        "Access-token lifetime: "
        f"{response_json.get('expires_in', 'not returned')} seconds"
    )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())