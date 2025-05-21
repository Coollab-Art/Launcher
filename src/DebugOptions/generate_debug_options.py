# ---HOW TO---

# ------------

import os
from pathlib import Path
from importlib.machinery import SourceFileLoader

generate_files = SourceFileLoader(
    "generate_files",
    os.path.join(
        Path(os.path.abspath(__file__)).parent.parent.parent,
        "Lab/Cool/src/Cool/DebugOptions",
        "debug_options_generator.py",
    ),
).load_module()


def all_debug_options():
    from generate_files import DebugOption, Kind

    return [
        DebugOption(
            name_in_code="log_when_uninstalling_versions_automatically",
            name_in_ui="Log when uninstalling versions automatically",
            available_in_release=True,
        ),
    ]


if __name__ == "__main__":
    from generate_files import generate_debug_options

    generate_debug_options(
        output_folder="generated",
        namespace="Launcher",
        cache_file_name="debug_options_launcher",
        debug_options=all_debug_options(),
    )
