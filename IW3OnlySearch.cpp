#include "IW3OnlySearch.hpp"
#include "SearchTree.hpp"
#include <list>
#include <cassert>
#include <utility>
#include <cstdio>

using namespace std;

const int IW3OnlySearch::CIRCLE[][2] = {{-2, 0}, {-1, -1}, {-1, 0},
						 {-1, 1}, {0, -2}, {0, -1}, {0, 0}, {0, 1}, {0, 2}, {1, -1}, {1, 0},
						 {1, 1}, {2, 0}};

IW3OnlySearch::IW3OnlySearch(Settings &settings,
			       ActionVect &actions, StellaEnvironment* _env) :
	AbstractIWSearch(settings, actions, _env) {

	add_ancestor_node = NULL;
	m_max_noop_reopen = settings.getInt( "iw3_max_noop_reopen", 0 );
	m_noop_parent_depth = settings.getInt( "iw3_noop_parent_depth", 0 );
	m_prune_screens_prob = settings.getFloat( "iw3_prune_screens_prob", 0 );
	first_visited = true;

	m_randomize_noop = (m_noop_parent_depth > 0);
	// If we are doing wait-for-obstacles, make sure the first available action
	// is a NOOP.
	if(m_max_noop_reopen > 0 || m_noop_parent_depth > 0)
		assert(available_actions[0] == PLAYER_A_NOOP);
}

IW3OnlySearch::~IW3OnlySearch() {
}


void IW3OnlySearch::unset_novelty(const ALERAM& machine_state)
{
	auto s = machine_state.get(RAM_SCREEN);
	if(s >= N_SCREENS) return;
	NodeID nid = novelty_pos[s][machine_state.get(RAM_Y)][machine_state.get(RAM_X)];
	auto y = machine_state.get(RAM_Y);
	auto x = machine_state.get(RAM_X);
	for(size_t i=0; i<CIRCLE_LEN; i++) {
		auto yy = y+CIRCLE[i][0];
		auto xx = x+CIRCLE[i][1];
		if(novelty_pos[s][yy][xx] == nid)
			novelty_pos[s][yy][xx] = EMPTY;
	}
	novelty_z[machine_state.get(RAM_Z)-Z_BASE] = false;
}

void IW3OnlySearch::update_novelty(const ALERAM& machine_state, NodeID parent_nid)
{
	auto screen = machine_state.get(RAM_SCREEN);
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
	int y = machine_state.get(RAM_Y);
	int x = machine_state.get(RAM_X);
	for(size_t i=0; i<CIRCLE_LEN; i++) {
		int yy = y+CIRCLE[i][0];
		int xx = x+CIRCLE[i][1];
		if(novelty_pos[screen][yy][xx] == 0)
			novelty_pos[screen][yy][xx] = parent_nid;
	}
	novelty_z[machine_state.get(RAM_Z)-Z_BASE] = true;
}

bool IW3OnlySearch::check_novelty(const ALERAM& machine_state, NodeID parent_nid)
{
	auto screen = machine_state.get(RAM_SCREEN);
	auto y = machine_state.get(RAM_Y);
	auto x = machine_state.get(RAM_X);
	return (!pruned_screens[screen] &&
			(novelty_pos[screen][y][x] == EMPTY ||
			 novelty_pos[screen][y][x] == parent_nid)) ||
		!novelty_z[machine_state.get(RAM_Z)-Z_BASE];
}


void IW3OnlySearch::clear()
{
	AbstractIWSearch::clear();
	memset(novelty_pos, EMPTY, sizeof(novelty_pos));
	fill(novelty_z, novelty_z+sizeof(novelty_z)/sizeof(novelty_z[0]), false);
	fill(visited_screens, visited_screens+N_SCREENS, false);
	fill(pruned_screens, pruned_screens+N_SCREENS, false);
	first_visited = true;
}


void IW3OnlySearch::enqueue_node(TreeNode *curr_node, TreeNode *child,
								 TNDeque &q, TNDeque &low_q, Action act) {
/*  Uncomment for Montezuma
   auto current_lives = curr_node->getRAM().get(0xba);
	// if life is lost, put it in the lower priority deque
	if(child->getRAM().get(0xba) < current_lives) {
		// enable NOOP if this is walking and kills
		if(child->getRAM().get(0xd8) == 0x00 &&
			child->getRAM().get(0xd6) == 0xFF &&
			curr_node->getRAM().get(0xd8) == 0x00 &&
			curr_node->getRAM().get(0xd6) == 0xFF &&
			(act == PLAYER_A_DOWN || act == PLAYER_A_LEFT ||
			act == PLAYER_A_RIGHT || act == PLAYER_A_UP )) {
			add_ancestor_node = child;
		}
		low_q.push_back(child);
	} else {
		if(child->getRAM().get(0xd8) != 0 &&
			curr_node->getRAM().get(0xd8) == 0 &&
			curr_node->getRAM().get(0xd6) == 0xff &&
			(act == PLAYER_A_DOWN || act == PLAYER_A_LEFT ||
				act == PLAYER_A_RIGHT || act == PLAYER_A_UP )) {
			add_ancestor_node = child;
			// The agent may want to drop down from a platform or cord
			if(act == PLAYER_A_DOWN)
				q.push_back(child);
				} else {*/
			q.push_back(child);
//		}
//	}
}

int IW3OnlySearch::expanded_node_postprocessing(
	TreeNode *curr_node, TNDeque &q, TNDeque &low_q) {
	int num_simulated_steps = 0;
	auto current_lives = curr_node->getRAM().get(0xba);
	if(add_ancestor_node != NULL) {
		TreeNode *prev_ancestor = add_ancestor_node;
		TreeNode *ancestor = curr_node;
		for(unsigned i=0; i<m_noop_parent_depth && ancestor;
			i++, prev_ancestor = ancestor, ancestor = ancestor->p_parent) {
			if(ancestor->getRAM().get(0xd6) != 0xff ||
			   ancestor->getRAM().get(0xd8) != 0)
				continue;
			TreeNode *cousin = ancestor;
			cousin->is_terminal = false;
			for(unsigned j=i+1; j!=0; j--) {
				if(cousin->v_children.empty()) {
					cousin->v_children.push_back(
						new TreeNode(cousin, cousin->state, m_current_nid++,
									 this, PLAYER_A_NOOP, frame_skip));
					num_simulated_steps += cousin->num_simulated_steps;
					cousin->best_branch = 0;
					cousin->available_actions.push_back(PLAYER_A_NOOP);
				}
				cousin = cousin->v_children[0];
				cousin->is_terminal = false;
			}
			if(cousin->getRAM().get(0xba) == current_lives &&
			   cousin->getRAM().get(0xd6) == 0xff &&
			   cousin->getRAM().get(0xd8) == 0x00) {
				const Action a = prev_ancestor->act;
				for(unsigned reopen=0; reopen<m_max_noop_reopen; reopen++) {
					//printf(" %u", reopen);
					TreeNode one(cousin, cousin->state, m_current_nid++, this,
								 a, frame_skip);
					TreeNode two(&one, one.state, m_current_nid++, this,
								 a, frame_skip);
					num_simulated_steps += one.num_simulated_steps +
						two.num_simulated_steps;
					if(two.getRAM().get(0xba) == current_lives &&
					   two.getRAM().get(0xd6) == 0xff &&
					   two.getRAM().get(0xd8) == 0x00) {
						unset_novelty(one.getRAM());
						unset_novelty(two.getRAM());
						q.push_front(cousin);
						i = m_noop_parent_depth;
						break; // We found a safe spot
					} else {
						if(cousin->v_children.empty()) {
							cousin->v_children.push_back(
								new TreeNode(cousin, cousin->state,
											 m_current_nid++, this,
											 PLAYER_A_NOOP, frame_skip));
							num_simulated_steps += cousin->num_simulated_steps;
							cousin->best_branch = 0;
							cousin->available_actions.push_back(PLAYER_A_NOOP);
						}
						cousin = cousin->v_children[0];
						cousin->is_terminal = false;
					}
				}
			}
		}
	}
	add_ancestor_node = NULL;
	return num_simulated_steps;
}

