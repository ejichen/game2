#pragma once

#include "Mode.hpp"

#include "MeshBuffer.hpp"
#include "GL.hpp"
#include "Connection.hpp"
#include "Game.hpp"

#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <utility>
#include <vector>

// The 'GameMode' mode is the main gameplay mode:

struct GameMode : public Mode {
	GameMode(Client &client);
	virtual ~GameMode();

	//handle_event is called when new mouse or keyboard events are received:
	// (note that this might be many times per frame or never)
	//The function should return 'true' if it handled the event.
	virtual bool handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) override;

	//update is called at the start of a new frame, after events are handled:
	virtual void update(float elapsed) override;

	//draw is called after update:
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//------- game state -------
	Game state;
	bool mouse_slide = false;
	bool mouse_click = false;
	int board_x = 5;
	int board_y = 4;
	int cur_x, cur_y;
	glm::uvec2 board_size = glm::uvec2(board_x, board_y);
	std::vector< MeshBuffer::Mesh const * > board_meshes;
	enum rod_color{Gray, Red, Blue};
	int rod_num = (board_x+1) * (board_y-1) + board_x * board_y;
	//hard code the table for rods, including the color and the bounding box
	std::vector<std::pair<int, std::vector<int>>> rod_table;
	bool got_cliked(std::vector<int> bbox, int x_cursor, int y_cursor);



	//------ networking ------
	Client &client; //client object; manages connection to server.
};
