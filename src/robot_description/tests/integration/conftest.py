#!/usr/bin/env python3
"""
@brief Shared pytest fixtures for robot_description integration tests.

Provides session-scoped fixtures for resolving package paths, expanding
the robot xacro file, and setting up a minimal ament index for testing.
"""

import os
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

import pytest


@pytest.fixture(scope='session')
def pkg_dir() -> Path:
    """@brief Return the root directory of the robot_description package."""
    return Path(__file__).parent.parent.parent


@pytest.fixture(scope='session')
def urdf_file(pkg_dir: Path) -> Path:
    """@brief Return the path to the robot xacro entry point."""
    return pkg_dir / 'urdf' / 'robot.urdf.xacro'


@pytest.fixture(scope='session')
def ament_prefix(pkg_dir: Path, tmp_path_factory: pytest.TempPathFactory) -> Path:
    """@brief Create a minimal ament index so xacro can resolve $(find robot_description)."""
    prefix = tmp_path_factory.mktemp('ament_prefix')
    marker = prefix / 'share' / 'ament_index' / 'resource_index' / 'packages' / 'robot_description'
    marker.parent.mkdir(parents=True)
    marker.touch()
    share_pkg = prefix / 'share' / 'robot_description'
    share_pkg.symlink_to(pkg_dir)
    return prefix


@pytest.fixture(scope='session')
def urdf_root(urdf_file: Path, ament_prefix: Path) -> ET.Element:
    """
    @brief Expand the xacro file and return the parsed URDF root element.

    @param urdf_file Path to the robot xacro file.
    @param ament_prefix Temporary ament prefix with robot_description registered.
    @return Parsed root XML element of the expanded URDF.
    """
    env = os.environ.copy()
    existing = env.get('AMENT_PREFIX_PATH', '')
    env['AMENT_PREFIX_PATH'] = str(ament_prefix) + (':' + existing if existing else '')
    result = subprocess.run(
        ['xacro', str(urdf_file)],
        capture_output=True,
        text=True,
        env=env,
    )
    assert result.returncode == 0, f'xacro expansion failed: {result.stderr}'
    return ET.fromstring(result.stdout)
