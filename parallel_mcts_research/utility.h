#pragma once

#include "tweakme.h"

#if ENABLE_JSON
#define RAPIDJSON_HAS_STDSTRING 1
#include <rapidjson/document.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
using namespace rapidjson;
#endif

#include "node.h"
#include "mcts.h"

namespace mcts {

#if ENABLE_JSON
template <typename State, typename Move>
void SerializeChildren(Value & parent_node, 
	typename MCTS<State, Move>::node_ptr_type parent,
	Document& document) {
	Value children_node(rapidjson::kArrayType);
	for (const auto &children : parent->GetChildren()) {
		Value object(kObjectType);
		object.AddMember("visits", children->GetVisits(), document.GetAllocator());
        object.AddMember("value", children->GetUCB(), document.GetAllocator());
        object.AddMember("state", children->GetState().ToString(), document.GetAllocator());
        object.AddMember("name", children->GetLastMove().ToString(), document.GetAllocator());
		if (children->IsLeaf()) {
			SerializeChildren<State, Move>(object, children, document);
		}
		children_node.PushBack(object, document.GetAllocator());
	}
	parent_node.AddMember("children", children_node, document.GetAllocator());
}

template <typename State, typename Move>
void Serialize(const MCTS<State, Move>& mcts, Document& document) {
	Value parent_node(kObjectType);
	parent_node.AddMember("value", mcts.GetRoot()->GetUCB(), document.GetAllocator());
	parent_node.AddMember("name", mcts.GetRoot()->GetLastMove().ToString(), document.GetAllocator());
	parent_node.AddMember("state", mcts.GetRoot()->GetState().ToString(), document.GetAllocator());
	SerializeChildren<State, Move>(parent_node, mcts.GetRoot(), document);
	document.AddMember("mcts_result", parent_node, document.GetAllocator());
}

template <typename State, typename Move>
std::ostream& operator<<(std::ostream& stream, const MCTS<State, Move> & mcts) {
	Document document;
	document.SetObject();
	mcts.Serialize(document);
	OStreamWrapper wrapper{ stream };
	PrettyWriter<OStreamWrapper> writer{ wrapper };
	document.Accept(writer);
	return stream;
}
#endif

}