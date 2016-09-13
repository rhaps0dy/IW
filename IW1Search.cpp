#include "IW1Search.hpp"
#include "SearchTree.hpp"
#include <list>
#include <cassert>

using namespace std;

IW1Search::IW1Search(Settings &settings,
			       ActionVect &actions, StellaEnvironment* _env) :
	AbstractIWSearch(settings, actions, _env) {

	m_novelty_boolean_representation = settings.getBool("novelty_boolean", false);
	if(m_novelty_boolean_representation) {
		m_ram_novelty_table_true = new aptk::Bit_Matrix(RAM_SIZE, 8);
		m_ram_novelty_table_false = new aptk::Bit_Matrix(RAM_SIZE, 8);
	} else {
		m_ram_novelty_table = new aptk::Bit_Matrix(RAM_SIZE, 256);
	}
}

IW1Search::~IW1Search() {
	if(m_novelty_boolean_representation){
		delete m_ram_novelty_table_true;
		delete m_ram_novelty_table_false;
	}
	else {
		delete m_ram_novelty_table;
	}
}

void IW1Search::update_novelty(const ALERAM& machine_state, NodeID parent_nid)
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
		}
}

bool IW1Search::check_novelty(const ALERAM& machine_state, NodeID parent_nid)
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
		}
	return false;
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
	}
}


void IW1Search::enqueue_node(TreeNode *curr_node, TreeNode *child,
							 TNDeque &q, TNDeque &low_q, Action act) {

	auto current_lives = curr_node->getRAM().get(0xba);
	if(child->getRAM().get(0xba) < current_lives) {
		low_q.push_back(child);
	} else {
		q.push_back(child);
	}
}

int IW1Search::expanded_node_postprocessing(
	TreeNode *curr_node, TNDeque &q, TNDeque &low_q) {
	return 0;
}
