#!/usr/bin/env python3
"""
@brief Integration tests that validate individual xacro file inclusion and macro expansion.

Verifies that base, wheel, and sensor xacros are correctly included and
that the resulting joints have the expected types.
"""

import xml.etree.ElementTree as ET


def test_base_xacro_expands(urdf_root: ET.Element) -> None:
    """@brief Test that base.xacro is included and expands."""
    chassis = urdf_root.find(".//link[@name='chassis']")
    assert chassis is not None, 'Chassis link from base.xacro not found'


def test_wheel_macro_instantiation(urdf_root: ET.Element) -> None:
    """@brief Test that the wheel macro is instantiated for left and right."""
    for name in ('left_wheel', 'right_wheel'):
        link = urdf_root.find(f".//link[@name='{name}']")
        assert link is not None, f'{name} link not found'


def test_sensor_xacros_expand(urdf_root: ET.Element) -> None:
    """@brief Test that sensor xacro files are included."""
    for name in ('hokuyo_lidar', 'camera'):
        link = urdf_root.find(f".//link[@name='{name}']")
        assert link is not None, f'{name} link not found'


def test_wheel_joints_are_continuous(urdf_root: ET.Element) -> None:
    """@brief Test that wheel joints are continuous type."""
    for name in ('chassis_to_left_wheel_joint', 'chassis_to_right_wheel_joint'):
        joint = urdf_root.find(f".//joint[@name='{name}'][@type]")
        assert joint is not None, f'{name} not found'
        assert joint.get('type') == 'continuous', f'{name} is not continuous'


def test_caster_joints_are_fixed(urdf_root: ET.Element) -> None:
    """@brief Test that caster joints are fixed type."""
    for name in ('chassis_to_front_caster_joint', 'chassis_to_back_caster_joint'):
        joint = urdf_root.find(f".//joint[@name='{name}']")
        assert joint is not None, f'{name} not found'
        assert joint.get('type') == 'fixed', f'{name} is not fixed'
