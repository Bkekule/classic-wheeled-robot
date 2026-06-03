#!/usr/bin/env python3
"""
Patch gz_ros2_control_plugin.cpp to remove all argument injection into the
controller_manager's node options.

This prevents gz_ros2_control from passing --params-file, --ros-args, -p,
or use_sim_time arguments into the CM. Controller configuration is loaded
via the spawner's --param-file flag instead.
"""

import re
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <gz_ros2_control_plugin.cpp>", file=sys.stderr)
        return 1

    path = Path(sys.argv[1])
    content = path.read_text()

    # 1. Replace the initial arguments vector to be empty (no --ros-args seed)
    old_init = 'std::vector<std::string> arguments = {"--ros-args"};'
    new_init = "std::vector<std::string> arguments = {};"
    if old_init not in content:
        print(f"ERROR: Could not find arguments init: {old_init}", file=sys.stderr)
        return 1
    content = content.replace(old_init, new_init, 1)
    print("  [1/3] Cleared --ros-args from arguments init")

    # 2. Remove the while loop that pushes RCL_PARAM_FILE_FLAG for each <parameters> element
    # Pattern: from the GetElement("parameters") line through the while loop's closing brace
    pattern = (
        r'[ \t]*sdf::ElementPtr argument_sdf_param = sdfPtr->GetElement\("parameters"\);\s*'
        r'while \(argument_sdf_param\) \{[^}]*\}'
    )
    match = re.search(pattern, content, re.DOTALL)
    if not match:
        # Try alternate: maybe already partially patched or slightly different formatting
        print("ERROR: Could not find the params-file while loop", file=sys.stderr)
        print("Searched for pattern matching: sdf::ElementPtr argument_sdf_param = ...; while (...) { ... }", file=sys.stderr)
        return 1
    content = content[:match.start()] + content[match.end():]
    print("  [2/3] Removed --params-file while loop")

    # 3. Remove the use_sim_time force injection lines:
    #    arguments.push_back("-p");
    #    arguments.push_back("use_sim_time:=true");
    # Also remove the comment if present
    content = re.sub(
        r'[ \t]*// Force setting of use_sim_time parameter\n', '', content
    )
    content = re.sub(
        r'[ \t]*arguments\.push_back\("-p"\);\n', '', content
    )
    content = re.sub(
        r'[ \t]*arguments\.push_back\("use_sim_time:=true"\);\n', '', content
    )
    print("  [3/3] Removed use_sim_time injection")

    path.write_text(content)
    print(f"Successfully patched {path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
