#!/usr/bin/env python3

import xml.etree.ElementTree as ET


def test_xacro_expansion(urdf_root: ET.Element) -> None:
    """Test that xacro expansion produces a non-empty robot element."""
    assert urdf_root.tag == 'robot'


def test_urdf_has_required_links(urdf_root: ET.Element) -> None:
    """Test that the expanded URDF contains all required links."""
    link_names = {link.get('name') for link in urdf_root.findall('link')}

    required_links = {
        'robot_footprint',
        'chassis',
        'left_wheel',
        'right_wheel',
        'front_caster',
        'back_caster',
        'lidar',
        'camera',
        'camera_optical_frame',
    }

    missing = required_links - link_names
    assert not missing, f'Missing links: {missing}'


def test_urdf_has_required_joints(urdf_root: ET.Element) -> None:
    """Test that the expanded URDF contains all required joints."""
    joint_names = {joint.get('name') for joint in urdf_root.findall('joint')}

    required_joints = {
        'robot_footprint_to_chassis_joint',
        'chassis_to_left_wheel_joint',
        'chassis_to_right_wheel_joint',
        'chassis_to_front_caster_joint',
        'chassis_to_back_caster_joint',
        'chassis_to_lidar_joint',
        'chassis_to_camera_joint',
        'camera_to_optical_joint',
    }

    missing = required_joints - joint_names
    assert not missing, f'Missing joints: {missing}'


def test_wheel_joints_are_continuous(urdf_root: ET.Element) -> None:
    """Test that driven wheel joints are continuous type."""
    for name in ('chassis_to_left_wheel_joint', 'chassis_to_right_wheel_joint'):
        joint = urdf_root.find(f".//joint[@name='{name}'][@type]")
        assert joint is not None, f'Joint not found: {name}'
        assert joint.get('type') == 'continuous', f'{name} is not continuous'


def test_fixed_joints_are_fixed(urdf_root: ET.Element) -> None:
    """Test that all non-driven joints are fixed type."""
    fixed_joints = {
        'robot_footprint_to_chassis_joint',
        'chassis_to_front_caster_joint',
        'chassis_to_back_caster_joint',
        'chassis_to_lidar_joint',
        'chassis_to_camera_joint',
        'camera_to_optical_joint',
    }
    for name in fixed_joints:
        joint = urdf_root.find(f".//joint[@name='{name}']")
        assert joint is not None, f'Joint not found: {name}'
        assert joint.get('type') == 'fixed', f'{name} is not fixed'


def test_ros2_control_block_present(urdf_root: ET.Element) -> None:
    """Test that the ros2_control block exists and references the correct joints."""
    rc = urdf_root.find('.//ros2_control')
    assert rc is not None, 'ros2_control block not found'

    declared_joints = {j.get('name') for j in rc.findall('joint')}
    assert 'chassis_to_left_wheel_joint' in declared_joints
    assert 'chassis_to_right_wheel_joint' in declared_joints


def test_ros2_control_joints_have_velocity_interface(urdf_root: ET.Element) -> None:
    """Test that ros2_control wheel joints expose a velocity command interface."""
    rc = urdf_root.find('.//ros2_control')
    assert rc is not None

    for joint_name in ('chassis_to_left_wheel_joint', 'chassis_to_right_wheel_joint'):
        joint = rc.find(f"joint[@name='{joint_name}']")
        assert joint is not None, f'ros2_control joint not found: {joint_name}'
        interfaces = {ci.get('name') for ci in joint.findall('command_interface')}
        assert 'velocity' in interfaces, f'{joint_name} missing velocity command interface'
