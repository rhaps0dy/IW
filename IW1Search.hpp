#ifndef __IW_SEARCH_HPP__
#define __IW_SEARCH_HPP__

#include "AbstractIWSearch.hpp"

class IW1Search : public AbstractIWSearch {
public:
	IW1Search(Settings &settings, ActionVect &actions, StellaEnvironment* _env);
	~IW1Search();
	virtual void update_novelty(const ALERAM &machine_state, NodeID parent_nid);
	virtual bool check_novelty(const ALERAM& machine_state, NodeID parent_nid);
	virtual void clear();

protected:
	virtual void enqueue_node(TreeNode *curr_node, TreeNode *child, TNDeque &q,
							  TNDeque &low_q, Action act);
	virtual int expanded_node_postprocessing(
		TreeNode *curr_node, TNDeque &q, TNDeque &low_q);

	aptk::Bit_Matrix* m_ram_novelty_table;
	aptk::Bit_Matrix* m_ram_novelty_table_true;
	aptk::Bit_Matrix* m_ram_novelty_table_false;
	unsigned m_pruned_nodes;
	bool m_stop_on_first_reward;
	unsigned m_reward_horizon;
	bool m_novelty_boolean_representation;

};

#endif // __IW_SEARCH_HPP__
