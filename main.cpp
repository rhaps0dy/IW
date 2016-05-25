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

	string trajectory_suffix=ale.getString("trajectory_suffix");
	unique_ptr<ScreenExporter> s_exp(
		ale.createScreenExporter(ale.getString("record_screen_dir_noauto")));

	ActionVect available_actions;
	if (ale.getBool("use_restricted_action_set"))
		available_actions = ale.romSettings->getMinimalActionSet();
	else
		available_actions = ale.romSettings->getAllActions();

	IW1Search search_tree(*ale.theSettings, available_actions, &ale);
	search_tree.set_novelty_pruning();

	int sim_steps_per_node = ale.getInt("sim_steps_per_node");

	reward_t total_reward = 0;
	ifstream exists("action_reward_" + trajectory_suffix);
	assert(!exists.is_open());
    while(!ale.game_over()) {
		ALEState state=ale.cloneSystemState();
		if (search_tree.is_built ) {
			// Re-use the old tree
			search_tree.move_to_best_sub_branch();
			//assert(search_tree.get_root()->state.equals(state));
			if( search_tree.get_root()->state.equals(state) ){
				//assert(search_tree.get_root()->state.equals(state));
				//assert (search_tree.get_root_frame_number() == state.getFrameNumber());
				search_tree.update_tree();
			
			}
			else{
				//std::cout << "\n\n\tDIFFERENT STATE!\n" << std::endl;
				search_tree.clear(); 
				search_tree.build(state);
			}
		} else 
	    {
			// Build a new Search-Tree
			search_tree.clear(); 
			search_tree.build(state);
		}
		Action action = search_tree.get_best_action();
		ale.restoreSystemState(state);
		for(int i=0; i<sim_steps_per_node; i++) {
			ale.act(action);
			reward_t r = ale.act(action);
			total_reward += r;
			fstream action_reward, statef;
			action_reward.open("action_reward_" + trajectory_suffix, fstream::app);
			action_reward << ale.cloneState().serialize() << "<endstate>";
			action_reward.close();
			statef.open("state_" + trajectory_suffix, fstream::app);
			statef << action << " " << r << " " << total_reward << endl;
			statef.close();
			s_exp->saveNext(ale.getScreen());
		}
    }
    return 0;
}
