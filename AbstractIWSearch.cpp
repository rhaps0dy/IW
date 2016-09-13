#include "AbstractIWSearch.hpp"
#include "SearchTree.hpp"
#include <list>
#include <cassert>
#include <utility>
#include <random>
#include <cstdio>

using namespace std;

AbstractIWSearch::AbstractIWSearch(Settings &settings,
			       ActionVect &actions, StellaEnvironment* _env) :
	SearchTree(settings, actions, _env), expanded_arr(HEIGHT*N_HEIGHT*WIDTH*N_WIDTH, 0) {
	m_stop_on_first_reward = settings.getBool( "iw_stop_on_first_reward", true );
	int val = settings.getInt( "iw_reward_horizon", -1 );
	m_reward_horizon = ( val < 0 ? std::numeric_limits<unsigned>::max() : val );
	m_randomize_noop = false;
}

// Builds a new tree
void AbstractIWSearch::build(ALEState & state) {
	assert(p_root == NULL);
	p_root = new TreeNode(NULL, state, 0, NULL, UNDEFINED, 0);
	update_tree();
	is_built = true;
}

void AbstractIWSearch::print_path(TreeNode * node, int a) {
	cerr << "Path, return " << node->v_children[a]->branch_return << endl;

	while (!node->is_leaf()) {
		TreeNode * child = node->v_children[a];

		cerr << "\tAction " << a << " Reward " << child->node_reward <<
			" Return " << child->branch_return <<
			" Terminal " << child->is_terminal << endl;

		node = child;
		if (!node->is_leaf())
			a = node->best_branch;
	}
}

void AbstractIWSearch::update_tree() {
	expand_tree(p_root);
}


int AbstractIWSearch::expand_node(
	TreeNode* curr_node, TNDeque& q, TNDeque& low_q)
{
	int num_simulated_steps = 0;
	size_t num_actions = available_actions.size();
	size_t initial_a = curr_node->v_children.size();
	bool children_need_simulation = initial_a <= 1;
	if(!children_need_simulation || m_randomize_noop)
		initial_a = 0;
	assert(frame_skip != 0);
	m_expanded_nodes++;
	// Expand all of its children (simulates the result)
	if(children_need_simulation){
		curr_node->v_children.resize( num_actions );
		curr_node->available_actions = available_actions;
		if(m_randomize_successor)
		    std::random_shuffle(curr_node->available_actions.begin() + initial_a,
								curr_node->available_actions.end());
	}
#ifdef PRINT
	std::cout << "Expanding node (" <<
		(int)curr_node->getRAM().get(RAM_X) << ", " <<
		(int)curr_node->getRAM().get(RAM_Y) << ")\n";
#endif
#ifdef OUTPUT_EXPLORE
	int n=curr_node->getRAM().get(RAM_SCREEN);
	auto y_pos = curr_node->getRAM().get(RAM_Y);
	auto x_pos = curr_node->getRAM().get(RAM_X);
	if(n < N_SCREENS && y_pos < HEIGHT && x_pos < WIDTH) {
		int xoffs, yoffs;
		if(N_SCREENS == 24) {
			// Means we are in Montezuma's Revenge
			if(n < 8) {
				if(n < 3) xoffs=3+n, yoffs=0;
				else xoffs=n-1, yoffs=1;
			} else {
				if(n < 15) xoffs=n-7, yoffs=2;
				else xoffs=n-15, yoffs=3;
			}
		} else {
			// Means we are in Private Eye
			xoffs = n % N_WIDTH;
			yoffs = n / N_WIDTH;
		}
		expanded_arr.at(
			(HEIGHT*yoffs + HEIGHT-(y_pos + 1))*N_WIDTH*WIDTH +
			(WIDTH *xoffs + x_pos))++;
	}
#endif
	for (size_t a = initial_a; a < num_actions; a++) {
		Action act = curr_node->available_actions[a];
		TreeNode * child;

		// If re-expanding an internal node, don't creates new nodes
		if (children_need_simulation) {
			m_generated_nodes++;
			child = new TreeNode(curr_node,
						curr_node->state,
						m_current_nid++,
						this,
						act,
						frame_skip);
			if(check_novelty(child->getRAM(), curr_node->nid)) {
				// child->is_terminal is already false
				child->is_terminal = false;
			} else {
				curr_node->v_children[a] = child;
				child->is_terminal = true;
				m_pruned_nodes++;
			}
			if (child->depth() > m_max_depth ) m_max_depth = child->depth();
			num_simulated_steps += child->num_simulated_steps;
			curr_node->v_children[a] = child;
		} else {
			child = curr_node->v_children[a];
			child->nid = m_current_nid++;

			// This recreates the novelty table (which gets resetted every time
			// we change the root of the search tree)
			if( child->is_terminal ) {
				if(check_novelty(child->getRAM(), curr_node->nid)) {
				    child->is_terminal = false;
				} else {
				    child->is_terminal = true;
				    m_pruned_nodes++;
				}
			}
			child->updateTreeNode();

			if (child->depth() > m_max_depth ) m_max_depth = child->depth();

			// DONT COUNT REUSED NODES
			//if ( !child->is_terminal )
			//	num_simulated_steps += child->num_simulated_steps;

		}

#ifdef PRINT
		std::cout << "\tQueuing child " << action_to_string(act) << " (" <<
			(int)child->getRAM().get(RAM_X) << ", " <<
			(int)child->getRAM().get(RAM_Y) << ") ";
#endif
		// Don't expand duplicate nodes, or terminal nodes
		if(!child->is_terminal) {
			// nodes marked terminal do not need their novelty updated.
			update_novelty( child->getRAM(), child->nid );
			if((! (ignore_duplicates && test_duplicate(child)) ) &&
			   ( child->num_nodes_reusable < max_nodes_per_frame )) {
				// if life is lost, put it in the lower priority deque
				enqueue_node(curr_node, child, q, low_q, act);
			}
#ifdef PRINT
			std::cout << "EXPANDED\n";
		} else {
			std::cout << "PRUNED\n";
#endif
		}
	}
	num_simulated_steps += expanded_node_postprocessing(curr_node, q, low_q);
	return num_simulated_steps;
}
/* *********************************************************************
   Expands the tree from the given node until i_max_sim_steps_per_frame
   is reached
   ******************************************************************* */
void AbstractIWSearch::expand_tree(TreeNode* start_node) {
	if(!start_node->v_children.empty()){
	    start_node->updateTreeNode();
	    for (int a = 0; a < start_node->v_children.size(); a++) {
			TreeNode* child = start_node->v_children[a];
			if( !child->is_terminal ){
		        child->num_nodes_reusable = child->num_nodes();
		}
	    }
	}
	start_node->nid = m_current_nid++;
#ifdef OUTPUT_EXPLORE
	fill(expanded_arr.begin(), expanded_arr.end(), 0);
#endif // OUTPUT_EXPLORE


	// low_q contains nodes that lose a life
	TNDeque q, low_q;
	std::list< TreeNode* > pivots;

	//q.push(start_node);
	pivots.push_back( start_node );

	update_novelty( m_env->getRAM(), start_node->nid );
	int num_simulated_steps = 0;

	m_expanded_nodes = 0;
	m_generated_nodes = 0;

	m_pruned_nodes = 0;

	do {

		std::cout << "# Pivots: " << pivots.size() << std::endl;
		std::cout << "First pivot reward: " << pivots.front()->node_reward << std::endl;
		pivots.front()->m_depth = 0;
		int steps = expand_node( pivots.front(), q, low_q );
		num_simulated_steps += steps;

		if (num_simulated_steps >= max_sim_steps_per_frame) {
			break;
		}

		pivots.pop_front();

		while(!q.empty() || !low_q.empty()) {
			// Pop a node to expand
			TreeNode* curr_node;
			if(!q.empty()) {
				curr_node = q.front();
				q.pop_front();
			} else {
				curr_node = low_q.front();
				low_q.pop_front();
			}

			if ( curr_node->depth() >= m_reward_horizon ) continue;
			if ( m_stop_on_first_reward && curr_node->node_reward != 0 )
			{
				pivots.push_back( curr_node );
				continue;
			}
			steps = expand_node( curr_node, q, low_q);
			num_simulated_steps += steps;
			// Stop once we have simulated a maximum number of steps
			if (num_simulated_steps >= max_sim_steps_per_frame) {
				break;
			}

		}
		std::cout << "\tExpanded so far: " << m_expanded_nodes << std::endl;
		std::cout << "\tPruned so far: " << m_pruned_nodes << std::endl;
		std::cout << "\tGenerated so far: " << m_generated_nodes << std::endl;

		if (q.empty()) { std::cout << "Search Space Exhausted!" << std::endl;
		}
		// Stop once we have simulated a maximum number of steps
		if (num_simulated_steps >= max_sim_steps_per_frame) {
			break;
		}

	} while ( !pivots.empty() );

	printf("Updating branch return\n");
	update_branch_return(start_node);

#ifdef OUTPUT_EXPLORE
	{
		ostringstream fname;
		fname << "exparr_" << setfill('0') << setw(3) << exparr_i << ".txt";
		ofstream exparr(fname.str());
		exparr << '[';
		for(int i=0; i<HEIGHT*N_HEIGHT; i++) {
			exparr << '[';
			for(int j=0; j<WIDTH*N_WIDTH; j++)
				exparr << expanded_arr[i*WIDTH*N_WIDTH + j] << ",";
			exparr << "],\n";
		}
		exparr << "]\n";
		exparr.close();
		exparr_i ++;
	}
#endif
}

void AbstractIWSearch::clear()
{
	SearchTree::clear();
	/* A different starting m_current_nid may be needed if EMPTY!=0, which is a
	 * constexpr */
	assert(EMPTY == 0);
	m_current_nid = 1;
}

void AbstractIWSearch::move_to_best_sub_branch()
{
	SearchTree::move_to_best_sub_branch();
	clear();
}

/* *********************************************************************
   Updates the branch reward for the given node
   which equals to: node_reward + max(children.branch_return)
   ******************************************************************* */
void AbstractIWSearch::update_branch_return(TreeNode* node) {
	// Base case (leaf node): the return is the immediate reward
	if (node->v_children.empty()) {
		node->branch_return = node->node_reward;
		node->best_branch = -1;
		node->branch_depth = node->m_depth;
		return;
	}

	// First, we have to make sure that all the children are updated
	for (unsigned int c = 0; c < node->v_children.size(); c++) {
		TreeNode* curr_child = node->v_children[c];

		if (ignore_duplicates && curr_child->is_duplicate()) continue;

		update_branch_return(curr_child);
	}

	// Now that all the children are updated, we can update the branch-reward
	float best_return = -1;
	int best_branch = -1;
	//return_t avg = 0;

	// Terminal nodes encur no reward beyond immediate
	if (node->is_terminal) {
		node->branch_depth = node->m_depth;
		best_return = node->node_reward;
		best_branch = 0;
	} else {

		for (size_t a = 0; a < node->v_children.size(); a++) {
			return_t child_return = node->v_children[a]->branch_return;
			if (best_branch == -1 || child_return > best_return) {
				best_return = child_return;
				best_branch = a;
				//avg+=child_return;
			} else if(child_return == best_return) {
				if(node->v_children[a]->branch_depth >
				   node->v_children[best_branch]->branch_depth )
					best_branch = a;
			}
			if( node->v_children[a]->branch_depth > node->branch_depth  )
				node->branch_depth = node->v_children[a]->branch_depth;

			//if( node->v_children.size() ) avg/=node->v_children.size();
		}
	}

	node->branch_return = node->node_reward + best_return * discount_factor;
	//node->branch_return = node->node_reward + avg * discount_factor;

	node->best_branch = best_branch;
}

void AbstractIWSearch::set_terminal_root(TreeNode * node) {
	node->branch_return = node->node_reward;

	if (node->v_children.empty()) {
		// Expand one child; add it to the node's children
		TreeNode* new_child = new TreeNode(node, node->state, m_current_nid++,
										   this, PLAYER_A_NOOP, frame_skip);

		node->v_children.push_back(new_child);
    	}

    	// Now we have at least one child, set the 'best branch' to the first action
    	node->best_branch = 0;
}

void	AbstractIWSearch::print_frame_data( int frame_number, float elapsed, Action curr_action, std::ostream& output )
{
	output << "frame=" << frame_number;
	output << ",expanded=" << expanded_nodes();
	output << ",generated=" << generated_nodes();
	output << ",pruned=" << pruned();
	output << ",depth_tree=" << max_depth();
	output << ",tree_size=" <<  num_nodes();
	output << ",best_action=" << action_to_string( curr_action );
	output << ",branch_reward=" << get_root_value();
	output << ",elapsed=" << elapsed;
	output << std::endl;
}
