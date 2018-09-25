#include "Connection.hpp"
#include "Game.hpp"

#include <iostream>
#include <set>
#include <chrono>

int main(int argc, char **argv) {
	if (argc != 2) {
		std::cerr << "Usage:\n\t./server <port>" << std::endl;
		return 1;
	}

	Server server(argv[1]);

	Game state;

	while (1) {
		server.poll([&](Connection *c, Connection::Event evt){
			if (evt == Connection::OnOpen) {
			} else if (evt == Connection::OnClose) {
			} else { assert(evt == Connection::OnRecv);
				if (c->recv_buffer[0] == 'h') {
					c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1);
					std::cout << c << ": Got hello." << std::endl;
				} else if (c->recv_buffer[0] == 's') {
					if (c->recv_buffer.size() < 1 + sizeof(float)) {
						return; //wait for more data
					} else {
						memcpy(&state.paddle.x, c->recv_buffer.data() + 1, sizeof(float));
						// the update is coming from the first client
						if(c == &server.connections.front()){
							server.connections.back().send_raw("t", 1);
							server.connections.back().send_raw(&state.paddle.x, sizeof(float));
						}
						else if (c == &server.connections.back()){
							server.connections.front().send_raw("t", 1);
							server.connections.front().send_raw(&state.paddle.x, sizeof(float));
						}
						else{
							std::cout << "someone else send the message" << std::endl;
						}

						c->recv_buffer.erase(c->recv_buffer.begin(), c->recv_buffer.begin() + 1 + sizeof(float));
						c->recv_buffer.clear();
					}
				}
			}
		}, 0.025);
		//every second or so, dump the current paddle position:
		static auto then = std::chrono::steady_clock::now();
		auto now = std::chrono::steady_clock::now();
		if (now > then + std::chrono::seconds(1)) {
			then = now;
			std::cout << "Current paddle position: " << state.paddle.x << std::endl;

		}
	}
}
