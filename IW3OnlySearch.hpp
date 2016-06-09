#ifndef __IW3_SEARCH_HPP__
#define __IW3_SEARCH_HPP__

#include "SearchTree.hpp"
#include "bit_matrix.hxx"
#include <environment/ale_ram.hpp>
#include <environment/stella_environment.hpp>
#include <map>

#include <queue>

class IW3OnlySearch : public SearchTree {
    public:
	IW3OnlySearch(Settings &settings, ActionVect &actions, StellaEnvironment* _env);

	virtual ~IW3OnlySearch();

	virtual void build(ALEState & state);
		
	virtual void update_tree();
	virtual int  expand_node( TreeNode* n, std::queue<TreeNode*>& q, std::queue<TreeNode*>& low_q); 

	int expanded() const { return m_expanded_nodes; }
	int generated() const { return m_generated_nodes; }
	int pruned() const { return m_pruned_nodes; }
	virtual unsigned max_depth(){ 	
		for (size_t c = 0; c < p_root->v_children.size(); c++)
			if(m_max_depth <  p_root->v_children[c]->branch_depth)
				m_max_depth =   p_root->v_children[c]->branch_depth;

			return m_max_depth;
	}
	virtual	void print_frame_data( int frame_number, float elapsed, Action curr_action, std::ostream& output );
public:	

	void print_path(TreeNode *start, int a);

	virtual void expand_tree(TreeNode* start);

	void update_branch_return(TreeNode* node);

    	void set_terminal_root(TreeNode* node); 

	void	update_novelty_table( const ALERAM &machine_state );
	bool	check_novelty_3( const ALERAM& machine_state );

	virtual void	clear();
	virtual void	move_to_best_sub_branch();
	
	ALERAM 			m_ram;
	aptk::Bit_Matrix*	m_pos_novelty_table;
	unsigned		m_pruned_nodes;
	bool			m_stop_on_first_reward;
	unsigned		m_reward_horizon;	
	std::map<unsigned, unsigned> m_positions_noop;
	unsigned m_max_noop_reopen;
};



#endif // __IW3_SEARCH_HPP__
