//---------------------------------------------------------
// file:	DeadReckoningControl.cpp
// author:	Matthew Picioccio
// email:	matthew.picioccio@digipen.edu
//
// brief:	Calculates the position as *interpolated* from the last two known positions
//
// Copyright © 2021 DigiPen, All rights reserved.
//---------------------------------------------------------
#include "pch.h"
#include "SnapshotControl.h"


void SnapshotControl::AddSnapshot(const State& new_state, const u_long remote_frame)
{
	sync_ratio_.base_frame = sync_ratio_.target_frame;
	sync_ratio_.target_frame = remote_frame;
	sync_ratio_.t = 0.0f;

	previous_state_ = latest_state_;
	latest_state_ = new_state;

	if (is_initialized_ == false)
	{
		current_x_ = previous_state_.x = latest_state_.x;
		current_y_ = previous_state_.y = latest_state_.y;
		is_initialized_ = true;
	}

	if (new_state.time_since_last_update_secs <= 0.0f)
	{
		current_x_ = previous_state_.x;
		current_y_ = previous_state_.y;
	}

	// added snapshot buffering
	snapshots_.push_back(new_state);

	if (snapshots_.size() > 3)
	{
		snapshots_.pop_front();
	}

	interpolation_timer_ = 0.0f;
}


float SnapshotControl::CalculateX(const float t) const
{
	const auto clamped_t = CP_Math_ClampFloat(t, 0.0f, 1.0f);
	return previous_state_.x + clamped_t * (latest_state_.x - previous_state_.x);
}


float SnapshotControl::CalculateY(const float t) const
{
	const auto clamped_t = CP_Math_ClampFloat(t, 0.0f, 1.0f);
	return previous_state_.y + clamped_t * (latest_state_.y - previous_state_.y);
}


void SnapshotControl::Update(const float dt)
{
	if (is_initialized_ == false)
	{
		return;
	}

	if (latest_state_.time_since_last_update_secs <= 0.0f)
	{
		return;
	}

	// simple idea: we should move from 0 - 1 in bias in the same amount of time it took for the object to arrive
	//sync_ratio_.t += dt / latest_state_.time_since_last_update_secs;

	//current_x_ = CalculateX(sync_ratio_.t);
	//current_y_ = CalculateY(sync_ratio_.t);

	// if we have at least 2 buffered snapshots, use them
	if (snapshots_.size() >= 2)
	{
		const auto& older = snapshots_[0];
		const auto& newer = snapshots_[1];

		interpolation_timer_ += dt;

		float duration = newer.time_since_last_update_secs;
		if (duration <= 0.0f)
			duration = 0.0001f; // avoid divide by zero

		float t = interpolation_timer_ / duration;
		t = CP_Math_ClampFloat(t, 0.0f, 1.0f);

		current_x_ = older.x + (newer.x - older.x) * t;
		current_y_ = older.y + (newer.y - older.y) * t;

		if (t >= 1.0f && snapshots_.size() > 2)
		{
			snapshots_.pop_front();
			interpolation_timer_ = 0.0f;
		}
	}
	else
	{
		// fallback to original behavior
		sync_ratio_.t += dt / latest_state_.time_since_last_update_secs;

		current_x_ = CalculateX(sync_ratio_.t);
		current_y_ = CalculateY(sync_ratio_.t);
	}
}


void SnapshotControl::Draw()
{
	CP_Settings_Stroke(CP_Color_Create(0, 0, 0, 255));
	CP_Settings_Fill(CP_Color_Create(0, 255, 0, 255));
	CP_Graphics_DrawCircle(latest_state_.x, latest_state_.y, 15);
	CP_Graphics_DrawCircle(previous_state_.x, previous_state_.y, 15);
	CP_Graphics_DrawLine(latest_state_.x, latest_state_.y, previous_state_.x, previous_state_.y);
	CP_Graphics_DrawLine(current_x_, current_y_, previous_state_.x, previous_state_.y);
}