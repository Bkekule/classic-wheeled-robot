#!/usr/bin/env python3
"""
@brief Integration tests that verify parameter consistency between configs.

Ensures that velocity and acceleration limits in diff_drive_controller.yaml
are consistent with the velocity_smoother section of nav2_params.yaml.

These tests catch drift between the two separate config files that wouldn't
otherwise be validated together at build time.
"""


class TestVelocityLimitsConsistency:
    """@brief Verify max velocity params are consistent across config files."""

    def test_max_linear_velocity_smoother(
        self, diff_drive_params: dict, nav2_params: dict
    ) -> None:
        """@brief velocity_smoother max linear velocity must match diff_drive max linear velocity."""
        diff_drive_max_vel_x = diff_drive_params['diff_drive_controller']['ros__parameters'][
            'linear'
        ]['x']['max_velocity']
        smoother_max_vel = nav2_params['velocity_smoother']['ros__parameters']['max_velocity']

        assert smoother_max_vel[0] == diff_drive_max_vel_x, (
            f'velocity_smoother max_velocity[0] ({smoother_max_vel[0]}) != '
            f'diff_drive max linear velocity ({diff_drive_max_vel_x})'
        )

    def test_max_angular_velocity_smoother(
        self, diff_drive_params: dict, nav2_params: dict
    ) -> None:
        """@brief velocity_smoother max angular velocity must match diff_drive max angular velocity."""
        diff_drive_max_vel_theta = diff_drive_params['diff_drive_controller']['ros__parameters'][
            'angular'
        ]['z']['max_velocity']
        smoother_max_vel = nav2_params['velocity_smoother']['ros__parameters']['max_velocity']

        assert smoother_max_vel[2] == diff_drive_max_vel_theta, (
            f'velocity_smoother max_velocity[2] ({smoother_max_vel[2]}) != '
            f'diff_drive max angular velocity ({diff_drive_max_vel_theta})'
        )


class TestAccelerationLimitsConsistency:
    """@brief Verify acceleration params are consistent across config files."""

    def test_max_angular_accel_follow_path_matches_smoother(
        self, nav2_params: dict
    ) -> None:
        """@brief FollowPath max_angular_accel must match velocity_smoother angular accel limit."""
        follow_path_max_angular_accel = nav2_params['controller_server']['ros__parameters'][
            'FollowPath'
        ]['max_angular_accel']
        smoother_max_accel = nav2_params['velocity_smoother']['ros__parameters']['max_accel']

        assert follow_path_max_angular_accel == smoother_max_accel[2], (
            f'FollowPath max_angular_accel ({follow_path_max_angular_accel}) != '
            f'velocity_smoother max_accel[2] ({smoother_max_accel[2]})'
        )

    def test_max_decel_angular_smoother_matches_accel(self, nav2_params: dict) -> None:
        """@brief velocity_smoother angular decel magnitude must match its accel magnitude."""
        smoother_params = nav2_params['velocity_smoother']['ros__parameters']
        max_accel_theta = smoother_params['max_accel'][2]
        max_decel_theta = smoother_params['max_decel'][2]

        assert abs(max_decel_theta) == abs(max_accel_theta), (
            f'velocity_smoother angular decel magnitude ({abs(max_decel_theta)}) != '
            f'accel magnitude ({abs(max_accel_theta)})'
        )
