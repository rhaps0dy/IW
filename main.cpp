#include <ale_interface.hpp>
#include<iostream>
#include<sstream>
#include<iomanip>
#include<fstream>
#include<string>
#include "IW1Search.hpp"
#include<cassert>
#include<memory>

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

	IW1Search search_tree(*ale.theSettings, available_actions, ale.environment.get());
	search_tree.set_novelty_pruning();

	reward_t total_reward = 0;
	ifstream exists(record_dir + "/action_reward_" + trajectory_suffix);
	assert(!exists.is_open());

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
		statef << ale.cloneState().serialize() << "<endstate>";
		statef.close();
    }
    return 0;
}
