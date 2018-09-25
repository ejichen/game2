#pragma once

#include <glm/glm.hpp>
#include <vector>

struct Game {
	glm::vec2 paddle = glm::vec2(0.0f,-3.0f);
	glm::vec2 ball = glm::vec2(0.0f, 0.0f);
	glm::vec2 ball_velocity = glm::vec2(0.0f,-2.0f);
  // store values 0 1 2 indicating gray, red, blue
	int rod_num = 38;
	std::vector<int> rod_meshes = std::vector<int>(rod_num, 0);
	int chaned_index = 0;
	int chaned_color = 0;
	void update(float time);

	static constexpr const float FrameWidth = 10.0f;
	static constexpr const float FrameHeight = 8.0f;
	static constexpr const float PaddleWidth = 2.0f;
	static constexpr const float PaddleHeight = 0.4f;
	static constexpr const float BallRadius = 0.5f;
};
