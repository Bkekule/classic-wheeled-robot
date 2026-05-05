#!/usr/bin/env python3

import os
from pathlib import Path
import subprocess
import xml.etree.ElementTree as ET

import pytest


@pytest.fixture(scope='session')
def bringup_dir() -> Path:
    return Path(__file__).parent.parent.parent


@pytest.fixture(scope='session')
def description_dir(bringup_dir: Path) -> Path:
    return bringup_dir.parent / 'robot_description'


@pytest.fixture(scope='session')
def urdf_file(description_dir: Path) -> Path:
    return description_dir / 'urdf' / 'my_robot.urdf.xacro'


@pytest.fixture(scope='session')
def ament_prefix(description_dir: Path, tmp_path_factory: pytest.TempPathFactory) -> Path:
    """Create a minimal ament index so xacro can resolve $(find robot_description)."""
    prefix = tmp_path_factory.mktemp('ament_prefix')
    marker = (
        prefix / 'share' / 'ament_index' / 'resource_index' / 'packages' / 'robot_description'
    )
    marker.parent.mkdir(parents=True)
    marker.touch()
    (prefix / 'share' / 'robot_description').symlink_to(description_dir)
    return prefix


@pytest.fixture(scope='session')
def urdf_root(urdf_file: Path, ament_prefix: Path) -> ET.Element:
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
