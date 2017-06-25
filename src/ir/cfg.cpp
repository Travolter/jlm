/*
 * Copyright 2014 Helge Bahmann <hcb@chaoticmind.net>
 * Copyright 2013 2014 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/common.hpp>
#include <jlm/ir/basic_block.hpp>
#include <jlm/ir/cfg.hpp>
#include <jlm/ir/cfg-structure.hpp>
#include <jlm/ir/cfg_node.hpp>
#include <jlm/ir/clg.hpp>
#include <jlm/ir/operators.hpp>
#include <jlm/ir/tac.hpp>

#include <algorithm>
#include <deque>
#include <sstream>
#include <unordered_map>

#include <stdio.h>
#include <stdlib.h>
#include <sstream>

namespace jlm {

/* entry attribute */

static inline jlm::cfg_node *
create_entry_node(jlm::cfg * cfg)
{
	jlm::entry_attribute attr;
	return cfg_node::create(*cfg, attr);
}

entry_attribute::~entry_attribute()
{}

std::string
entry_attribute::debug_string() const noexcept
{
	return "ENTRY";
}

std::unique_ptr<attribute>
entry_attribute::copy() const
{
	return std::unique_ptr<attribute>(new entry_attribute(*this));
}

/* exit attribute */

static inline jlm::cfg_node *
create_exit_node(jlm::cfg * cfg)
{
	jlm::exit_attribute attr;
	return cfg_node::create(*cfg, attr);
}

exit_attribute::~exit_attribute()
{}

std::string
exit_attribute::debug_string() const noexcept
{
	return "EXIT";
}

std::unique_ptr<attribute>
exit_attribute::copy() const
{
	return std::unique_ptr<attribute>(new exit_attribute(*this));
}

/* cfg */

cfg::cfg(jlm::module & module)
: module_(module)
{
	entry_ = create_entry_node(this);
	exit_ = create_exit_node(this);
	entry_->add_outedge(exit_);
}

std::string
cfg::convert_to_dot() const
{
	std::string dot("digraph cfg{\n");
	for (const auto & node : nodes_) {
		dot += strfmt((intptr_t)node.get());
		dot += strfmt("[shape = box, label = \"", node->debug_string(), "\"];\n");
		for (auto it = node->begin_outedges(); it != node->end_outedges(); it++) {
			dot += strfmt((intptr_t)it->source(), " -> ", (intptr_t)it->sink());
			dot += strfmt("[label = \"", it->index(), "\"];\n");
		}
	}
	dot += "}\n";

	return dot;
}

cfg::iterator
cfg::remove_node(cfg::iterator & it)
{
	if (it->cfg() != this)
		throw std::logic_error("Node does not belong to this CFG.");

	if (it->ninedges())
		throw std::logic_error("Cannot remove node. It has still incoming edges.");

	it->remove_outedges();
	std::unique_ptr<jlm::cfg_node> tmp(it.node());
	auto rit = iterator(std::next(nodes_.find(tmp)));
	nodes_.erase(tmp);
	tmp.release();
	return rit;
}

}

void
jive_cfg_view(const jlm::cfg & cfg)
{
	FILE * file = popen("tee /tmp/cfg.dot | dot -Tps > /tmp/cfg.ps ; gv /tmp/cfg.ps", "w");
	auto dot = cfg.convert_to_dot();
	fwrite(dot.c_str(), dot.size(), 1, file);
	pclose(file);
}
