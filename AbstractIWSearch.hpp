#ifndef __ABSTRACT_IW_SEARCH_HPP__
#define __ABSTRACT_IW_SEARCH_HPP__

#include "SearchTree.hpp"
#include "bit_matrix.hxx"
#include <environment/ale_ram.hpp>
#include <environment/stella_environment.hpp>
#include <deque>
#include <vector>

typedef std::deque<TreeNode*> TNDeque;

class AbstractIWSearch : public SearchTree {
public:
	AbstractIWSearch(Settings &settings, ActionVect &actions,
					 StellaEnvironment* _env);
	void build(ALEState & state);
	void update_tree();

	int expanded() const { return m_expanded_nodes; }
	int generated() const { return m_generated_nodes; }
	int pruned() const { return m_pruned_nodes; }
	unsigned max_depth(){
		for (size_t c = 0; c < p_root->v_children.size(); c++)
			if(m_max_depth <  p_root->v_children[c]->branch_depth)
				m_max_depth =   p_root->v_children[c]->branch_depth;
			return m_max_depth;
	}

	void print_frame_data(int frame_number, float elapsed, Action curr_action,
						  std::ostream& output );
	void print_path(TreeNode *start, int a);

	void expand_tree(TreeNode* start);
	void update_branch_return(TreeNode* node);
    void set_terminal_root(TreeNode* node);

	virtual void update_novelty(const ALERAM &machine_state, NodeID parent_nid) = 0;
	virtual bool check_novelty(const ALERAM &machine_state, NodeID parent_nid) = 0;
	virtual void clear();

	void move_to_best_sub_branch();

protected:
	virtual int expand_node(TreeNode* n, TNDeque& q, TNDeque& low_q);
	virtual void enqueue_node(TreeNode *curr_node, TreeNode *child, TNDeque &q,
							  TNDeque &low_q, Action act) = 0;
	virtual int expanded_node_postprocessing(
		TreeNode *curr_node, TNDeque &q, TNDeque &low_q) = 0;

	/* Montezuma's Revenge
	static constexpr int HEIGHT=128, WIDTH=160, N_HEIGHT=4, N_WIDTH=9;
	static constexpr size_t N_SCREENS=24;
	static constexpr int RAM_X=0x2a, RAM_Y=0x2b, RAM_SCREEN=0x03;
	*/
	/* Private Eye */
	static constexpr int HEIGHT=0x63, WIDTH=0x92, N_HEIGHT=4, N_WIDTH=8;
	static constexpr size_t N_SCREENS=0x20;
	static constexpr int RAM_X=0x3f, RAM_Y=0x61, RAM_SCREEN=0x5c;

	std::vector<int> expanded_arr;
	int exparr_i = 0;

	bool m_randomize_noop;
	unsigned m_pruned_nodes;
	bool m_stop_on_first_reward;
	unsigned m_reward_horizon;
	NodeID m_current_nid;
};


#endif // __ABSTRACT_IW_SEARCH_HPP__
