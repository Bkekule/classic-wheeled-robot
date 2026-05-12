#!/usr/bin/env python3
"""Validates that statically-detectable dependencies in launch/test files have
corresponding exec_depend or test_depend entries in the package's package.xml.

Launch mode detects:
  - Python imports
  - Node(package='...') references

Test mode detects:
  - Python imports
"""

import argparse
import ast
import sys
from typing import cast
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


def _node_packages(filepath: Path) -> set[str]:
    tree = ast.parse(filepath.read_text(), filename=str(filepath))
    packages: set[str] = set()
    for node in ast.walk(tree):
        if not isinstance(node, ast.Call):
            continue
        func = node.func
        func_name = (
            func.id
            if isinstance(func, ast.Name)
            else func.attr
            if isinstance(func, ast.Attribute)
            else None
        )
        if func_name != 'Node':
            continue
        for kw in node.keywords:
            if kw.arg == 'package' and isinstance(kw.value, ast.Constant):
                packages.add(cast(str, kw.value.value))
    return packages


def _depends(package_xml: Path, tag: str) -> set[str]:
    root = ET.parse(package_xml).getroot()
    return {dep.text.strip() for dep in root.findall(tag) if dep.text}


def main() -> int:
    parser = argparse.ArgumentParser(
        description='Check that launch/test file dependencies are declared in package.xml'
    )
    parser.add_argument('directory', help='ROS package directory to scan')
    parser.add_argument(
        '--mode',
        choices=['launch', 'test'],
        default='launch',
        help='launch: checks launch/*.launch.py vs exec_depend; test: checks tests/**/*.py vs test_depend',
    )
    parser.add_argument(
        '--exclude',
        nargs='*',
        default=[],
        metavar='FILE',
        help='Filenames to skip (basename only)',
    )
    args = parser.parse_args()

    pkg_dir = Path(args.directory).resolve()
    package_xml = pkg_dir / 'package.xml'

    if not package_xml.exists():
        print(f'ERROR: {package_xml} not found', file=sys.stderr)
        return 1

    excluded = set(args.exclude)

    if args.mode == 'launch':
        scan_dir = pkg_dir / 'launch'
        if not scan_dir.exists():
            return 0
        files = sorted(f for f in scan_dir.glob('*.launch.py') if f.name not in excluded)
        declared = _depends(package_xml, 'exec_depend')
        dep_tag = 'exec_depend'
    else:
        scan_dir = pkg_dir / 'tests'
        if not scan_dir.exists():
            scan_dir = pkg_dir / 'test'
        if not scan_dir.exists():
            return 0
        files = sorted(f for f in scan_dir.rglob('*.py') if f.name not in excluded)
        declared = _depends(package_xml, 'test_depend')
        dep_tag = 'test_depend'

    failures: dict[Path, set[str]] = {}
    for filepath in files:
        candidates = _top_level_imports(filepath)
        if args.mode == 'launch':
            candidates |= _node_packages(filepath)
        missing = {m for m in candidates if m not in STDLIB and m not in declared}
        if missing:
            failures[filepath] = missing

    if not failures:
        return 0

    print(f'Missing <{dep_tag}> entries in package.xml:\n', file=sys.stderr)
    for filepath, missing in sorted(failures.items()):
        for module in sorted(missing):
            print(f"  {filepath.relative_to(pkg_dir)}  →  '{module}'", file=sys.stderr)
    print(f'\nAdd the missing entries to: {package_xml}', file=sys.stderr)
    return 1


if __name__ == '__main__':
    sys.exit(main())
