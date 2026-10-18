"""Read local Wi-Fi settings before PlatformIO compiles the ESP32 firmware.

The ESP32 cannot read the laptop's .env at runtime. This build step puts the
settings into an ignored generated header, which is compiled into the firmware.
Values are never printed or placed in compiler command-line arguments.
"""
import json
from pathlib import Path

Import("env")

project_dir = Path(env.subst("$PROJECT_DIR"))
settings = {"WIFI_SSID": "", "WIFI_PASSWORD": ""}
dotenv = project_dir / ".env"
if dotenv.exists():
    for line_number, raw_line in enumerate(dotenv.read_text(encoding="utf-8-sig").splitlines(), 1):
        line = raw_line.strip()
        if not line or line.startswith("#"):
            continue
        key, separator, value = line.partition("=")
        key = key.strip()
        if key not in settings:
            continue
        if not separator:
            raise ValueError(f".env line {line_number}: expected KEY=value")
        value = value.strip()
        # Matching quotes are removed. Everything inside is literal, including
        # spaces, #, $, and backslashes; there is no variable expansion.
        if value.startswith(('"', "'")):
            if len(value) < 2 or value[-1] != value[0]:
                raise ValueError(f".env line {line_number}: unmatched quotes")
            value = value[1:-1]
        if "\x00" in value:
            raise ValueError(f".env line {line_number}: null characters are not supported")
        settings[key] = value

generated_dir = project_dir / ".pio" / "wifi_env"
generated_dir.mkdir(parents=True, exist_ok=True)
header = generated_dir / "wifi_env.h"
contents = (
    "// Generated from local .env. Do not commit this file.\n#pragma once\n"
    + "static constexpr char WIFI_ENV_SSID[] = "
    + json.dumps(settings["WIFI_SSID"], ensure_ascii=False) + ";\n"
    + "static constexpr char WIFI_ENV_PASSWORD[] = "
    + json.dumps(settings["WIFI_PASSWORD"], ensure_ascii=False) + ";\n"
)
if not header.exists() or header.read_text(encoding="utf-8") != contents:
    header.write_text(contents, encoding="utf-8")
env.Append(CPPPATH=[str(generated_dir)])
