#include "IW3OnlySearch.hpp"
#include "SearchTree.hpp"
#include <list>
#include <cassert>
#include <utility>
#include <random>

using namespace std;

#ifdef OUTPUT_EXPLORE
constexpr int HEIGHT=128, WIDTH=160, N_HEIGHT=4, N_WIDTH=9;
static vector<int> expanded_arr(HEIGHT*N_HEIGHT*WIDTH*N_WIDTH, 0);
static int exparr_i = 0;
#endif

IW3OnlySearch::IW3OnlySearch(Settings &settings,
			       ActionVect &actions, StellaEnvironment* _env) :
	SearchTree(settings, actions, _env) {
	
	m_stop_on_first_reward = settings.getBool( "iw1_stop_on_first_reward", true );

	int val = settings.getInt( "iw1_reward_horizon", -1 );

	m_reward_horizon = ( val < 0 ? std::numeric_limits<unsigned>::max() : val ); 

	m_pos_novelty_table = new aptk::Bit_Matrix( 24, 256 * 256 );
	m_max_noop_reopen = settings.getInt( "iw3_max_noop_reopen", 0 );
	m_noop_parent_depth = settings.getInt( "iw3_noop_parent_depth", 0 );
	m_prune_screens_prob = settings.getFloat( "iw3_prune_screens_prob", 0 );
	first_visited = true;
}

IW3OnlySearch::~IW3OnlySearch() {
	delete m_pos_novelty_table;
}

/* *********************************************************************
   Builds a new tree
   ******************************************************************* */
void IW3OnlySearch::build(ALEState & state) {	
	assert(p_root == NULL);
	p_root = new TreeNode(NULL, state, NULL, UNDEFINED, 0);
	update_tree();
	is_built = true;	
}

void IW3OnlySearch::print_path(TreeNode * node, int a) {
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

void IW3OnlySearch::update_tree() {
	expand_tree(p_root);
}

void IW3OnlySearch::unset_novelty_table( const ALERAM& machine_state )
{
	auto s = machine_state.get(0x03);
	if(s >= 24) return;
	m_pos_novelty_table->unset(s, machine_state.get(0x2a)*256 + machine_state.get(0x2b) );
}

static default_random_engine generator;

void IW3OnlySearch::set_novelty_table( const ALERAM& machine_state )
{
	auto screen = machine_state.get(0x03);
	if(!visited_screens[screen]) {
		std::uniform_real_distribution<double> d;
		cout << "Visited screen " << (int)screen;
		if(!first_visited && d(generator) < m_prune_screens_prob) {
			pruned_screens[screen] = true;
			cout << " PRUNED";
		}
		visited_screens[screen] = true;
		cout << endl;
		first_visited = false;
	}
	m_pos_novelty_table->set(screen,
		machine_state.get(0x2a)*256 + machine_state.get(0x2b) );
}

bool IW3OnlySearch::check_novelty_3( const ALERAM& machine_state )
{
	auto screen = machine_state.get(0x03);
	if(pruned_screens[screen]) return false;
	if ( !m_pos_novelty_table->isset( screen,
			machine_state.get(0x2a)*256 + machine_state.get(0x2b) ))
		return true;
	return false;
}

int IW3OnlySearch::expand_node( TreeNode* curr_node, deque<TreeNode*>& q, deque<TreeNode*>& low_q)
{
	int num_simulated_steps =0;
	size_t num_actions = available_actions.size();
	size_t initial_a = curr_node->v_children.size();
	bool leaf_node = initial_a <= 1;
	if(!leaf_node)
		initial_a = 0;
	assert(frame_skip != 0);
	static     int max_nodes_per_frame = max_sim_steps_per_frame / frame_skip;
	m_expanded_nodes++;
	// Expand all of its children (simulates the result)
	if(leaf_node){
		curr_node->v_children.resize( num_actions );
		curr_node->available_actions = available_actions;
		if(m_randomize_successor)
		    std::random_shuffle ( curr_node->available_actions.begin()+1, curr_node->available_actions.end());
	
	}
#ifdef PRINT
	std::cout << "Expanding node (" <<
		(int)curr_node->getRAM().get(0xaa) << ", " <<
		(int)curr_node->getRAM().get(0xab) << ")\n";
#endif
#ifdef OUTPUT_EXPLORE
	int n=curr_node->getRAM().get(0x03);
	if(n < 24) {
		int xoffs, yoffs;
		if(n < 8) {
			if(n < 3) xoffs=3+n, yoffs=0;
			else xoffs=n-1, yoffs=1;
		} else {
			if(n < 15) xoffs=n-7, yoffs=2;
			else xoffs=n-15, yoffs=3;
		}
		expanded_arr[
			(HEIGHT*yoffs + HEIGHT-(curr_node->getRAM().get(0xab)-0x80))*N_WIDTH*WIDTH +
			(WIDTH *xoffs + curr_node->getRAM().get(0xaa))]++;
		if(!visited_screens[n]) {
			visited_screens[n] = true;
			std::cout << "Visited screen " << n << std::endl;
		}

//		std::cout << "Increasing (" << (HEIGHT*(n/4) + HEIGHT-curr_node->getRAM().get(0xab)-1) << ", " << (WIDTH *(n%4) + curr_node->getRAM().get(0xaa)) << ")\n";
//		std::cout << "n=" << n <<", y="<<(int)curr_node->getRAM().get(0xab)<<"\n";
	}
#endif
	bool add_ancestor_kill = false, add_ancestor_fall = false;

	auto current_lives = curr_node->getRAM().get(0xba);
	for (size_t a = initial_a; a < num_actions; a++) {
		Action act = curr_node->available_actions[a];
		
		TreeNode * child;
	
		// If re-expanding an internal node, don't creates new nodes
		if (leaf_node) {
			m_generated_nodes++;
			child = new TreeNode(	curr_node,	
						curr_node->state,
						this,
						act,
						frame_skip); 
			if (check_novelty_3(child->getRAM() ) ) {
				// child->is_terminal is already false
				child->is_terminal = false;
			}
			else{
				curr_node->v_children[a] = child;
				child->is_terminal = true;
				m_pruned_nodes++;
			}
			if (child->depth() > m_max_depth ) m_max_depth = child->depth();
			num_simulated_steps += child->num_simulated_steps;
					
			curr_node->v_children[a] = child;
		}
		else {
			child = curr_node->v_children[a];
		
			// This recreates the novelty table (which gets resetted every time
			// we change the root of the search tree)
			if ( m_novelty_pruning )
			{
			    if( child->is_terminal )
			    {
				if (check_novelty_3( child->getRAM() ) ){
				    child->is_terminal = false;
				}
				else{
				    child->is_terminal = true;
				    m_pruned_nodes++;
				}
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
			(int)child->getRAM().get(0xaa) << ", " <<
			(int)child->getRAM().get(0xab) << ") ";
#endif
		unsigned pos = (((unsigned)curr_node->getRAM().get(0x83)) * 256 +
				   ((unsigned)curr_node->getRAM().get(0xaa))) * 256 +
			((unsigned)curr_node->getRAM().get(0xaa));
		auto f = m_positions_noop.find(pos);
		// Don't expand duplicate nodes, or terminal nodes
		if(!child->is_terminal) {
			if((! (ignore_duplicates && test_duplicate(child)) ) &&
			   ( child->num_nodes_reusable < max_nodes_per_frame )) {
				// if life is lost, put it in the lower priority deque
				if(child->getRAM().get(0xba) < current_lives) {
					// enable NOOP if this is walking and kills
					unsigned n=0;
					if((f == m_positions_noop.end() || (n=f->second) < m_max_noop_reopen) &&
					   child->getRAM().get(0xd8) == 0x00 &&
					   child->getRAM().get(0xd6) == 0xFF &&
					   (act == PLAYER_A_DOWN || act == PLAYER_A_LEFT ||
						act == PLAYER_A_RIGHT || act == PLAYER_A_UP )) {
						if(!add_ancestor_kill)
							m_positions_noop[pos] = n + 1;
						add_ancestor_kill = true;
					}
					low_q.push_back(child);
				} else {
					unsigned n=0;
					if((f == m_positions_noop.end() || (n=f->second) < m_max_noop_reopen) &&
					   child->getRAM().get(0xd8) != 0 &&
					   curr_node->getRAM().get(0xd8) == 0 &&
					   curr_node->getRAM().get(0xd6) == 0xff &&
					   (act == PLAYER_A_DOWN || act == PLAYER_A_LEFT ||
						act == PLAYER_A_RIGHT || act == PLAYER_A_UP )) {
						if(!add_ancestor_fall)
							m_positions_noop[pos] = n + 1;
						add_ancestor_fall = true;
					} else {
						set_novelty_table( child->getRAM() );
					}
					q.push_back(child);
				}
			} else {
				set_novelty_table( child->getRAM() );
			}
#ifdef PRINT
			std::cout << "EXPANDED\n";
		} else {
			std::cout << "PRUNED\n";
#endif
		}
	}
	if(add_ancestor_kill || add_ancestor_fall) {
		TreeNode *ancestor = curr_node;
		if(add_ancestor_kill)
			std::cout << "Going back " << action_to_string(curr_node->act);
		for(unsigned i=0; i<m_noop_parent_depth && ancestor; i++) {
			if(add_ancestor_kill && ancestor->act != curr_node->act) {
				std::cout << " Failed! " << action_to_string(ancestor->act);
				continue;
			}
			TreeNode *cousin = ancestor;
			for(unsigned j=i+1; j!=0; j--) {
				if(cousin->v_children.empty()) {
					cousin->v_children.push_back(new TreeNode(cousin,
															  cousin->state, this, PLAYER_A_NOOP, frame_skip));
					num_simulated_steps += cousin->num_simulated_steps;
					cousin->best_branch = 0;
					cousin->available_actions.push_back(PLAYER_A_NOOP);
				}
				cousin = cousin->v_children[0];
			}
			unset_novelty_table(ancestor->getRAM());
			if((add_ancestor_kill && cousin->getRAM().get(0xba) == current_lives) ||
			   (add_ancestor_fall && cousin->getRAM().get(0xd6) == 0xff && cousin->getRAM().get(0xd8) == 0x00)) {
					unset_novelty_table(cousin->getRAM());
					q.push_front(cousin);
//					cout << "Backed up " << i << " positions\n";
					break; // We found a safe spot
				}
				ancestor = ancestor->p_parent;
			}
	}
	return num_simulated_steps;
}
/* *********************************************************************
   Expands the tree from the given node until i_max_sim_steps_per_frame
   is reached
	
   ******************************************************************* */
void IW3OnlySearch::expand_tree(TreeNode* start_node) {

	if(!start_node->v_children.empty()){
	    start_node->updateTreeNode();
	    for (int a = 0; a < available_actions.size(); a++) {
			TreeNode* child = start_node->v_children[a];
			if( !child->is_terminal ){
		        child->num_nodes_reusable = child->num_nodes();
		}
	    }
	}
	fill(visited_screens, visited_screens+N_SCREENS, false);
	fill(pruned_screens, pruned_screens+N_SCREENS, false);
	first_visited = true;
#ifdef OUTPUT_EXPLORE
	fill(expanded_arr.begin(), expanded_arr.end(), 0);
#endif // OUTPUT_EXPLORE


	// low_q contains nodes that lose a life
	deque<TreeNode*> q, low_q;
	std::list< TreeNode* > pivots;
	
	//q.push(start_node);
	pivots.push_back( start_node );

//	set_novelty_table( start_node->state.getRAM() );
	set_novelty_table( m_env->getRAM() );
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
	
			if ( curr_node->depth() > m_reward_horizon - 1 ) continue;
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
#ifdef OUTPUT_EXPLORE
	if(q.empty()) {
	ostringstream fname;
	fname << "empty_exparr_" << exparr_i << ".txt";
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

	update_branch_return(start_node);
}

void IW3OnlySearch::clear()
{
	SearchTree::clear();

	m_pos_novelty_table->clear();
	m_positions_noop.clear();
}

void IW3OnlySearch::move_to_best_sub_branch() 
{
	SearchTree::move_to_best_sub_branch();
	m_pos_novelty_table->clear();
}

/* *********************************************************************
   Updates the branch reward for the given node
   which equals to: node_reward + max(children.branch_return)
   ******************************************************************* */
void IW3OnlySearch::update_branch_return(TreeNode* node) {
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
			if (best_branch == -1 || child_return >= best_return) {
				best_return = child_return;
				best_branch = a;
				//avg+=child_return;
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

void IW3OnlySearch::set_terminal_root(TreeNode * node) {
	node->branch_return = node->node_reward; 

	if (node->v_children.empty()) {
		// Expand one child; add it to the node's children
		TreeNode* new_child = new TreeNode(	node, node->state, 
							this, PLAYER_A_NOOP, frame_skip);

		node->v_children.push_back(new_child);
    	}
  
    	// Now we have at least one child, set the 'best branch' to the first action
    	node->best_branch = 0; 
}

void	IW3OnlySearch::print_frame_data( int frame_number, float elapsed, Action curr_action, std::ostream& output )
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
