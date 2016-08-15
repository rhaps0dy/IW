#ifndef __IW3_SEARCH_HPP__
#define __IW3_SEARCH_HPP__

#include "AbstractIWSearch.hpp"
#include <random>

class IW3OnlySearch : public AbstractIWSearch {
public:
	IW3OnlySearch(Settings &settings, ActionVect &actions, StellaEnvironment* _env);
	~IW3OnlySearch();
	virtual void update_novelty( const ALERAM &machine_state );
	virtual bool check_novelty( const ALERAM& machine_state );
	virtual void clear();

protected:
	virtual void enqueue_node(TreeNode *curr_node, TreeNode *child, TNDeque &q,
							  TNDeque &low_q, Action act);
	virtual int expanded_node_postprocessing(
		TreeNode *curr_node, TNDeque &q, TNDeque &low_q);

	virtual void unset_novelty( const ALERAM& machine_state );

	TreeNode *add_ancestor_node;
	std::default_random_engine generator;

	bool visited_screens[N_SCREENS], pruned_screens[N_SCREENS];
	double m_prune_screens_prob;
	bool first_visited;

	aptk::Bit_Matrix*	m_pos_novelty_table;
	unsigned m_max_noop_reopen;
	unsigned m_noop_parent_depth;

};



#endif // __IW3_SEARCH_HPP__
