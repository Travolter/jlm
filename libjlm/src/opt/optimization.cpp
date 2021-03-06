/*
 * Copyright 2018 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/ir/rvsdg-module.hpp>

#include <jlm/opt/cne.hpp>
#include <jlm/opt/dne.hpp>
#include <jlm/opt/inlining.hpp>
#include <jlm/opt/invariance.hpp>
#include <jlm/opt/inversion.hpp>
#include <jlm/opt/optimization.hpp>
#include <jlm/opt/pull.hpp>
#include <jlm/opt/push.hpp>
#include <jlm/opt/reduction.hpp>
#include <jlm/opt/unroll.hpp>

#include <jlm/util/stats.hpp>
#include <jlm/util/strfmt.hpp>
#include <jlm/util/time.hpp>

#include <unordered_map>

namespace jlm {

class optimization_stat final : public stat {
public:
	virtual
	~optimization_stat()
	{}

	optimization_stat(const jlm::filepath & filename)
	: filename_(filename)
	, nnodes_before_(0)
	, nnodes_after_(0)
	{}

	void
	start(const jive::graph & graph) noexcept
	{
		nnodes_before_ = jive::nnodes(graph.root());
		timer_.start();
	}

	void
	end(const jive::graph & graph) noexcept
	{
		timer_.stop();
		nnodes_after_ = jive::nnodes(graph.root());
	}

	virtual std::string
	to_str() const override
	{
		return strfmt("RVSDGOPTIMIZATION ", filename_.to_str(), " ",
			nnodes_before_, " ", nnodes_after_, " ", timer_.ns());
	}

private:
	jlm::timer timer_;
	jlm::filepath filename_;
	size_t nnodes_before_, nnodes_after_;
};

static void
unroll(rvsdg_module & rm, const stats_descriptor & sd)
{
	jlm::unroll(rm, sd, 4);
}

void
optimize(rvsdg_module & rm, const stats_descriptor & sd, const optimization & opt)
{
	static std::unordered_map<optimization, void(*)(rvsdg_module&, const stats_descriptor&)> map({
	  {optimization::cne, jlm::cne }
	, {optimization::dne, jlm::dne }
	, {optimization::iln, jlm::inlining }
	, {optimization::inv, jlm::invariance }
	, {optimization::pll, jlm::pull }
	, {optimization::psh, jlm::push }
	, {optimization::ivt, jlm::invert }
	, {optimization::url, unroll }
	, {optimization::red, jlm::reduce }
	});


	JLM_DEBUG_ASSERT(map.find(opt) != map.end());
	map[opt](rm, sd);
}

void
optimize(
	rvsdg_module & rm,
	const stats_descriptor & sd,
	const std::vector<optimization> & opts)
{
	optimization_stat stat(rm.source_filename());

	stat.start(*rm.graph());
	for (const auto & opt : opts)
		optimize(rm, sd, opt);
	stat.end(*rm.graph());

	if (sd.print_rvsdg_optimization)
		sd.print_stat(stat);
}

}
