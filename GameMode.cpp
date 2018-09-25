#include "GameMode.hpp"

#include "MenuMode.hpp"
#include "Load.hpp"
#include "MeshBuffer.hpp"
#include "Scene.hpp"
#include "gl_errors.hpp" //helper for dumpping OpenGL error messages
#include "read_chunk.hpp" //helper for reading a vector of structures from a file
#include "data_path.hpp" //helper to get paths relative to executable
#include "compile_program.hpp" //helper to compile opengl shader programs
#include "draw_text.hpp" //helper to... um.. draw text
#include "vertex_color_program.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <map>
#include <cstddef>
#include <random>

std::vector< MeshBuffer::Mesh const * > rod_meshes;
MeshBuffer::Mesh tile_mesh;
MeshBuffer::Mesh rodRed_mesh;
MeshBuffer::Mesh rodBlue_mesh;
MeshBuffer::Mesh rodGray_mesh;

Load< MeshBuffer > meshes(LoadTagDefault, [](){
	MeshBuffer const *ret = new MeshBuffer(data_path("paddle-ball.pnc"));
	tile_mesh = ret->lookup("Tile");
	rodRed_mesh = ret->lookup("RodRed");
	rodBlue_mesh = ret->lookup("RodBlue");
	rodGray_mesh = ret->lookup("RodGray");
	rod_meshes.push_back(&rodGray_mesh);
	rod_meshes.push_back(&rodRed_mesh);
	rod_meshes.push_back(&rodBlue_mesh);
	return ret;
});



Load< GLuint > meshes_for_vertex_color_program(LoadTagDefault, [](){
	return new GLuint(meshes->make_vao_for_program(vertex_color_program->program));
});

Scene::Transform *paddle_transform = nullptr;
Scene::Transform *ball_transform = nullptr;
Scene::Camera *camera = nullptr;


Load< Scene > scene(LoadTagDefault, [](){
	Scene *ret = new Scene;
	//load transform hierarchy:
	ret->load(data_path("paddle-ball.scene"), [](Scene &s, Scene::Transform *t, std::string const &m){
		if (t->name == "Paddle"){


			Scene::Object *obj = s.new_object(t);

			obj->program = vertex_color_program->program;
			obj->program_mvp_mat4  = vertex_color_program->object_to_clip_mat4;
			obj->program_mv_mat4x3 = vertex_color_program->object_to_light_mat4x3;
			obj->program_itmv_mat3 = vertex_color_program->normal_to_light_mat3;

			MeshBuffer::Mesh const &mesh = meshes->lookup(m);
			obj->vao = *meshes_for_vertex_color_program;
			obj->start = mesh.start;
			obj->count = mesh.count;
		}
	});

	//look up paddle and ball transforms:
	for (Scene::Transform *t = ret->first_transform; t != nullptr; t = t->alloc_next) {
		if (t->name == "Paddle") {
			if (paddle_transform) throw std::runtime_error("Multiple 'Paddle' transforms in scene.");
			paddle_transform = t;
		}
		if (t->name == "Ball") {
			if (ball_transform) throw std::runtime_error("Multiple 'Ball' transforms in scene.");
			ball_transform = t;
		}

	}
	if (!paddle_transform) throw std::runtime_error("No 'Paddle' transform in scene.");
	if (!ball_transform) throw std::runtime_error("No 'Ball' transform in scene.");
	// look up the camera:
	for (Scene::Camera *c = ret->first_camera; c != nullptr; c = c->alloc_next) {
		if (c->transform->name == "Camera") {
			if (camera) throw std::runtime_error("Multiple 'Camera' objects in scene.");
			camera = c;
		}
	}
	if (!camera) throw std::runtime_error("No 'Camera' camera in scene.");
	return ret;
});

GameMode::GameMode(Client &client_) : client(client_) {
	board_meshes.reserve(board_size.x * board_size.y);
	rod_table.reserve(rod_num);
	rod_color col = Gray;
	int xmax = 160; int xmin = 80; int ymax = 60; int ymin = 40;
	for (uint32_t y = 0; y < board_size.y; ++y) {
		for (uint32_t x = 0; x < board_size.x; ++x) {
			//bbox = [xmax, ymax, xmin, ymin]
			std::vector<int> bbox{xmax, ymax, xmin ,ymin};
			//intialize the color as gray
			rod_table.push_back(std::make_pair(col, bbox));
			xmax += 100;
			xmin += 100;


		}
		xmax = 160;
		xmin = 80;
		ymax += 100;
		ymin += 100;
	}

	xmax = 80; xmin = 60; ymax = 140; ymin = 60;
	for (uint32_t y = 1; y < board_size.y; ++y) {
		for (uint32_t x = 0; x < (board_size.x+1); ++x) {
			std::vector<int> bbox{xmax, ymax, xmin ,ymin};
			rod_table.push_back(std::make_pair(col, bbox));
			xmax += 100;
			xmin += 100;

		}
		xmax = 80;
		xmin = 60;
		ymax += 100;
		ymin += 100;
	}
	client.connection.send_raw("h", 1); //send a 'hello' to the server
}

GameMode::~GameMode() {
}


bool GameMode::got_cliked(std::vector<int> bbox, int x_cursor, int y_cursor){
	return bbox[0] - x_cursor >= 0 && bbox[1] - y_cursor >= 0 && bbox[2] - x_cursor <= 0 && bbox[3] - y_cursor <= 0;
}


bool GameMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	//ignore any keys that are the result of automatic key repeat:

	if (evt.type == SDL_KEYDOWN && evt.key.repeat) {
		return false;
	}

	// if (evt.type == SDL_MOUSEMOTION) {
	// 	state.paddle.x = (evt.motion.x - 0.5f * window_size.x) / (0.5f * window_size.x) * Game::FrameWidth;
	// 	state.paddle.x = std::max(state.paddle.x, -0.5f * Game::FrameWidth + 0.5f * Game::PaddleWidth);
	// 	state.paddle.x = std::min(state.paddle.x,  0.5f * Game::FrameWidth - 0.5f * Game::PaddleWidth);
	// 	mouse_slide = evt.type;
	// 	return true;
	// }

	if (evt.type == SDL_MOUSEBUTTONDOWN && SDL_BUTTON(SDL_GetMouseState(&cur_x, &cur_y))) {
		for(int i = 0; i < rod_num; i++){
			if(got_cliked(rod_table[i].second, cur_x, cur_y)) {
				std::cout << "click: " << i << " at  " << cur_x << " " << cur_y << std::endl;
				state.chaned_index = i;
				mouse_click = evt.type;
				return true;
			}
		}
		return false;
	}

	return false;
}

void GameMode::update(float elapsed) {
	state.update(elapsed);

	// if (client.connection) {
	// if (mouse_slide) {
	// 	//send game state to server:
	// 	client.connection.send_raw("s", 1);
	// 	client.connection.send_raw(&state.paddle.x, sizeof(float));
	// }
	if(mouse_click && rod_table[state.chaned_index].first == 0){
		//change the color
		std::cout << "sending messge: " << state.chaned_index << std::endl;
		client.connection.send_raw("c", 1);
		client.connection.send_raw(&state.chaned_index, sizeof(int));
	}
	client.poll([&](Connection *c, Connection::Event event){
		if (event == Connection::OnOpen) {
			//probably won't get this.
		} else if (event == Connection::OnClose) {
			std::cerr << "Lost connection to server." << std::endl;
		} else { assert(event == Connection::OnRecv);
			while(!(c->recv_buffer.empty())){
			// if (*(c->recv_buffer.begin()) == 't') {
			// 	if (c->recv_buffer.size() < 1 + sizeof(float)) {
			// 		return; //wait for more data
			// 	} else {
			// 		memcpy(&state.paddle.x, c->recv_buffer.data() + 1, sizeof(float));
			// 		c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(float));
			// 	}
			// }
			if (*(c->recv_buffer.begin()) == 'i') {
				if (c->recv_buffer.size() < 1 + sizeof(int)) {
					return; //wait for more data
				} else {
					memcpy(&state.chaned_color, c->recv_buffer.data() + 1, sizeof(int));
					memcpy(&state.chaned_index, c->recv_buffer.data() + 1 + sizeof(int), sizeof(int));
					rod_table[state.chaned_index].first = state.chaned_color;
					c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + 2 * sizeof(int));
					std::cerr << "receive " << state.chaned_color << " " << state.chaned_index << " index from server" << std::endl;
				}
			}
			// c->recv_buffer.clear();
		}
	}
	});

	// //copy game state to scene positions:
	// ball_transform->position.x = state.ball.x;
	// ball_transform->position.y = state.ball.y;
	//
	// paddle_transform->position.x = state.paddle.x;
	// paddle_transform->position.y = state.paddle.y;
}

void GameMode::draw(glm::uvec2 const &drawable_size) {
	camera->aspect = drawable_size.x / float(drawable_size.y);

	glClearColor(0.20f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//set up basic OpenGL state:
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glm::mat4 world_to_clip;
	{
		float aspect = float(drawable_size.x) / float(drawable_size.y);

		//want scale such that board * scale fits in [-aspect,aspect]x[-1.0,1.0] screen box:
		float scale = glm::min(
			2.0f * aspect / float(board_size.x),
			2.0f / float(board_size.y)
		);

		//center of board will be placed at center of screen:
		glm::vec2 center = 0.5f * glm::vec2(board_size);

		//NOTE: glm matrices are specified in column-major order
		world_to_clip = glm::mat4(
			scale / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, scale, 0.0f, 0.0f,
			0.0f, 0.0f,-1.0f, 0.0f,
			-(scale / aspect) * center.x, -scale * center.y, 0.0f, 1.0f
		);
	}
	//set up light positions:
	glUseProgram(vertex_color_program->program);

	glUniform3fv(vertex_color_program->sun_color_vec3, 1, glm::value_ptr(glm::vec3(0.81f, 0.81f, 0.76f)));
	glUniform3fv(vertex_color_program->sun_direction_vec3, 1, glm::value_ptr(glm::normalize(glm::vec3(-0.2f, 0.2f, 1.0f))));
	glUniform3fv(vertex_color_program->sky_color_vec3, 1, glm::value_ptr(glm::vec3(0.2f, 0.2f, 0.3f)));
	glUniform3fv(vertex_color_program->sky_direction_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 1.0f, 0.0f)));
	//helper function to draw a given mesh with a given transformation:
	auto draw_mesh = [&](MeshBuffer::Mesh const &mesh, glm::mat4 const &object_to_world) {
		//set up the matrix uniforms:
		if (vertex_color_program->object_to_clip_mat4 != -1U) {
			glm::mat4 object_to_clip = world_to_clip * object_to_world;
			glUniformMatrix4fv(vertex_color_program->object_to_clip_mat4, 1, GL_FALSE, glm::value_ptr(object_to_clip));
		}
		if (vertex_color_program->object_to_light_mat4x3 != -1U) {
			glUniformMatrix4x3fv(vertex_color_program->object_to_light_mat4x3, 1, GL_FALSE, glm::value_ptr(object_to_world));
		}
		if (vertex_color_program->normal_to_light_mat3 != -1U) {
			//NOTE: if there isn't any non-uniform scaling in the object_to_world matrix, then the inverse transpose is the matrix itself, and computing it wastes some CPU time:
			glm::mat3 normal_to_world = glm::inverse(glm::transpose(glm::mat3(object_to_world)));
			glUniformMatrix3fv(vertex_color_program->normal_to_light_mat3, 1, GL_FALSE, glm::value_ptr(normal_to_world));
		}

		//draw the mesh:
		glDrawArrays(GL_TRIANGLES, mesh.start, mesh.count);
	};
	for (uint32_t y = 0; y < board_size.y; ++y) {
		for (uint32_t x = 0; x < board_size.x; ++x) {
			// //bbox = [xmax, ymax, xmin, ymin]
			// std::vector<float> bbox{x+rod_length, y+rod_width, x-rod_length ,y-rod_width};
			// //intialize the color as gray
			// rod_table.push_back(std::make_pair(2, bbox));
			int idx = y*board_x + x;
			// std::cout << "idx: " << idx << std::endl;
			draw_mesh(*rod_meshes[rod_table[idx].first],
				glm::mat4(
					0.0f, 0.0f, 1.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					-1.0f, 0.0f, 0.0f, 0.0f,
					x+0.5f, y+0.5f, -1.0f, 1.0f
				)
			);
		}
	}
	for (uint32_t y = 1; y < board_size.y; ++y) {
		for (uint32_t x = 0; x < (board_size.x+1); ++x) {
			// std::vector<float> bbox{x+rod_width, y+rod_length, x-rod_width ,y-rod_length};
			// rod_table.push_back(std::make_pair(1, bbox));
			int idx = 20+((y-1)*(board_x+1))+x;
			// std::cout << "idx: " << idx << std::endl;
			draw_mesh(*rod_meshes[rod_table[idx].first],
				glm::mat4(
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 0.0f, -1.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					x, y, -1.0f, 1.0f
				)
			);
		}
	}
	// for (int i = 0; i < rod_num; i++){
	// 	rod_table[i].second = std::vector<int>{};
	// }

	scene->draw(camera);
	GL_ERRORS();
}
