#!/usr/bin/env python3
"""
@brief Validates that hardcoded geometry values in controllers.yaml match the URDF.

wheel_radius and wheel_separation cannot use xacro expressions in YAML,
so they must be kept in sync manually. These tests catch drift.
"""

from pathlib import Path
from typing import cast
import xml.etree.ElementTree as ET

import pytest
import yaml


@pytest.fixture(scope='module')
def controllers(description_dir: Path) -> dict:  # type: ignore
    """
    @brief Load and return the diff_drive_controller configuration from YAML.

    @param description_dir Path to the robot_description package root.
    @return Parsed YAML configuration as a dictionary.
    """
    with open(description_dir / 'config' / 'diff_drive_controller.yaml') as f:
        return cast(dict, yaml.safe_load(f))  # type: ignore


def _wheel_radius_from_urdf(urdf_root: ET.Element) -> float:
    """
    @brief Extract the wheel radius from the left_wheel link geometry in the URDF.

    @param urdf_root Parsed root XML element of the expanded URDF.
    @return Wheel radius in metres.
    """
    link = urdf_root.find(".//link[@name='left_wheel']")
    assert link is not None, 'left_wheel link not found in URDF'
    cylinder = link.find('.//collision/geometry/cylinder')
    assert cylinder is not None, 'cylinder geometry not found in left_wheel'
    return float(cast(str, cylinder.get('radius')))


def _wheel_separation_from_urdf(urdf_root: ET.Element) -> float:
    """
    Extract wheel separation from the left wheel joint's y-offset.

    Each wheel is placed at ±(chassis_width/2 + wheel_length/2) from centre,
    so separation = 2 * |y_offset|.
    """
    joint = urdf_root.find(".//joint[@name='chassis_to_left_wheel_joint'][@type='continuous']")
    assert joint is not None, 'chassis_to_left_wheel_joint not found in URDF'
    origin = joint.find('.//origin')
    assert origin is not None
    xyz = origin.get('xyz', '0 0 0').split()
    return abs(float(xyz[1])) * 2


def test_wheel_radius_matches_urdf(
    urdf_root: ET.Element,
    controllers: dict,  # type: ignore
) -> None:
    """Wheel radius in controllers.yaml must match the URDF geometry."""
    ddc_params = controllers['diff_drive_controller']['ros__parameters']  # type: ignore
    yaml_radius = ddc_params['wheel_radius']  # type: ignore
    urdf_radius = _wheel_radius_from_urdf(urdf_root)
    assert yaml_radius == pytest.approx(urdf_radius), (  # type: ignore
        f'controllers.yaml wheel_radius ({yaml_radius}) != URDF wheel radius ({urdf_radius})'
    )


def test_wheel_separation_matches_urdf(
    urdf_root: ET.Element,
    controllers: dict,  # type: ignore
) -> None:
    """Wheel separation in controllers.yaml must match URDF joint placement."""
    ddc_params = controllers['diff_drive_controller']['ros__parameters']  # type: ignore
    yaml_separation = ddc_params['wheel_separation']  # type: ignore
    urdf_separation = _wheel_separation_from_urdf(urdf_root)
    assert yaml_separation == pytest.approx(urdf_separation), (  # type: ignore
        f'controllers.yaml wheel_separation ({yaml_separation}) != '
        f'URDF wheel separation ({urdf_separation})'
    )
