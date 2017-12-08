/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jive/arch/addresstype.h>
#include <jive/types/bitstring/arithmetic.h>
#include <jive/types/bitstring/comparison.h>
#include <jive/types/bitstring/constant.h>
#include <jive/rvsdg/binary.h>
#include <jive/rvsdg/structural-node.h>
#include <jive/rvsdg/substitution.h>
#include <jive/rvsdg/theta.h>
#include <jive/rvsdg/traverser.h>

#include <jlm/common.hpp>
#include <jlm/opt/unroll.hpp>

namespace jlm {

static bool
contains_theta(const jive::region * region)
{
	for (const auto & node : *region) {
		if (jive::is_theta_node(&node))
			return true;

		if (auto structnode = dynamic_cast<const jive::structural_node*>(&node)) {
			for (size_t r = 0; r < structnode->nsubregions(); r++)
				if (contains_theta(structnode->subregion(r)))
					return true;
		}
	}

	return false;
}

typedef struct unrollinfo {
	inline
	unrollinfo(
		bool eqop_,
		size_t nbits_,
		jive::argument * min_,
		jive::argument * max_)
	: eqop(eqop_)
	, nbits(nbits_)
	, min(min_)
	, max(max_)
	{}

	bool eqop;
	size_t nbits;
	jive::argument * min;
	jive::argument * max;
} unrollinfo;

static bool
is_greaterop(const jive::operation & op)
{
	return dynamic_cast<const jive::bits::uge_op*>(&op)
	    || dynamic_cast<const jive::bits::ugt_op*>(&op)
	    || dynamic_cast<const jive::bits::sge_op*>(&op)
	    || dynamic_cast<const jive::bits::sgt_op*>(&op);
}

static bool
is_eqcmp(const jive::operation & op)
{
	return dynamic_cast<const jive::bits::uge_op*>(&op)
	    || dynamic_cast<const jive::bits::sge_op*>(&op)
	    || dynamic_cast<const jive::bits::ule_op*>(&op)
	    || dynamic_cast<const jive::bits::sle_op*>(&op);
}

static bool
is_invariant(const jive::argument * argument)
{
	JLM_DEBUG_ASSERT(is_theta_node(argument->region()->node()));
	auto node = argument->input()->node();
	auto result = node->output(argument->input()->index())->results.first;

	return argument == result->origin();
}

static std::unique_ptr<struct unrollinfo>
is_applicable(const jive::theta_node * theta)
{
	if (contains_theta(theta->subregion()))
		return nullptr;

	auto matchnode = theta->predicate()->origin()->node();
	if (!is_match_node(matchnode))
		return nullptr;

	auto cmpnode = matchnode->input(0)->origin()->node();
	if (!jive::is_opnode<jive::bits::compare_op>(cmpnode))
		return nullptr;

	auto max = is_greaterop(cmpnode->operation()) ? cmpnode->input(0) : cmpnode->input(1);
	auto min = is_greaterop(cmpnode->operation()) ? cmpnode->input(1) : cmpnode->input(0);

	auto maxargument = dynamic_cast<jive::argument*>(max->origin());
	if (!maxargument || !is_invariant(maxargument))
		return nullptr;

	auto node = min->origin()->node();
	if (!jive::bits::is_add_node(node) || node->ninputs() != 2)
		return nullptr;

	auto origin0 = node->input(0)->origin();
	auto origin1 = node->input(1)->origin();
	auto minorigin = jive::producer(origin0) ? origin1 : origin0;
	auto minargument = dynamic_cast<jive::argument*>(minorigin);
	if (!minargument) return nullptr;

	auto eqop = is_eqcmp(cmpnode->operation());
	auto nbits = static_cast<const jive::bits::compare_op*>(&cmpnode->operation())->type().nbits();
	return std::make_unique<struct unrollinfo>(eqop, nbits, minargument, maxargument);
}

static void
unroll(jive::theta_node * theta, size_t factor)
{
	JLM_DEBUG_ASSERT(factor >= 2);

	auto ti = is_applicable(theta);
	if (!ti) return;

	auto minorigin = ti->min->input()->origin();
	auto maxorigin = ti->max->input()->origin();

	auto one = jive::create_bitconstant(theta->region(), ti->nbits, 1);
	auto uf = jive::create_bitconstant(theta->region(), ti->nbits, factor);
	auto r = jive::bits::create_sub(ti->nbits, maxorigin, minorigin);
	if (ti->eqop) r = jive::bits::create_add(ti->nbits, r, one);
	auto cmp = jive::bits::create_sge(ti->nbits, r, uf);
	auto pred = jive::ctl::match(1, {{1, 1}}, 0, 2, cmp);

	/* handle gamma with unrolled loop */
	jive::substitution_map smap;
	{
		auto ngamma = jive::gamma_node::create(pred, 2);
		auto ntheta = jive::theta_node::create(ngamma->subregion(1));

		jive::substitution_map rmap[2];
		for (const auto & olv : *theta) {
			auto ev = ngamma->add_entryvar(olv.input()->origin());
			auto nlv = ntheta->add_loopvar(ev->argument(1));
			rmap[0].insert(olv.output(), ev->argument(0));
			rmap[1].insert(olv.argument(), nlv->argument());
		}

		/* unroll loop body */
		for (size_t n = 0; n < factor-1; n++) {
			theta->subregion()->copy(ntheta->subregion(), rmap[1], false, false);
			jive::substitution_map tmap;
			for (const auto & olv : *theta)
				tmap.insert(olv.argument(), rmap[1].lookup(olv.result()->origin()));
			rmap[1] = tmap;
		}
		theta->subregion()->copy(ntheta->subregion(), rmap[1], false, false);

		for (auto olv = theta->begin(), nlv = ntheta->begin(); olv != theta->end(); olv++, nlv++) {
			auto origin = rmap[1].lookup(olv->result()->origin());
			nlv->result()->divert_origin(origin);
			rmap[1].insert(olv->output(), nlv->output());
		}

		ntheta->set_predicate(rmap[1].lookup(theta->predicate()->origin()));

		for (const auto & olv : *theta) {
			auto output = olv.output();
			auto xv = ngamma->add_exitvar({rmap[0].lookup(output), rmap[1].lookup(output)});
			smap.insert(olv.output(), xv->output());
		}

		/* add new loop condition */
		auto evr = ngamma->add_entryvar(r);
		auto lvr = ntheta->add_loopvar(evr->argument(1));

		auto uf = jive::create_bitconstant(ntheta->subregion(), ti->nbits, factor);
		auto sub = jive::bits::create_sub(ti->nbits, lvr->argument(), uf);
		auto cmp = jive::bits::create_sge(ti->nbits, sub, uf);
		auto pred = jive::ctl::match(1, {{1, 1}}, 0, 2, cmp);

		lvr->result()->divert_origin(sub);
		ntheta->predicate()->divert_origin(pred);

		auto xvr = ngamma->add_exitvar({evr->argument(0), lvr->output()});
		r = xvr->output();
	}

	auto zero = jive::create_bitconstant(theta->region(), ti->nbits, 0);
	cmp = jive::bits::create_sgt(ti->nbits, r, zero);
	pred = jive::ctl::match(1, {{1, 1}}, 0, 2, cmp);

	/* handle gamma for leftover iterations */
	{
		auto ngamma = jive::gamma_node::create(pred, 2);
		auto ntheta = jive::theta_node::create(ngamma->subregion(1));

		jive::substitution_map rmap[2];
		for (const auto & olv : *theta) {
			auto ev = ngamma->add_entryvar(smap.lookup(olv.output()));
			auto nlv = ntheta->add_loopvar(ev->argument(1));
			rmap[0].insert(olv.output(), ev->argument(0));
			rmap[1].insert(olv.argument(), nlv->argument());
		}

		theta->subregion()->copy(ntheta->subregion(), rmap[1], false, false);

		for (auto olv = theta->begin(), nlv = ntheta->begin(); olv != theta->end(); olv++, nlv++) {
			auto origin = rmap[1].lookup(olv->result()->origin());
			nlv->result()->divert_origin(origin);
			auto xv = ngamma->add_exitvar({rmap[0].lookup(olv->output()), nlv->output()});
			smap.insert(olv->output(), xv->output());
		}

		ntheta->set_predicate(rmap[1].lookup(theta->predicate()->origin()));

		/* add new loop condition */
		auto evr = ngamma->add_entryvar(r);
		auto lvr = ntheta->add_loopvar(evr->argument(1));

		auto zero = jive::create_bitconstant(ntheta->subregion(), ti->nbits, 0);
		auto one = jive::create_bitconstant(ntheta->subregion(), ti->nbits, 1);
		auto sub = jive::bits::create_sub(ti->nbits, lvr->argument(), one);
		auto cmp = jive::bits::create_sgt(ti->nbits, sub, zero);
		auto pred = jive::ctl::match(1, {{1, 1}}, 0, 2, cmp);

		lvr->result()->divert_origin(sub);
		ntheta->predicate()->divert_origin(pred);
	}

	for (const auto & olv : *theta)
		olv.output()->replace(smap.lookup(olv.output()));
	remove(theta);
}

static void
unroll(jive::region * region, size_t factor)
{
	for (auto & node : jive::topdown_traverser(region)) {
		if (auto structnode = dynamic_cast<jive::structural_node*>(node)) {
			for (size_t n = 0; n < structnode->nsubregions(); n++)
				unroll(structnode->subregion(n), factor);

			if (auto theta = dynamic_cast<jive::theta_node*>(node))
				unroll(theta, factor);
		}
	}
}

void
unroll(jive::graph & rvsdg, size_t factor)
{
	if (factor < 2)
		return;

	/* FIXME: Remove tracing again */
	jive::binary_op::normal_form(&rvsdg)->set_mutable(false);

	unroll(rvsdg.root(), factor);

	jive::binary_op::normal_form(&rvsdg)->set_mutable(true);
}

}
