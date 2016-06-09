#include <ale_interface.hpp>
#include<iostream>
#include<sstream>
#include<iomanip>
#include<iterator>
#include<fstream>
#include<string>
#include "IW1Search.hpp"
#include "IW3OnlySearch.hpp"
#include<cassert>
#include<memory>
#include<stdexcept>
#include<cstdlib>

using namespace std;

int main(int argc, char *argv[]) {
	ALEInterface ale;
	ale.loadROM(ale.getString("rom_file"));

	string trajectory_suffix = ale.getString("trajectory_suffix");
	string record_dir = ale.getString("record_screen_dir");

	ActionVect available_actions = {PLAYER_A_NOOP, PLAYER_A_FIRE};
	if (ale.getBool("use_restricted_action_set"))
		available_actions = ale.romSettings->getMinimalActionSet();
	else
		available_actions = ale.romSettings->getAllActions();

	IW3OnlySearch search_tree(*ale.theSettings, available_actions, ale.environment.get());
	search_tree.set_novelty_pruning();
#if SHOW_SEARCH
	search_tree.aleint = &ale;
#endif

	reward_t total_reward = 0;
	ifstream exists(record_dir + "/action_reward_" + trajectory_suffix);
	assert(!exists.is_open());

	if(argc == 2) {
		cout << "Attempting to load last state from file: " << argv[1] << endl;
		ifstream state_f(argv[1], ios::in|ios::binary);
		string delimiter = "<endstate>";
		string state_seq((istreambuf_iterator<char>(state_f)),
				istreambuf_iterator<char>());
		state_f.close();
		for(int i=0; i<20; i++)
			state_seq.erase(state_seq.rfind(delimiter));
		string state_s = state_seq.substr(
				state_seq.rfind(delimiter)+delimiter.size(),
				string::npos);

		try {
			ale.restoreState(ALEState(state_s));
		} catch(const runtime_error *e) {
			ale.restoreSystemState(ALEState(state_s));
		}
	}
	// randomise initial situation
	for(int i=0; i<rand()%30; i++)
		ale.act(PLAYER_A_NOOP);
    while(!ale.game_over()) {
		ALEState state=ale.cloneSystemState();
		const ALEScreen saved_screen=ale.getScreen();
		if (search_tree.is_built ) {
			// Re-use the old tree
			search_tree.move_to_best_sub_branch();
			if(search_tree.get_root()->state.equals(state)) {
				search_tree.update_tree();
			} else {
				search_tree.clear();
				search_tree.build(state);
			}
		} else {
			// Build a new Search-Tree
			search_tree.clear();
			search_tree.build(state);
		}
		Action action = search_tree.get_best_action();
		ale.restoreSystemState(state);
		ale.environment->m_screen = saved_screen;
		reward_t r = ale.act(action);
		total_reward += r;
		fstream action_reward, statef;
		action_reward.open(record_dir + "/action_reward_" + trajectory_suffix, fstream::app);
		action_reward << static_cast<int>(action) << " " << r << " " << total_reward << endl;
		action_reward.close();
		statef.open(record_dir + "/state_" + trajectory_suffix, fstream::app);
		statef << ale.cloneSystemState().serialize() << "<endstate>";
		statef.close();
    }
    return 0;
}
