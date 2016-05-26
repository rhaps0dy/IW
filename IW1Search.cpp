#include "IW1Search.hpp"
#include "SearchTree.hpp"
#include <list>
#include <cassert>

using namespace std;

#ifdef OUTPUT_EXPLORE
constexpr int HEIGHT=128, WIDTH=160, N_HEIGHT=12, N_WIDTH=4;
static int expanded_arr[HEIGHT*N_HEIGHT][WIDTH*N_WIDTH] = {0};
#endif

IW1Search::IW1Search(Settings &settings,
			       ActionVect &actions, StellaEnvironment* _env) :
	SearchTree(settings, actions, _env) {
	
	m_stop_on_first_reward = settings.getBool( "iw1_stop_on_first_reward", true );

	int val = settings.getInt( "iw1_reward_horizon", -1 );

	m_novelty_boolean_representation = settings.getBool( "novelty_boolean", false );

	m_reward_horizon = ( val < 0 ? std::numeric_limits<unsigned>::max() : val ); 

	if( m_novelty_boolean_representation){
		m_ram_novelty_table_true = new aptk::Bit_Matrix( RAM_SIZE, 8 );
		m_ram_novelty_table_false = new aptk::Bit_Matrix( RAM_SIZE, 8 );		
	}
	else {
		m_ram_novelty_table = new aptk::Bit_Matrix( RAM_SIZE, 256 );
		m_pos_novelty_table = new aptk::Bit_Matrix( 0x20, 256 * 256 );
	}
}

IW1Search::~IW1Search() {
	if(m_novelty_boolean_representation){
		delete m_ram_novelty_table_true;
		delete m_ram_novelty_table_false;
	}
	else {
		delete m_ram_novelty_table;
		delete m_pos_novelty_table;
	}
}

/* *********************************************************************
   Builds a new tree
   ******************************************************************* */
void IW1Search::build(ALEState & state) {	
	assert(p_root == NULL);
	p_root = new TreeNode(NULL, state, NULL, UNDEFINED, 0);
	update_tree();
	is_built = true;	
}

void IW1Search::print_path(TreeNode * node, int a) {
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

void IW1Search::update_tree() {
	expand_tree(p_root);
}

void IW1Search::update_novelty_table( const ALERAM& machine_state )
{
	for ( size_t i = 0; i < machine_state.size(); i++ )
		if( m_novelty_boolean_representation ){
			unsigned char mask = 1;
			byte_t byte =  machine_state.get(i);
			for(int j = 0; j < 8; j++) {
				bool bit_is_set = (byte & (mask << j)) != 0;
				if( bit_is_set )
					m_ram_novelty_table_true->set( i, j );
				else
					m_ram_novelty_table_false->set( i, j );
			}
		}
		else {
			m_ram_novelty_table->set( i, machine_state.get(i) );
			m_pos_novelty_table->set( machine_state.get(0x03),
				machine_state.get(0x2a)*256 + machine_state.get(0x2b) );
		}
}

bool IW1Search::check_novelty_1( const ALERAM& machine_state )
{
	for ( size_t i = 0; i < machine_state.size(); i++ )	
		if( m_novelty_boolean_representation ){
			unsigned char mask = 1;
			byte_t byte =  machine_state.get(i);
			for(int j = 0; j < 8; j++) {
				bool bit_is_set = (byte & (mask << j)) != 0;
				if( bit_is_set ){
					if( ! m_ram_novelty_table_true->isset( i, j ) )
						return true;
				}
				else{
					if( ! m_ram_novelty_table_false->isset( i, j ) )
						return true;

				}
			}
		}
		else {
			if ( !m_ram_novelty_table->isset( i, machine_state.get(i) ) )
				return true;
			if ( !m_pos_novelty_table->isset( machine_state.get(0x03),
					machine_state.get(0x2a)*256 + machine_state.get(0x2b) ))
				return true;
		}
	return false;
}

int IW1Search::expand_node( TreeNode* curr_node, queue<TreeNode*>& q, queue<TreeNode*>& low_q)
{
	int num_simulated_steps =0;
	int num_actions = available_actions.size();
	bool leaf_node = (curr_node->v_children.empty());
	assert(frame_skip != 0);
	static     int max_nodes_per_frame = max_sim_steps_per_frame / frame_skip;
	m_expanded_nodes++;
	// Expand all of its children (simulates the result)
	if(leaf_node){
		curr_node->v_children.resize( num_actions );
		curr_node->available_actions = available_actions;
		if(m_randomize_successor)
		    std::random_shuffle ( curr_node->available_actions.begin(), curr_node->available_actions.end() );
	
	}
#ifdef PRINT
	std::cout << "Expanding node (" <<
		(int)curr_node->getRAM().get(0xaa) << ", " <<
		(int)curr_node->getRAM().get(0xab) << ")\n";
#endif
#ifdef OUTPUT_EXPLORE
	int n=curr_node->getRAM().get(0x03);
	expanded_arr
		[HEIGHT*(n/4) + HEIGHT-curr_node->getRAM().get(0xab)]
		[WIDTH *(n%4) + curr_node->getRAM().get(0xaa)]++;
#endif
	for (int a = 0; a < num_actions; a++) {
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
//			if ( check_novelty_1( egetRAM() ) ) {
//				update_novelty_table( child->state.getRAM() );
			if ( check_novelty_1(child->getRAM() ) ) {
				update_novelty_table( child->getRAM() );
			}
			else{
				curr_node->v_children[a] = child;
				child->is_terminal = true;
				m_pruned_nodes++;
				//continue;				
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
//				if ( check_novelty_1( child->state.getRAM() ) ){
//				    update_novelty_table( child->state.getRAM() );
				if ( check_novelty_1( child->getRAM() ) ){
				    update_novelty_table( child->getRAM() );
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
		// Don't expand duplicate nodes, or terminal nodes
		if((!child->is_terminal) &&
		   (! (ignore_duplicates && test_duplicate(child)) ) &&
		   ( child->num_nodes_reusable < max_nodes_per_frame )) {
			// if life is lost, put it in the lower priority queue
			if(child->getRAM().get(0xba) < curr_node->getRAM().get(0xba)) {
				low_q.push(child);
			} else {
				q.push(child);
			}
#ifdef PRINT
			std::cout << "EXPANDED\n";
		} else {
			std::cout << "PRUNED\n";
#endif
		}
	}
	return num_simulated_steps;
}

/* *********************************************************************
   Expands the tree from the given node until i_max_sim_steps_per_frame
   is reached
	
   ******************************************************************* */
void IW1Search::expand_tree(TreeNode* start_node) {

	if(!start_node->v_children.empty()){
	    start_node->updateTreeNode();
	    for (int a = 0; a < available_actions.size(); a++) {
		TreeNode* child = start_node->v_children[a];
		if( !child->is_terminal ){
		        child->num_nodes_reusable = child->num_nodes();
		}
	    }
	}

	// low_q contains nodes that lose a life
	queue<TreeNode*> q, low_q;
	std::list< TreeNode* > pivots;
	
	//q.push(start_node);
	pivots.push_back( start_node );

//	update_novelty_table( start_node->state.getRAM() );
	update_novelty_table( m_env->getRAM() );
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
				q.pop();
			} else {
				curr_node = low_q.front();
				low_q.pop();
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
	ofstream exparr("exparr.txt");
	exparr << '[';
	for(int i=0; i<HEIGHT*N_HEIGHT; i++) {
		exparr << '[';
		for(int j=0; j<WIDTH*N_WIDTH; j++)
			exparr << expanded_arr[i][j] << ",";
		exparr << "],\n";
	}
	exparr << "]\n";
	exparr.close();
	assert(false);
#endif

	update_branch_return(start_node);
}

void IW1Search::clear()
{
	SearchTree::clear();

	if(m_novelty_boolean_representation){
		m_ram_novelty_table_true->clear();
		m_ram_novelty_table_false->clear();
	}
	else {
		m_ram_novelty_table->clear();
		m_pos_novelty_table->clear();
	}
}

void IW1Search::move_to_best_sub_branch() 
{
	SearchTree::move_to_best_sub_branch();
	if(m_novelty_boolean_representation){
		m_ram_novelty_table_true->clear();
		m_ram_novelty_table_false->clear();
	}
	else {
		m_ram_novelty_table->clear();
		m_pos_novelty_table->clear();
	}
}

/* *********************************************************************
   Updates the branch reward for the given node
   which equals to: node_reward + max(children.branch_return)
   ******************************************************************* */
void IW1Search::update_branch_return(TreeNode* node) {
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
	return_t avg = 0;

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
				avg+=child_return;
			}
			if( node->v_children[a]->branch_depth > node->branch_depth  ) 
				node->branch_depth = node->v_children[a]->branch_depth;

			if( node->v_children.size() ) avg/=node->v_children.size();
		}
	}

	node->branch_return = node->node_reward + best_return * discount_factor; 
	//node->branch_return = node->node_reward + avg * discount_factor; 
	
	node->best_branch = best_branch;
}

void IW1Search::set_terminal_root(TreeNode * node) {
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

void	IW1Search::print_frame_data( int frame_number, float elapsed, Action curr_action, std::ostream& output )
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
