#!/usr/bin/env python3
"""
@brief Shared pytest fixtures for robot_navigation integration tests.

Provides session-scoped fixtures for loading and parsing the nav2_params.yaml
and diff_drive_controller.yaml configuration files.
"""

from pathlib import Path

import pytest
import yaml


@pytest.fixture(scope='session')
def nav_pkg_dir() -> Path:
    """@brief Return the root directory of the robot_navigation package."""
    return Path(__file__).parent.parent.parent


@pytest.fixture(scope='session')
def description_pkg_dir() -> Path:
    """@brief Return the root directory of the robot_description package."""
    return Path(__file__).parent.parent.parent.parent / 'robot_description'


@pytest.fixture(scope='session')
def nav2_params(nav_pkg_dir: Path) -> dict:
    """@brief Load and return the parsed nav2_params.yaml configuration."""
    params_file = nav_pkg_dir / 'config' / 'nav2_params.yaml'
    assert params_file.exists(), f'nav2_params.yaml not found at {params_file}'
    with open(params_file) as f:
        return yaml.safe_load(f)


@pytest.fixture(scope='session')
def diff_drive_params(description_pkg_dir: Path) -> dict:
    """@brief Load and return the parsed diff_drive_controller.yaml configuration."""
    params_file = description_pkg_dir / 'config' / 'diff_drive_controller.yaml'
    assert params_file.exists(), f'diff_drive_controller.yaml not found at {params_file}'
    with open(params_file) as f:
        return yaml.safe_load(f)
