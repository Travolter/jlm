/*
 * Copyright 2020 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include "aa-tests.hpp"

#include <test-registry.hpp>

#include <jive/view.h>

#include <jlm/opt/alias-analyses/steensgaard.hpp>
#include <jlm/util/stats.hpp>

static void
run_steensgaard(jlm::rvsdg_module & module)
{
	using namespace jlm;

	aa::steensgaard stgd;
	stats_descriptor sd;
	stgd.run(module, sd);
}

static void
test_store1()
{
	using namespace jlm;

	auto module = setup_store_test1();
	auto graph = module->graph();
//	jive::view(graph->root(), stdout);

	run_steensgaard(*module);

	/**
	* FIXME: Add assertions
	* {a} -> {b} -> {c} -> {d}
	*/
}

static void
test_store2()
{
	using namespace jlm;

	auto module = setup_store_test2();
	auto graph = module->graph();
//	jive::view(graph->root(), stdout);

	run_steensgaard(*module);
	/**
	* FIXME: Add assertions
	* {p} -> {x,y} -> {a,b}
	*/
}

static void
test_load1()
{
	using namespace jlm;

	auto module = setup_load_test1();
	auto graph = module->graph();
//	jive::view(graph->root(), stdout);

	run_steensgaard(*module);
	/**
	* FIXME: Add assertions
	*/
}

static void
test_load2()
{
	using namespace jlm;

	auto module = setup_load_test2();
	auto graph = module->graph();
//	jive::view(graph->root(), stdout);

	run_steensgaard(*module);
	/**
	* FIXME: Add assertions
	* {p} -> {x} -> {a,b} <- {y}
	*/
}

static void
test_getelementptr()
{
	using namespace jlm;

	auto module = setup_getelementptr_test();
	auto graph = module->graph();
//	jive::view(graph->root(), stdout);

	run_steensgaard(*module);
	/**
	* FIXME: Add assertions
	* {p, p->x, p->y} -> {ld(p->x), ld(p->y)}
	*/
}

static void
test_bitcast()
{
	using namespace jlm;

	auto module = setup_bitcast_test();
	auto graph = module->graph();
//	jive::view(graph->root(), stdout);

	run_steensgaard(*module);
	/**
	* FIXME: Add assertions
	* {Lambda:arg, bitcast result}
	*/
}

static void
test_null()
{
	using namespace jlm;

	auto module = setup_null_test();
	auto graph = module->graph();
//	jive::view(graph->root(), stdout);

	run_steensgaard(*module);
	/**
	* FIXME: Add assertions
	*/
}

static void
test_call()
{
	using namespace jlm;

	auto module = setup_call_test();
	auto graph = module->graph();
//	jive::view(graph->root(), stdout);

	run_steensgaard(*module);
	/**
	* FIXME: Add assertions
	*/
}

static void
test_call2()
{
	using namespace jlm;

	auto module = setup_call_test2();
	auto graph = module->graph();
//	jive::view(graph->root(), stdout);

	run_steensgaard(*module);
	/**
	* FIXME: Add assertions
	*/
}

static void
test_gamma()
{
	using namespace jlm;

	auto module = setup_gamma_test();
	auto graph = module->graph();
//	jive::view(graph->root(), stdout);

	run_steensgaard(*module);
	/**
	* FIXME: Add assertions
	*/
}

static void
test_theta()
{
	using namespace jlm;

	auto module = setup_theta_test();
	auto graph = module->graph();
//	jive::view(graph->root(), stdout);

	run_steensgaard(*module);
	/**
	* FIXME: Add assertions
	*/
}

static void
test_delta()
{
	using namespace jlm;

	auto module = setup_delta_test();
	auto graph = module->graph();
//	jive::view(graph->root(), stdout);

	run_steensgaard(*module);
	/**
	* FIXME: Add assertions
	*/
}

static void
test_alloc_store_load()
{
	using namespace jlm;

	auto module = setup_alloc_store_load_test();
	auto graph = module->graph();
//	jive::view(graph->root(), stdout);

	aa::steensgaard stgd;
	stats_descriptor sd;
	stgd.run(*module, sd);

	//FIXME: add assertions
}

static int
test()
{
//	test_store1();
//	test_store2();
//	test_load1();
//		test_load2();
//	test_getelementptr();
//	test_bitcast();
	test_null();
//	test_call();
//	test_call2();

//	test_gamma();
//	test_theta();
//	test_delta();

//	test_alloc_store_load();

	return 0;
}

JLM_UNIT_TEST_REGISTER("libjlm/opt/alias-analyses/test-steensgaard", test)
