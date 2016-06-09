#include <ale_interface.hpp>
#include<iostream>
#include<fstream>
#include<string>
#include<cassert>

using namespace std;

int main(int argc, char *argv[]) {
	ALEInterface ale;
	int frame_skip = ale.getInt("frame_skip");
	ale.setInt("frame_skip", 1);

	string record_dir = ale.getString("record_screen_dir");
	ale.setString("record_screen_dir", "");

	ale.loadROM(ale.getString("rom_file"));

	string trajectory_suffix = ale.getString("trajectory_suffix");
	ifstream action_reward(record_dir + "/action_reward_" + trajectory_suffix);
	assert(action_reward.is_open());

	int action, reward, reward_total;
    while(!ale.game_over() && (
			  action_reward >> action >> reward >> reward_total)) {
		reward_t r = 0;
		for(int i=0; i<frame_skip; i++) {
			r += ale.act(static_cast<Action>(action));
		}
    }
	action_reward.close();
    return 0;
}
