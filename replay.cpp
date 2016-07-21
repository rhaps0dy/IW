#include <ale_interface.hpp>
#include<iostream>
#include<fstream>
#include<string>
#include<cstdio>
#include<cassert>
#include<unistd.h>

using namespace std;

int main(int argc, char *argv[]) {
	ALEInterface ale;
	int frame_skip = ale.getInt("frame_skip");
	ale.setInt("frame_skip", 1);

	string record_dir = ale.getString("record_screen_dir");
	ale.setString("record_screen_dir", "");
	ale.loadROM(ale.getString("rom_file"));
	if(argc != 2) {
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
	} else {
		ifstream state_f(argv[1], ios::in|ios::binary);
		if(!state_f.is_open()) {
			cout << "Could not open file: " << argv[1] << endl;
			return -1;
		}
		const string delimiter = "<endstate>";
		string state_seq((istreambuf_iterator<char>(state_f)),
						 istreambuf_iterator<char>());
		state_f.close();
		size_t del_i=state_seq.find(delimiter);
		size_t total=1, i=0;
		while((del_i=state_seq.find(delimiter, del_i+1)) < state_seq.size()) {
			total++;
		}
		putchar('\n');
		while((del_i=state_seq.find(delimiter)) < state_seq.size()) {
			ale.restoreState(ALEState(state_seq.substr(0, del_i)));
			ale.act(PLAYER_A_NOOP);
			state_seq.erase(0, del_i+delimiter.size());
			usleep(16667*5);
			printf("\033[A\n%zu / %zu", total-i-1, total);
			i++;
		}
	}
    return 0;
}
