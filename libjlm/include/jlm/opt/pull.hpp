/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_OPT_PULL_HPP
#define JLM_OPT_PULL_HPP

#include <jive/rvsdg/graph.h>

namespace jive {
	class gamma_node;
	class region;
}

namespace jlm {

class rvsdg_module;
class stats_descriptor;

void
pullin_top(jive::gamma_node * gamma);

void
pullin_bottom(jive::gamma_node * gamma);


void
pull(jive::gamma_node * gamma);

void
pull(jive::region * region);

void
pull(rvsdg_module & rm, const stats_descriptor & sd);

}

#endif
