#ifndef __IW3_SEARCH_HPP__
#define __IW3_SEARCH_HPP__

#include "AbstractIWSearch.hpp"
#include <random>

class IW3OnlySearch : public AbstractIWSearch {
public:
	IW3OnlySearch(Settings &settings, ActionVect &actions, StellaEnvironment* _env);
	~IW3OnlySearch();
	virtual void update_novelty(const ALERAM &machine_state, NodeID parent_nid);
	virtual bool check_novelty(const ALERAM& machine_state, NodeID parent_nid);
	virtual void clear();

protected:
	virtual void enqueue_node(TreeNode *curr_node, TreeNode *child, TNDeque &q,
							  TNDeque &low_q, Action act);
	virtual int expanded_node_postprocessing(
		TreeNode *curr_node, TNDeque &q, TNDeque &low_q);

	virtual void unset_novelty(const ALERAM& machine_state);

	TreeNode *add_ancestor_node;
	std::default_random_engine generator;

	bool visited_screens[N_SCREENS], pruned_screens[N_SCREENS];
	double m_prune_screens_prob;
	bool first_visited;

	NodeID novelty_pos[N_SCREENS][256][256];
	constexpr static int Z_BASE=0x26;
	bool novelty_z[0x37 - Z_BASE + 1];
	// List of offset positions that form a circle, to mark having been in that position
	constexpr static size_t CIRCLE_LEN = 13;
	const static int CIRCLE[CIRCLE_LEN][2];
	constexpr static size_t CIRCLE_RADIUS = 2;
	constexpr static NodeID EMPTY = 0;

	unsigned m_max_noop_reopen;
	unsigned m_noop_parent_depth;

};

#endif // __IW3_SEARCH_HPP__
