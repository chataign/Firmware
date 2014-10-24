/**
 * Path follow mode implementation
 */
#include <nuttx/config.h>

#include <geo/geo.h>
#include <math.h>
#include <mavlink/mavlink_log.h>
#include <uORB/topics/external_trajectory.h>

#include "navigator.h"
#include "path_follow.hpp"

PathFollow::PathFollow(Navigator *navigator, const char *name):
		NavigatorMode(navigator, name),
		_last_trajectory_time(0),
		_saved_trajectory(),
		_trajectory_distance(0),
		_has_valid_setpoint(false)
{

}
PathFollow::~PathFollow()
{

}
bool PathFollow::init()
{
	// TODO! Parameters
	return (_saved_trajectory.init(500));
}
void PathFollow::on_inactive()
{
	// TODO! Consider if we want to continue collecting trajectory data while inactive
	// update_trajectory();
}
void PathFollow::on_activation()
{
	_has_valid_setpoint = false;
	update_trajectory();
	global_pos = _navigator->get_global_position();
	pos_sp_triplet = _navigator->get_position_setpoint_triplet();
	pos_sp_triplet->next.valid = false;
	// Reset position setpoint to shoot and loiter until we get an acceptable trajectory point
	pos_sp_triplet->current.lat = global_pos->lat;
	pos_sp_triplet->current.lon = global_pos->lon;
	pos_sp_triplet->current.alt = global_pos->alt;
	point_camera_to_target(&(pos_sp_triplet->current));
	_navigator->set_position_setpoint_triplet_updated();
}
void PathFollow::on_active()
{
	update_trajectory();
	if (!_has_valid_setpoint) {
		if (!_saved_trajectory.is_empty()) {
			_saved_trajectory.pop(_actual_point);
		}
	}
}
void PathFollow::execute_vehicle_command()
{

}

void PathFollow::update_trajectory()
{
	struct external_trajectory_s *target_trajectory = _navigator->get_target_trajectory();
	// Assuming timestamp won't be 0 on first call
	if (_last_trajectory_time != target_trajectory->timestamp && target_trajectory->point_type != 0) {
		if (_saved_trajectory.is_empty()) {
			_trajectory_distance = 0;
		}
		else {
			_trajectory_distance += get_distance_to_next_waypoint(_last_point.lat, _last_point.lon,
					target_trajectory->lat, target_trajectory->lon);
		}
		_last_trajectory_time = target_trajectory->timestamp;
		_last_point.lat = target_trajectory->lat;
		_last_point.lon = target_trajectory->lon;
		_last_point.alt = target_trajectory->alt;
		if (!_saved_trajectory.add(_last_point, true)) {
			mavlink_log_info(_mavlink_fd, "Trajectory overflow!");
		}
	}
}

float PathFollow::calculate_desired_speed(float distance, float target_speed) {
	// TODO! Parameters both for min, max, ok and for functions
	float max_dist = 20.0f, min_dist = 5.0f, ok_dist = 10.0f;
	float max_speed = 11.1111f;
	float res;
	if (distance <= min_dist) {
		return 0;
	}
	else if (distance >= max_dist) {
		// TODO! Max speed
		return max_speed;
	}
	else if (distance >= ok_dist) {
		// TODO! Simplify and add precalculated values
		res = target_speed * ((distance - ok_dist)*(distance - ok_dist)*4.0f/(max_dist-ok_dist)/(max_dist-ok_dist)+1.0f);
	}
	else {
		// TODO! Simplify and add precalculated values
		res = target_speed * ((float(-atan(double(-(distance-2.5f-min_dist)*20.0f/(ok_dist-min_dist))))/M_PI_F)+0.5f);
	}
	if (res < 0.0f) return 0.0f;
	if (res > max_speed) return max_speed;
	return res;
}

float PathFollow::calculate_current_distance() {

}
