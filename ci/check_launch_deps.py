#!/usr/bin/env python3
"""Validates that Python imports in launch files have corresponding exec_depend
entries in the package's package.xml.
"""

import argparse
import ast
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

STDLIB = frozenset(sys.stdlib_module_names)


def _top_level_imports(filepath: Path) -> set[str]:
    tree = ast.parse(filepath.read_text(), filename=str(filepath))
    modules: set[str] = set()
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            for alias in node.names:
                modules.add(alias.name.split('.')[0])
        elif isinstance(node, ast.ImportFrom) and node.module:
            modules.add(node.module.split('.')[0])
    return modules


def _exec_depends(package_xml: Path) -> set[str]:
    root = ET.parse(package_xml).getroot()
    return {dep.text.strip() for dep in root.findall('exec_depend') if dep.text}


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Check that launch file imports have matching exec_depend in package.xml'
    )
    parser.add_argument('directory', help='ROS package directory to scan')
    parser.add_argument(
        '--exclude',
        nargs='*',
        default=[],
        metavar='FILE',
        help='Launch filenames to skip (e.g. debug.launch.py)',
    )
    args = parser.parse_args()

    pkg_dir = Path(args.directory).resolve()
    package_xml = pkg_dir / 'package.xml'
    launch_dir = pkg_dir / 'launch'

    if not package_xml.exists():
        print(f'ERROR: {package_xml} not found', file=sys.stderr)
        return 1

    if not launch_dir.exists():
        return 0

    exec_deps = _exec_depends(package_xml)
    excluded = set(args.exclude)
    launch_files = sorted(f for f in launch_dir.glob('*.launch.py') if f.name not in excluded)

    failures: dict[Path, set[str]] = {}
    for launch_file in launch_files:
        missing = {
            m for m in _top_level_imports(launch_file) if m not in STDLIB and m not in exec_deps
        }
        if missing:
            failures[launch_file] = missing

    if not failures:
        return 0

    print('Missing exec_depend entries in package.xml:\n', file=sys.stderr)
    for launch_file, missing in sorted(failures.items()):
        for module in sorted(missing):
            print(f"  {launch_file.relative_to(pkg_dir)}  →  import '{module}'", file=sys.stderr)
    print(f'\nAdd the missing entries to: {package_xml}', file=sys.stderr)
    return 1


if __name__ == '__main__':
    sys.exit(main())
