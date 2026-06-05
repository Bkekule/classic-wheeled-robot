#!/usr/bin/env python3
"""
Patch ros2_control's controller_manager.cpp so that
determine_controller_node_options() skips --params-file forwarding to controllers.

The controller_manager forwards its own node options to each controller it loads.
This includes --params-file arguments that point to the full controller YAML,
causing "parameter not declared" errors inside individual controllers.

This patch filters out RCL_PARAM_FILE_FLAG ("--params-file") and its following
filename argument before iterating over them in determine_controller_node_options.
"""

import sys
from pathlib import Path

SEARCH = """\
  for (const std::string & arg : cm_node_options_.arguments())
  {"""

REPLACE = """\
  // [PATCH] Filter --params-file flags from CM args before forwarding to controllers.
  // This prevents the controller_manager from passing its own parameter files
  // to individual controller nodes where they cause "parameter not declared" errors.
  // Also handles the edge case of a trailing --params-file with no file path.
  // RCL_PARAM_FILE_FLAG = "--params-file"
  std::vector<std::string> filtered_cm_args;
  {
    bool skip_next = false;
    for (const std::string & a : cm_node_options_.arguments())
    {
      if (skip_next) { skip_next = false; continue; }
      if (a == RCL_PARAM_FILE_FLAG) { skip_next = true; continue; }
      filtered_cm_args.push_back(a);
    }
  }

  for (const std::string & arg : filtered_cm_args)
  {"""


def main() -> int:
    if len(sys.argv) != 2:
        print(f'Usage: {sys.argv[0]} <controller_manager.cpp>', file=sys.stderr)
        return 1

    path = Path(sys.argv[1])
    content = path.read_text()

    if SEARCH not in content:
        print('ERROR: Could not find target code block in controller_manager.cpp', file=sys.stderr)
        print('The ros2_control source may have changed. Patch needs updating.', file=sys.stderr)
        return 1

    count = content.count(SEARCH)
    if count != 1:
        print(f'ERROR: Found {count} occurrences of target block (expected 1)', file=sys.stderr)
        return 1

    patched = content.replace(SEARCH, REPLACE, 1)
    path.write_text(patched)
    print(f'Successfully patched {path}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
