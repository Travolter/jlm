/*
 * Copyright 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2013 2014 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/common.hpp>
#include <jlm/ir/basic_block.hpp>
#include <jlm/ir/callgraph.hpp>
#include <jlm/ir/cfg.hpp>
#include <jlm/ir/cfg_node.hpp>

#include <string.h>

#include <algorithm>

namespace jlm {

/* attribute */

attribute::~attribute()
{}

/* edge */

cfg_edge::cfg_edge(cfg_node * source, cfg_node * sink, size_t index) noexcept
	: source_(source)
	, sink_(sink)
	, index_(index)
{}

void
cfg_edge::divert(cfg_node * new_sink)
{
	if (sink_ == new_sink)
		return;

	sink_->inedges_.erase(this);
	sink_ = new_sink;
	new_sink->inedges_.insert(this);
}

cfg_node *
cfg_edge::split()
{
	auto sink = sink_;
	auto bb = create_basic_block_node(source_->cfg());
	divert(bb);
	bb->add_outedge(sink);
	return bb;
}

/* node */

size_t
cfg_node::noutedges() const noexcept
{
	return outedges_.size();
}

void
cfg_node::remove_inedges()
{
	while (inedges_.size() != 0) {
		cfg_edge * edge = *inedges_.begin();
		JLM_DEBUG_ASSERT(edge->sink() == this);
		edge->source()->remove_outedge(edge->index());
	}
}

size_t
cfg_node::ninedges() const noexcept
{
	return inedges_.size();
}

bool
cfg_node::no_predecessor() const noexcept
{
	return ninedges() == 0;
}

bool
cfg_node::single_predecessor() const noexcept
{
	if (ninedges() == 0)
		return false;

	for (auto i = inedges_.begin(); i != inedges_.end(); i++) {
		JLM_DEBUG_ASSERT((*i)->sink() == this);
		if ((*i)->source() != (*inedges_.begin())->source())
			return false;
	}

	return true;
}

bool
cfg_node::no_successor() const noexcept
{
	return noutedges() == 0;
}

bool
cfg_node::single_successor() const noexcept
{
	if (noutedges() == 0)
		return false;

	for (auto it = begin_outedges(); it != end_outedges(); it++) {
		JLM_DEBUG_ASSERT(it->source() == this);
		if (it->sink() != begin_outedges()->sink())
			return false;
	}

	return true;
}

bool
cfg_node::has_selfloop_edge() const noexcept
{
	for (auto it = begin_outedges(); it != end_outedges(); it++) {
		if (it->is_selfloop())
			return true;
	}

	return false;
}

cfg_node *
cfg_node::create(jlm::cfg & cfg, std::unique_ptr<jlm::attribute> attribute)
{
	std::unique_ptr<jlm::cfg_node> node(new cfg_node(cfg, std::move(attribute)));
	auto tmp = node.get();
	cfg.add_node(std::move(node));
	return tmp;
}

}