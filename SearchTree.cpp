/* *****************************************************************************
 * A.L.E (Arcade Learning Environment)
 * Copyright (c) 2009-2012 by Yavar Naddaf, Joel Veness, Marc G. Bellemare
 * Released under GNU General Public License www.gnu.org/licenses/gpl-3.0.txt
 *
 * Based on: Stella  --  "An Atari 2600 VCS Emulator"
 * Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
 *
 * *****************************************************************************
 *  SearchTree.cpp
 *
 *  Implementation of the SearchTree class, which represents the search tree for the
 *  Search-Agent
 **************************************************************************** */

#include "SearchTree.hpp"
#include "random_tools.h"
#include <emucore/m6502/src/System.hxx>

using namespace std;

/* *********************************************************************
	Constructor
 ******************************************************************* */
SearchTree::SearchTree(Settings & settings, 
		       ActionVect &_actions,
		       StellaEnvironment* _env):
    is_built(false),
    p_root(NULL),
    ignore_duplicates(true),
    reward_magnitude(0),
    available_actions(_actions),
    total_simulation_steps(0)
{
    
    frame_skip = settings.getInt("frame_skip", true);
    max_sim_steps_per_frame = settings.getInt("max_sim_steps_per_frame",false);
    num_simulations_per_frame = settings.getInt("num_simulations_per_frame",false);
    
//    std::cout << "max_sim_steps_per_frame: "<< max_sim_steps_per_frame << std::endl;
    assert(max_sim_steps_per_frame != -1 || settings.getInt("uct_monte_carlo_steps",false) != -1);
    
    discount_factor = settings.getFloat("discount_factor", true);
    // Default: false
    normalize_rewards = settings.getBool("normalize_rewards", false);
    // Default: false
    ignore_duplicates = settings.getBool("ignore_duplicates_nodes", false);
    m_env = _env;
    m_randomize_successor = settings.getBool( "randomize_successor_novelty", false );
    m_novelty_pruning = false;
}



/* *********************************************************************
	Deletes the search-tree
 ******************************************************************* */
void SearchTree::clear(void) {
	if (p_root != NULL) {
		delete_branch(p_root);
		p_root = NULL;
	}
	is_built = false;
	m_max_depth = 0;
}

/* *********************************************************************
	Destructor
 ******************************************************************* */
SearchTree::~SearchTree(void) {
	clear();
}

/* *********************************************************************
	Returns the best action based on the expanded search tree
 ******************************************************************* */
Action SearchTree::get_best_action(void) {
	assert (p_root != NULL);
	int best_branch = p_root->best_branch;
	TreeNode* best_child = p_root->v_children[best_branch];
	assert(best_branch != -1);
	vector<int> best_branches;
	best_branches.push_back(best_branch);

	for (size_t c = 0; c < p_root->v_children.size(); c++) {
		TreeNode* curr_child = p_root->v_children[c];
		// Ignore duplicates if the flag is set
		if (ignore_duplicates && curr_child->is_duplicate()) continue;
				
		if (c != (size_t)best_branch && 
		    curr_child->branch_return == best_child->branch_return && 
		    curr_child->is_terminal == best_child->is_terminal) {
			best_branches.push_back(c);
			
		}
	}
	
	if (best_branches.size() > 1) {
		// best_branch = 0;
		// unsigned best_depth = p_root->v_children[ best_branches[0] ]->branch_depth;
				
		// for( unsigned i = 0; i < best_branches.size(); i++){
		// 	TreeNode* curr_child = p_root->v_children[ best_branches[i] ];

		// 	std::cout << "Action: " << action_to_string(curr_child->act) << "/" << action_to_string( p_root->available_actions[ best_branches[i] ] ) << " Depth: " << curr_child->branch_depth << " NumNodes: " << curr_child->num_nodes() << " Reward: "<< curr_child->branch_return   << std::endl;
		// 	if(best_depth <  curr_child->branch_depth ){
		// 		best_depth = curr_child->branch_depth;
		// 		best_branch = best_branches[i];
		// 	}
		
		// }

	// 	// when we have more than one best-branch, pick one randomly
	 	best_branch = choice(&best_branches);
	}
	TreeNode *nn = p_root;
	std::cout << "Best trajectory:";
	while(nn->v_children.size() != 0) {
		std::cout << " " << action_to_string(nn->available_actions[nn->best_branch]) << " " << nn->node_reward;
		TreeNode *child = nn->v_children[nn->best_branch];
		printf(" (%d, %d)\n", child->getRAM().get(0xAA), child->getRAM().get(0xAB));
		nn = child;
	}
//	std::cout << "\nCurrent position: " << m_env->getRAM().get(0xAA) << ' ' << m_env->getRAM().get(0xAB) << std::endl;
	for (size_t c = 0; c < p_root->v_children.size(); c++) {
		TreeNode* curr_child = p_root->v_children[ c ];

		std::cout << "Action: " << action_to_string(curr_child->act) << " Depth: " << curr_child->branch_depth  <<" NumNodes: " << curr_child->num_nodes() << " Reward: "<< curr_child->branch_return   << std::endl;
	
	}
	
	
	p_root->best_branch = best_branch;
	
	return p_root->available_actions[best_branch];
}


/* *********************************************************************
	Moves the best sub-branch of the root to be the new root of the tree
 ******************************************************************* */
void SearchTree::move_to_best_sub_branch(void) {
	assert(p_root->v_children.size() > 0);
	assert(p_root->best_branch != -1);
	
	// Delete all the other branches
	for (size_t del = 0; del < p_root->v_children.size(); del++) {
		if (del != (size_t)p_root->best_branch) {
			delete_branch(p_root->v_children[del]);
		}
	}

	TreeNode* old_root = p_root;
	p_root = p_root->v_children[p_root->best_branch];
	// make sure the child I want to become root doesn't get deleted:
	old_root->v_children[old_root->best_branch] = NULL;
	delete old_root;
	p_root->p_parent = NULL;
	m_max_depth = 0;
}


/* *********************************************************************
	Deletes a node and all its children, all the way down the branch
 ******************************************************************* */
void SearchTree::delete_branch(TreeNode* node) {
	if (!node->v_children.empty()) {
		for(size_t c = 0; c < node->v_children.size(); c++) {
			delete_branch(node->v_children[c]);
		}
	}
	delete node;

}

bool SearchTree::test_duplicate(TreeNode *node) {
  if (node->p_parent == NULL) return false;
  else {
    TreeNode * parent = node->p_parent;

    // Compare each valid sibling with this one
    for (size_t c = 0; c < parent->v_children.size(); c++) {
      TreeNode * sibling = parent->v_children[c];
      // Ignore duplicates, this node and uninitialized nodes
      if (sibling->is_duplicate() || sibling == node || 
        !sibling->is_initialized()) continue;

      if (sibling->state.equals(node->state)) {
        node->duplicate = true;
        return true;
      }
    }

    // None of the siblings match, unique node
    node->duplicate = false;
    return false;
  }
}

int SearchTree::simulate_game(	ALEState & state, ALERAM &ram, Action act, int num_steps, 
			       	return_t &traj_return, bool &game_ended, 
                		bool discount_return, bool save_state) {

	// Load the state into the emulator - a copy of the parent state
	m_env->restoreSystemState(state);


	// For discounting purposes
	float g = 1.0;
	traj_return = 0.0;
	game_ended = false;
	Action a;

	// So that the compiler doesn't complain
	if (act == RANDOM) a = PLAYER_A_NOOP; 
	else a = act; 

	int i;

	for (i = 0; i < num_steps; i++) {
		if (act == RANDOM && i%frame_skip == 0)
			a = choice(&available_actions);

		// Move state forward using action a
		reward_t curr_reward = m_env->oneStepAct( a, PLAYER_B_NOOP);
		game_ended = m_env->isTerminal();
			
		return_t r = normalize_rewards ? normalize(curr_reward) : curr_reward;
    
		// Add curr_reward to the trajectory return
		if (discount_return) {
			traj_return += r * g;
			if((i+1)%frame_skip == 0)
				g *= discount_factor;
		}
		else
			traj_return += r;

		// Early exit if we reach termination
		if (game_ended) {
			i++;
			break;
		}
	}

	// Save the result
	if(save_state) {
		state = m_env->cloneSystemState();
		for(int i=0x80; i<0x100; i++)
			*ram.byte(i) = m_env->getSystem().peek(i);
	}

	return i;
}

return_t SearchTree::normalize(reward_t reward) {
  if (reward == 0) return 0;
  else {
    // Set reward magnitude to first observed non-zero reward
    if (reward_magnitude == 0)
      reward_magnitude = abs(reward);
    return (reward + 0.0) / reward_magnitude;
  }
  
}

int SearchTree::num_nodes() {
  if (p_root == NULL) return 0;
  else return p_root->num_nodes();
}

void SearchTree::print_best_path() {
  if (p_root == NULL) cerr << "no path" << endl;
  else {
    TreeNode * node = p_root;

    cerr << "Path ";
    while (!node->v_children.empty()) {
      // Print action taken at node
      cerr << node->best_branch << " ";
      node = node->v_children[node->best_branch];
    }

    cerr << endl;
  }
}

int SearchTree::get_root_frame_number() {
  return p_root->state.getFrameNumber();
}

long SearchTree::num_simulation_steps() {
  long s = total_simulation_steps;
  total_simulation_steps = 0;

  return s;
}

void SearchTree::print_frame_data( int frame_number, float elapsed, Action curr_action, std::ostream& output )
{
	output << "frame=" << frame_number;
	output << ",expanded=" << expanded_nodes();
	output << ",generated=" << generated_nodes();
	output << ",depth_tree=" << max_depth();
	output << ",tree_size=" <<  num_nodes(); 
	output << ",best_action=" << action_to_string( curr_action );
	output << ",branch_reward=" << get_root_value();
	output << ",elapsed=" << elapsed;
	output << std::endl;
}
