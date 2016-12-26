#include <ale_interface.hpp>
#include<iostream>
#include<sstream>
#include<iomanip>
#include<iterator>
#include<fstream>
#include<string>
#include "AbstractIWSearch.hpp"
#include "IW1Search.hpp"
#include "IW3OnlySearch.hpp"
#include<cassert>
#include<memory>
#include<stdexcept>
#include<cstdlib>
#include<deque>

// define if you wish to output <n> exparr.txt files, which then can be
// processed with all_exparr.py or exparr.py
// #define NUM_EXPARRS 12

using namespace std;

int main(int argc, char *argv[]) {
	ALEInterface ale;
	ale.loadROM(ale.getString("rom_file"));
	// randomise initial situation
	for(int i=-1; i<rand()%30; i++)
		ale.act(PLAYER_A_NOOP);

	string trajectory_suffix = ale.getString("trajectory_suffix");
	string record_dir = ale.getString("record_screen_dir");

	ActionVect available_actions = {PLAYER_A_NOOP, PLAYER_A_FIRE};
	if (ale.getBool("use_restricted_action_set"))
		available_actions = ale.romSettings->getMinimalActionSet();
	else
		available_actions = ale.romSettings->getAllActions();
	const int n_states_back = ale.getInt("n_states_back");
	const int actions_no_recalc = ale.getInt("actions_no_recalc");

	AbstractIWSearch *search_tree;
	string search_method = ale.getString("search_method");
	if(search_method == "iw3") {
		search_tree = new IW3OnlySearch(*ale.theSettings, available_actions, ale.environment.get());
	} else if(search_method == "iw1") {
		search_tree = new IW1Search(*ale.theSettings, available_actions, ale.environment.get());
	}
#if SHOW_SEARCH
	search_tree->aleint = &ale;
#endif

	reward_t total_reward = 0;
	ifstream exists(record_dir + "/action_reward_" + trajectory_suffix);
	assert(!exists.is_open());

	if(argc == 2) {
		cout << "Attempting to load last state from file: " << argv[1] << endl;
		ifstream state_f(argv[1], ios::in|ios::binary);
		const string delimiter = "<endstate>";
		string state_seq((istreambuf_iterator<char>(state_f)),
				istreambuf_iterator<char>());
		state_f.close();
		for(int i=0; i<n_states_back; i++)
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
	int n_iter=0;
	Action action = UNDEFINED;
	deque<Action> sequence_old;
	deque<return_t> sequence_return_old ;
	sequence_return_old.push_front(-10000000);
    while(!ale.game_over()) {
		ALEState state=ale.cloneState();
		const ALEScreen saved_screen=ale.getScreen();
		if (search_tree->is_built ) {
			// Re-use the old tree
			if(n_iter % actions_no_recalc == 0) {
				if(search_tree->get_root()->state.equals(state)) {
					search_tree->update_tree();
				} else {
					fstream action_reward;
					action_reward.open(record_dir + "/action_reward_" + trajectory_suffix, fstream::app);
					action_reward << "Search tree does not coincide\n";
					cout << "Search tree does not coincide\n";
					action_reward.close();
					search_tree->clear();
					search_tree->build(state);
				}
			}
		} else {
			// Build a new Search-Tree
			search_tree->clear();
			search_tree->build(state);
		}
		ale.restoreState(state);
		ale.environment->m_screen = saved_screen;
		deque<Action> sequence;
		deque<return_t> sequence_return;
		search_tree->get_best_action(sequence, sequence_return);
		if(sequence_return_old.empty() ||
			sequence_return.front() >= sequence_return_old.front()) {
			sequence_old = sequence;
			sequence_return_old = sequence_return;
			cout << "New action sequence:\n";
			for(size_t i = sequence.size()-1; i<sequence.size(); i--)
				cout << " " << action_to_string(sequence[i]) << " " << sequence_return[i] << endl;
		}
		action = sequence_old.front();
		sequence_old.pop_front();
		sequence_return_old.pop_front();
		if(action == UNDEFINED) {
			n_iter = 0;
			continue;
		}
		search_tree->move_to_best_sub_branch();

		reward_t r = ale.act(action);
		total_reward += r;
		fstream action_reward, statef;
		action_reward.open(record_dir + "/action_reward_" + trajectory_suffix, fstream::app);
#ifdef NUM_EXPARRS
		action_reward << (int)ale.getRAM().get(0xaa) << ' ' << (int)ale.getRAM().get(0xab) << '\n';
#else
		action_reward << static_cast<int>(action) << " " << r << " " << total_reward << endl;
#endif
		action_reward.close();
		statef.open(record_dir + "/state_" + trajectory_suffix, fstream::app);
		statef << ale.cloneState().serialize() << "<endstate>";
		statef.close();
#ifdef NUM_EXPARRS
		if(n_iter >= NUM_EXPARRS*actions_no_recalc)
			return 0;
#endif
		n_iter++;
    }
    return 0;
}
