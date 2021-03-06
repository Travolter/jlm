/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_RVSDG2JLM_RVSDG2JLM_HPP
#define JLM_RVSDG2JLM_RVSDG2JLM_HPP

#include <memory>

namespace jive {

class graph;

}

namespace jlm {

class ipgraph_module;
class rvsdg_module;
class stats_descriptor;

namespace rvsdg2jlm {

std::unique_ptr<ipgraph_module>
rvsdg2jlm(const rvsdg_module & rm, const stats_descriptor & sd);

}}

#endif
