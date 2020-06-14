/*
 * Copyright 2020 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jive/rvsdg/theta.h>

#include <jlm/ir/rvsdg-module.hpp>
#include <jlm/ir/operators.hpp>

/**
* This function sets up an RVSDG representing the following function:
*
* \code{.c}
*   void f()
*   {
*     uint32_t d;
*     uint32_t * c;
*     uint32_t ** b;
*     uint32_t *** a;
*
*     a = &b;
*     b = &c;
*     c = &d;
*   }
* \endcode
*
* It uses a single memory state to sequentialize the respective memory
* operations.
*/
static inline std::unique_ptr<jlm::rvsdg_module>
setup_store_test1()
{
	using namespace jlm;

	auto pt = ptrtype::create(jive::bit32);
	auto ppt = ptrtype::create(*pt);
	auto pppt = ptrtype::create(*ppt);
	jive::fcttype fcttype(
	  {&jive::memtype::instance()}
	, {&jive::memtype::instance()});

	auto module = rvsdg_module::create(filepath(""), "", "");
	auto graph = module->graph();

	auto nf = graph->node_normal_form(typeid(jive::operation));
	nf->set_mutable(false);

	lambda_op op(fcttype, "f", linkage::external_linkage);

	lambda_builder lb;
	auto fctargs = lb.begin_lambda(graph->root(), op);

	auto size = jive::create_bitconstant(lb.subregion(), 32, 4);

	auto d = alloca_op::create(jive::bit32, size, 4);
	auto c = alloca_op::create(*pt, size, 4);
	auto b = alloca_op::create(*ppt, size, 4);
	auto a = alloca_op::create(*pppt, size, 4);

	auto mux1 = memstatemux_op::create_merge({d[1], fctargs[0]});
	auto mux2 = memstatemux_op::create_merge({c[1], mux1});
	auto mux3 = memstatemux_op::create_merge({b[1], mux2});
	auto mux4 = memstatemux_op::create_merge({a[1], mux3});

	auto a_amp_b = store_op::create(a[0], b[0], {mux4}, 4);
	auto b_amp_c = store_op::create(b[0], c[0], {a_amp_b[0]}, 4);
	auto c_amp_d = store_op::create(c[0], d[0], {b_amp_c[0]}, 4); 

	auto fct = lb.end_lambda({c_amp_d[0]});

	graph->add_export(fct->output(0), {ptrtype(fct->fcttype()), "f"});

	return module;
}

/**
* This function sets up an RVSDG representing the following function:
*
* \code{.c}
*   void f()
*   {
*     uint32_t a, b;
*     uint32_t * x, * y;
*     uint32_t ** p;
*
*     x = &a;
*     y = &b;
*     p = &x;
*     p = &y;
*   }
* \endcode
*
* It uses a single memory state to sequentialize the respective memory
* operations.
*/
static inline std::unique_ptr<jlm::rvsdg_module>
setup_store_test2()
{
	using namespace jlm;

	auto pt = ptrtype::create(jive::bit32);
	auto ppt = ptrtype::create(*pt);
	jive::fcttype fcttype(
	  {&jive::memtype::instance()}
	, {&jive::memtype::instance()});

	auto module = rvsdg_module::create(filepath(""), "", "");
	auto graph = module->graph();

	auto nf = graph->node_normal_form(typeid(jive::operation));
	nf->set_mutable(false);

	lambda_op op(fcttype, "f", linkage::external_linkage);

	lambda_builder lb;
	auto fctargs = lb.begin_lambda(graph->root(), op);

	auto size = jive::create_bitconstant(lb.subregion(), 32, 4);

	auto a = alloca_op::create(jive::bit32, size, 4);
	auto b = alloca_op::create(jive::bit32, size, 4);
	auto x = alloca_op::create(*pt, size, 4);
	auto y = alloca_op::create(*pt, size, 4);
	auto p = alloca_op::create(*ppt, size, 4);

	auto mux1 = memstatemux_op::create_merge({a[1], fctargs[0]});
	auto mux2 = memstatemux_op::create_merge({b[1], mux1});
	auto mux3 = memstatemux_op::create_merge({x[1], mux2});
	auto mux4 = memstatemux_op::create_merge({y[1], mux3});
	auto mux5 = memstatemux_op::create_merge({p[1], mux4});

	auto x_amp_a = store_op::create(x[0], a[0], {mux5}, 4);
	auto y_amp_b = store_op::create(y[0], b[0], {x_amp_a[0]}, 4);
	auto p_amp_x = store_op::create(p[0], x[0], {y_amp_b[0]}, 4); 
	auto p_amp_y = store_op::create(p[0], y[0], {p_amp_x[0]}, 4); 

	auto fct = lb.end_lambda({p_amp_y[0]});

	graph->add_export(fct->output(0), {ptrtype(fct->fcttype()), "f"});

	return module;
}

/**
* This function sets up an RVSDG representing the following function:
*
* \code{.c}
*   void f()
*   {
*     uint32_t a, b;
*     uint32_t * c, * d;
*     uint32_t ** x, ** y;
*
*     c = &a;
*     d = &b;
*     x = &c;
*     y = &d;
*     x = y;
*   }
* \endcode
*
* It uses a single memory state to sequentialize the respective memory
* operations.
*/
static inline std::unique_ptr<jlm::rvsdg_module>
setup_assignment_test()
{
	using namespace jlm;

	auto pt = ptrtype::create(jive::bit32);
	auto ppt = ptrtype::create(*pt);
	jive::fcttype fcttype(
	  {&jive::memtype::instance()}
	, {&jive::memtype::instance()});

	auto module = rvsdg_module::create(filepath(""), "", "");
	auto graph = module->graph();

	auto nf = graph->node_normal_form(typeid(jive::operation));
	nf->set_mutable(false);

	lambda_op op(fcttype, "f", linkage::external_linkage);

	lambda_builder lb;
	auto fctargs = lb.begin_lambda(graph->root(), op);

	auto size = jive::create_bitconstant(lb.subregion(), 32, 4);

	auto a = alloca_op::create(jive::bit32, size, 4);
	auto b = alloca_op::create(jive::bit32, size, 4);
	auto c = alloca_op::create(*pt, size, 4);
	auto d = alloca_op::create(*pt, size, 4);
	auto x = alloca_op::create(*ppt, size, 4);
	auto y = alloca_op::create(*ppt, size, 4);

	auto mux1 = memstatemux_op::create_merge({a[1], fctargs[0]});
	auto mux2 = memstatemux_op::create_merge({b[1], mux1});
	auto mux3 = memstatemux_op::create_merge({c[1], mux2});
	auto mux4 = memstatemux_op::create_merge({d[1], mux3});
	auto mux5 = memstatemux_op::create_merge({x[1], mux4});
	auto mux6 = memstatemux_op::create_merge({y[1], mux5});

	auto c_amp_a = store_op::create(c[0], a[0], {mux6}, 4);
	auto d_amp_b = store_op::create(d[0], b[0], {c_amp_a[0]}, 4);
	auto x_amp_c = store_op::create(x[0], c[0], {d_amp_b[0]}, 4); 
	auto y_amp_d = store_op::create(y[0], d[0], {x_amp_c[0]}, 4); 

	auto x_eq_y = store_op::create(x[0], d[0], {y_amp_d[0]}, 4);

	auto fct = lb.end_lambda({x_eq_y[0]});

	graph->add_export(fct->output(0), {ptrtype(fct->fcttype()), "f"});

	return module;
}

/**
* This function sets up an RVSDG representing the following function:
*
* \code{.c}
*   uint32_t f(uint32_t ** p)
*   {
*     uint32_t * x = *p;
*     uint32_t a = *x;
*     return a;
*   }
* \endcode
*
* It uses a single memory state to sequentialize the respective memory
* operations.
*/
static inline std::unique_ptr<jlm::rvsdg_module>
setup_load_test1()
{
	using namespace jlm;

	auto pt = ptrtype::create(jive::bit32);
	auto ppt = ptrtype::create(*pt);
	jive::fcttype fcttype(
	  {ppt.get(), &jive::memtype::instance()}
	, {&jive::bit32, &jive::memtype::instance()});

	auto module = rvsdg_module::create(filepath(""), "", "");
	auto graph = module->graph();

	auto nf = graph->node_normal_form(typeid(jive::operation));
	nf->set_mutable(false);

	lambda_op op(fcttype, "f", linkage::external_linkage);

	lambda_builder lb;
	auto fctargs = lb.begin_lambda(graph->root(), op);

	auto ld1 = load_op::create(fctargs[0], {fctargs[1]}, 4);
	auto ld2 = load_op::create(ld1[0], {ld1[1]}, 4);

	auto fct = lb.end_lambda(ld2);

	graph->add_export(fct->output(0), {ptrtype(fct->fcttype()), "f"});

	return module;
}

/**
* This function sets up an RVSDG representing the following function:
*
* \code{.c}
*   void f()
*   {
*     uint32_t a, b;
*     uint32_t * x, * y;
*     uint32_t ** p;
*
*     x = &a;
*     y = &b;
*     p = &x;
*     y = *p;
*   }
* \endcode
*
* It uses a single memory state to sequentialize the respective memory
* operations.
*/
static inline std::unique_ptr<jlm::rvsdg_module>
setup_load_test2()
{
	using namespace jlm;

	auto pt = ptrtype::create(jive::bit32);
	auto ppt = ptrtype::create(*pt);
	jive::fcttype fcttype(
	  {&jive::memtype::instance()}
	, {&jive::memtype::instance()});

	auto module = rvsdg_module::create(filepath(""), "", "");
	auto graph = module->graph();

	auto nf = graph->node_normal_form(typeid(jive::operation));
	nf->set_mutable(false);

	lambda_op op(fcttype, "f", linkage::external_linkage);

	lambda_builder lb;
	auto fctargs = lb.begin_lambda(graph->root(), op);

	auto size = jive::create_bitconstant(lb.subregion(), 32, 4);

	auto a = alloca_op::create(jive::bit32, size, 4);
	auto b = alloca_op::create(jive::bit32, size, 4);
	auto x = alloca_op::create(*pt, size, 4);
	auto y = alloca_op::create(*pt, size, 4);
	auto p = alloca_op::create(*ppt, size, 4);

	auto mux1 = memstatemux_op::create_merge({a[1], fctargs[0]});
	auto mux2 = memstatemux_op::create_merge({b[1], mux1});
	auto mux3 = memstatemux_op::create_merge({x[1], mux2});
	auto mux4 = memstatemux_op::create_merge({y[1], mux3});
	auto mux5 = memstatemux_op::create_merge({p[1], mux4});

	auto x_amp_a = store_op::create(x[0], a[0], {mux5}, 4);
	auto y_amp_b = store_op::create(y[0], b[0], x_amp_a, 4);
	auto p_amp_x = store_op::create(p[0], x[0], y_amp_b, 4);

	auto ld1 = load_op::create(p[0], p_amp_x, 4);
	auto ld2 = load_op::create(ld1[0], {ld1[1]}, 4);
	auto y_star_p = store_op::create(y[0], ld2[0], {ld2[1]}, 4);

	auto fct = lb.end_lambda({y_star_p[0]});

	graph->add_export(fct->output(0), {ptrtype(fct->fcttype()), "f"});

	return module;
}

/**
* This function sets up an RVSDG representing the following function:
*
* \code{.c}
*   struct point {
*     uint32_t x;
*     uint32_t y;
*   };
*
*   uint32_t f(const struct point * p)
*   {
*     return p->x + p->y;
*   }
* \endcode
*
* It uses a single memory state to sequentialize the respective memory
* operations.
*/
static inline std::unique_ptr<jlm::rvsdg_module>
setup_getelementptr_test()
{
	using namespace jlm;

	auto dcl = jive::rcddeclaration::create({&jive::bit32, &jive::bit32});
	jive::rcdtype rt(dcl.get());

	auto pt = ptrtype::create(rt);
	auto pbt = ptrtype::create(jive::bit32);
	jive::fcttype fcttype(
	  {pt.get(), &jive::memtype::instance()}
	, {&jive::bit32, &jive::memtype::instance()});

	auto module = rvsdg_module::create(filepath(""), "", "");
	auto graph = module->graph();

	auto nf = graph->node_normal_form(typeid(jive::operation));
	nf->set_mutable(false);

	lambda_op op(fcttype, "f", linkage::external_linkage);

	lambda_builder lb;
	auto fctargs = lb.begin_lambda(graph->root(), op);

	auto zero = jive::create_bitconstant(lb.subregion(), 32, 0);
	auto one = jive::create_bitconstant(lb.subregion(), 32, 1);

	auto gepx = getelementptr_op::create(fctargs[0], {zero, zero}, *pbt);
	auto ldx = load_op::create(gepx, {fctargs[1]}, 4);

	auto gepy = getelementptr_op::create(fctargs[0], {zero, one}, *pbt);
	auto ldy = load_op::create(gepy, {ldx[1]}, 4);

	auto sum = jive::bitadd_op::create(32, ldx[0], ldy[0]);

	auto fct = lb.end_lambda({sum, ldy[1]});

	graph->add_export(fct->output(0), {ptrtype(fct->fcttype()), "f"});

	return module;
}

/**
* This function sets up an RVSDG representing the following function:
*
* \code{.c}
*   uint16_t * f(uint32_t * p)
*   {
*     return (uint16_t*)p;
*   }
* \endcode
*/
static inline std::unique_ptr<jlm::rvsdg_module>
setup_bitcast_test()
{
	using namespace jlm;

	auto pbt16 = ptrtype::create(jive::bit16);
	auto pbt32 = ptrtype::create(jive::bit32);
	jive::fcttype fcttype({pbt32.get()}, {pbt16.get()});

	auto module = rvsdg_module::create(filepath(""), "", "");
	auto graph = module->graph();

	auto nf = graph->node_normal_form(typeid(jive::operation));
	nf->set_mutable(false);

	lambda_op op(fcttype, "f", linkage::external_linkage);

	lambda_builder lb;
	auto fctargs = lb.begin_lambda(graph->root(), op);

	auto cast = bitcast_op::create(fctargs[0], *pbt16);

	auto fct = lb.end_lambda({cast});

	graph->add_export(fct->output(0), {ptrtype(fct->fcttype()), "f"});

	return module;
}

/**
* This function sets up an RVSDG representing the following function:
*
* \code{.c}
*   uint32_t * f()
*   {
*     return NULL;
*   }
* \endcode
*/
static inline std::unique_ptr<jlm::rvsdg_module>
setup_null_test()
{
	using namespace jlm;

	auto pt = ptrtype::create(jive::bit32);
	auto ppt = ptrtype::create(*pt);
	jive::fcttype fcttype(
	  {ppt.get(), &jive::memtype::instance()}
	, {&jive::memtype::instance()});

	auto module = rvsdg_module::create(filepath(""), "", "");
	auto graph = module->graph();

	auto nf = graph->node_normal_form(typeid(jive::operation));
	nf->set_mutable(false);

	lambda_op op(fcttype, "f", linkage::external_linkage);

	lambda_builder lb;
	auto fctargs = lb.begin_lambda(graph->root(), op);

	auto null = ptr_constant_null_op::create(lb.subregion(), *pt);
	auto st = store_op::create(fctargs[0], null, {fctargs[1]}, 4);

	auto fct = lb.end_lambda({st[0]});

	graph->add_export(fct->output(0), {ptrtype(fct->fcttype()), "f"});

	return module;
}

/**
* This function sets up an RVSDG representing the following function:
*
* \code{.c}
*	  uint32_t
*	  f(uint32_t * x, uint32_t * y)
*	  {
*	    return *x + *y;
*	  }
*
*	  uint32_t
*	  g(uint32_t * x, uint32_t * y)
*	  {
*	    return *x - *y;
*	  }
*
*	  uint32_t
*	  h()
*	  {
*	    uint32_t x = 5, y = 6, z = 7;
*	    return f(&x, &y) + g(&z, &z);
*	  }
* \endcode
*
* It uses a single memory state to sequentialize the respective memory
* operations within each function.
*/
static inline std::unique_ptr<jlm::rvsdg_module>
setup_call_test()
{
	using namespace jlm;

	auto pt = ptrtype::create(jive::bit32);
	jive::fcttype ft1(
		{pt.get(), pt.get(), &jive::memtype::instance()}
	, {&jive::bit32, &jive::memtype::instance()});

	jive::fcttype ft2(
	  {&jive::memtype::instance()}
	, {&jive::bit32, &jive::memtype::instance()});

	auto module = rvsdg_module::create(filepath(""), "", "");
	auto graph = module->graph();

	auto nf = graph->node_normal_form(typeid(jive::operation));
	nf->set_mutable(false);

	lambda_builder lb;

	/* function f */
	lambda_op fop(ft1, "f", linkage::external_linkage);
	auto fargs = lb.begin_lambda(graph->root(), fop);

	auto ld1 = load_op::create(fargs[0], {fargs[2]}, 4);
	auto ld2 = load_op::create(fargs[1], {ld1[1]}, 4);

	auto sum = jive::bitadd_op::create(32, ld1[0], ld2[0]);

	auto f = lb.end_lambda({sum, ld2[1]});
	graph->add_export(f->output(0), {ptrtype(f->fcttype()), "f"});

	/* function g */
	lambda_op gop(ft1, "g", linkage::external_linkage);
	auto gargs = lb.begin_lambda(graph->root(), gop);

	ld1 = load_op::create(gargs[0], {gargs[2]}, 4);
	ld2 = load_op::create(gargs[1], {ld1[1]}, 4);

	auto diff = jive::bitsub_op::create(32, ld1[0], ld2[0]);

	auto g = lb.end_lambda({diff, ld2[1]});
	graph->add_export(g->output(0), {ptrtype(g->fcttype()), "g"});

	/* function h */
	lambda_op hop(ft2, "h", linkage::external_linkage);
	auto hargs = lb.begin_lambda(graph->root(), hop);

	auto cvf = lb.add_dependency(f->output(0));
	auto cvg = lb.add_dependency(g->output(0));

	auto size = jive::create_bitconstant(lb.subregion(), 32, 4);

	auto x = alloca_op::create(jive::bit32, size, 4);
	auto y = alloca_op::create(jive::bit32, size, 4);
	auto z = alloca_op::create(jive::bit32, size, 4);

	auto mx = memstatemux_op::create_merge({x[1], hargs[0]});
	auto my = memstatemux_op::create_merge({y[1], mx});
	auto mz = memstatemux_op::create_merge({z[1], my});

	auto five = jive::create_bitconstant(lb.subregion(), 32, 5);
	auto six = jive::create_bitconstant(lb.subregion(), 32, 6);
	auto seven = jive::create_bitconstant(lb.subregion(), 32, 7);

	auto stx = store_op::create(x[0], five, {mz}, 4);
	auto sty = store_op::create(y[0], six, {stx[0]}, 4);
	auto stz = store_op::create(z[0], seven, {sty[0]}, 4);

	auto callf = call_op::create(cvf, {x[0], y[0], stz[0]});
	auto callg = call_op::create(cvg, {z[0], z[0], callf[1]});

	sum = jive::bitadd_op::create(32, callf[0], callg[0]);

	auto h = lb.end_lambda({sum, callg[1]});
	graph->add_export(h->output(0), {ptrtype(h->fcttype()), "h"});

	return module;
}

/**
* This function sets up an RVSDG representing the following function:
*
* \code{.c}
*	  uint32_t *
*	  create(size_t n)
*	  {
*	    return (uint32_t*)malloc(n * sizeof(uint32_t));
*	  }
*
*	  void
*	  destroy(uint32_t * p)
*	  {
*	    free(p);
*	  }
*
*	  void
*	  test()
*	  {
*		  uint32_t * p1 = create(6);
*		  uint32_t * p2 = create(7);
*
*	    destroy(p1);
*		  destroy(p2);
*	  }
* \endcode
*
* It uses a single memory state to sequentialize the respective memory
* operations within each function.
*/
static inline std::unique_ptr<jlm::rvsdg_module>
setup_call_test2()
{
	using namespace jlm;

	auto pbit8 = ptrtype::create(jive::bit8);
	auto pbit32 = ptrtype::create(jive::bit32);

	jive::fcttype create_type(
		{&jive::bit32, &jive::memtype::instance()}
	, {pbit32.get(), &jive::memtype::instance()});

	jive::fcttype destroy_type(
	  {pbit32.get(), &jive::memtype::instance()}
	, {&jive::memtype::instance()});

	jive::fcttype test_type(
	  {&jive::memtype::instance()}
	, {&jive::memtype::instance()});

	jive::fcttype free_type(
	  {pbit8.get(), &jive::memtype::instance()}
	, {&jive::memtype::instance()});

	auto module = rvsdg_module::create(filepath(""), "", "");
	auto graph = module->graph();

	auto nf = graph->node_normal_form(typeid(jive::operation));
	nf->set_mutable(false);

	auto free = graph->add_import({ptrtype(free_type), "free"});

	lambda_builder lb;

	/* function create */
	lambda_op create_op(create_type, "create", linkage::external_linkage);
	auto cargs = lb.begin_lambda(graph->root(), create_op);

	auto four = jive::create_bitconstant(lb.subregion(), 32, 4);
	auto prod = jive::bitmul_op::create(32, cargs[0], four);

	auto alloc = malloc_op::create(prod);
	auto cast = bitcast_op::create(alloc[0], *pbit32);
	auto mx = memstatemux_op::create_merge({alloc[1], cargs[1]});

	auto create = lb.end_lambda({cast, mx});
	graph->add_export(create->output(0), {ptrtype(create->fcttype()), "create"});

	/* function destroy */
	lambda_op destroy_op(destroy_type, "destroy", linkage::external_linkage);
	auto dargs = lb.begin_lambda(graph->root(), destroy_op);
	auto free_cv = lb.add_dependency(free);

	cast = bitcast_op::create(dargs[0], *pbit8);
	auto call = call_op::create(free_cv, {cast, dargs[1]});

	auto destroy = lb.end_lambda(call);
	graph->add_export(destroy->output(0), {ptrtype(destroy->fcttype()), "destroy"});

	/* function test */
	lambda_op test_op(test_type, "test", linkage::external_linkage);
	auto targs = lb.begin_lambda(graph->root(), test_op);
	auto create_cv = lb.add_dependency(create->output(0));
	auto destroy_cv = lb.add_dependency(destroy->output(0));

	auto six = jive::create_bitconstant(lb.subregion(), 32, 6);
	auto seven = jive::create_bitconstant(lb.subregion(), 32, 7);

	auto call_create1 = call_op::create(create_cv, {six, targs[0]});
	auto call_create2 = call_op::create(create_cv, {seven, call_create1[1]});

	auto call_destroy1 = call_op::create(destroy_cv, {call_create1[0], call_create2[1]});
	auto call_destroy2 = call_op::create(destroy_cv, {call_create2[0], call_destroy1[0]});

	auto test = lb.end_lambda(call_destroy2);
	graph->add_export(test->output(0), {ptrtype(test->fcttype()), "test"});

	return module;
}

/**
* This function sets up an RVSDG representing the following function:
*
* \code{.c}
*   uint32_t f(uint32_t c, uint32_t * p1, uint32_t * p2, uint32_t * p3, uint32_t * p4)
*   {
*		  uint32_t * tmp1, * tmp2;
*     if (c == 0) {
*		    tmp1 = p1;
*       tmp2 = p2;
*     } else {
*		    tmp1 = p3;
*       tmp2 = p4;
*     }
*		  return *tmp1 + *tmp2;
*   }
* \endcode
*
* It uses a single memory state to sequentialize the respective memory
* operations.
*/
static inline std::unique_ptr<jlm::rvsdg_module>
setup_gamma_test()
{
	using namespace jlm;

	auto pt = ptrtype::create(jive::bit32);
	jive::fcttype fcttype(
		{&jive::bit32, pt.get(), pt.get(), pt.get(), pt.get(), &jive::memtype::instance()}
	, {&jive::bit32, &jive::memtype::instance()});

	auto module = rvsdg_module::create(filepath(""), "", "");
	auto graph = module->graph();

	auto nf = graph->node_normal_form(typeid(jive::operation));
	nf->set_mutable(false);

	lambda_op op(fcttype, "f", linkage::external_linkage);

	lambda_builder lb;
	auto fctargs = lb.begin_lambda(graph->root(), op);

	auto zero = jive::create_bitconstant(lb.subregion(), 32, 0);
	auto biteq = jive::biteq_op::create(32, fctargs[0], zero);
	auto predicate = jive::match(1, {{0, 1}}, 0, 2, biteq);

	auto gamma = jive::gamma_node::create(predicate, 2);
	auto p1ev = gamma->add_entryvar(fctargs[1]);
	auto p2ev = gamma->add_entryvar(fctargs[2]);
	auto p3ev = gamma->add_entryvar(fctargs[3]);
	auto p4ev = gamma->add_entryvar(fctargs[4]);

	auto tmp1 = gamma->add_exitvar({p1ev->argument(0), p3ev->argument(1)});
	auto tmp2 = gamma->add_exitvar({p2ev->argument(0), p4ev->argument(1)}); 

	auto ld1 = load_op::create(tmp1, {fctargs[5]}, 4);
	auto ld2 = load_op::create(tmp2, {ld1[1]}, 4);
	auto sum = jive::bitadd_op::create(32, ld1[0], ld2[0]);

	auto fct = lb.end_lambda({sum, ld2[1]});

	graph->add_export(fct->output(0), {ptrtype(fct->fcttype()), "f"});

	return module;
}

/**
* This function sets up an RVSDG representing the following function:
*
* \code{.c}
*   void f(uint32_t l, uint32_t  a[], uint32_t c)
*   {
*		  uint32_t n = 0;
*		  do {
*		    a[n++] = c;
*		  } while (n < l);
*   }
* \endcode
*
* It uses a single memory state to sequentialize the respective memory
* operations.
*/
static inline std::unique_ptr<jlm::rvsdg_module>
setup_theta_test()
{
	using namespace jlm;

	auto pt = ptrtype::create(jive::bit32);
	jive::fcttype fcttype(
		{&jive::bit32, pt.get(), &jive::bit32, &jive::memtype::instance()}
	, {&jive::memtype::instance()});

	auto module = rvsdg_module::create(filepath(""), "", "");
	auto graph = module->graph();

	auto nf = graph->node_normal_form(typeid(jive::operation));
	nf->set_mutable(false);

	lambda_op op(fcttype, "f", linkage::external_linkage);

	lambda_builder lb;
	auto fctargs = lb.begin_lambda(graph->root(), op);

	auto zero = jive::create_bitconstant(lb.subregion(), 32, 0);

	auto theta = jive::theta_node::create(lb.subregion());

	auto n = theta->add_loopvar(zero);
	auto l = theta->add_loopvar(fctargs[0]);
	auto a = theta->add_loopvar(fctargs[1]);
	auto c = theta->add_loopvar(fctargs[2]);
	auto s = theta->add_loopvar(fctargs[3]);

	auto gep = getelementptr_op::create(a->argument(), {n->argument()}, *pt);
	auto store = store_op::create(gep, c->argument(), {s->argument()}, 4);

	auto one = jive::create_bitconstant(theta->subregion(), 32, 1);
	auto sum = jive::bitadd_op::create(32, n->argument(), one);
	auto cmp = jive::bitult_op::create(32, sum, l->argument()); 
	auto predicate = jive::match(1, {{1, 1}}, 0, 2, cmp);

	n->result()->divert_to(sum);
	s->result()->divert_to(store[0]);
	theta->set_predicate(predicate);

	auto fct = lb.end_lambda({s});

	graph->add_export(fct->output(0), {ptrtype(fct->fcttype()), "f"});

	return module;
}

/**
* This function sets up an RVSDG representing the following function:
*
* \code{.c}
*   uint32_t f;
*
*   uint32_t
*   g(uint32_t * v)
*   {
*     return *v;
*   }
*
*   uint32_t
*   h()
*   {
*     f = 5;
*     return g(&f);
*   }
* \endcode
*
* It uses a single memory state to sequentialize the respective memory
* operations.
*/
static inline std::unique_ptr<jlm::rvsdg_module>
setup_delta_test()
{
	using namespace jlm;

	auto pt = ptrtype::create(jive::bit32);
	jive::fcttype fctgtype(
		{pt.get(), &jive::memtype::instance()}
	, {&jive::bit32, &jive::memtype::instance()});
	jive::fcttype fcthtype(
		{&jive::memtype::instance()}
	, {&jive::bit32, &jive::memtype::instance()});

	auto module = rvsdg_module::create(filepath(""), "", "");
	auto graph = module->graph();

	auto nf = graph->node_normal_form(typeid(jive::operation));
	nf->set_mutable(false);

	delta_builder db;
	lambda_builder lb;

	/* global f */
	auto deltaregion = db.begin(graph->root(), ptrtype(jive::bit32), "f",
		linkage::external_linkage, false);
	auto f = db.end(jive::create_bitconstant(deltaregion, 32, 0)); 
	graph->add_export(f, {f->type(), "f"});

	/* function g */
	lambda_op gop(fctgtype, "g", linkage::external_linkage);	
	auto gargs = lb.begin_lambda(graph->root(), gop);

	auto ld = load_op::create(gargs[0], {gargs[1]}, 4);
	
	auto g = lb.end_lambda(ld);
	graph->add_export(g->output(0), {ptrtype(g->fcttype()), "g"});

	/* function h */
	lambda_op hop(fcthtype, "h", linkage::external_linkage);
	auto hargs = lb.begin_lambda(graph->root(), hop);
	auto cvf = lb.add_dependency(f);
	auto cvg = lb.add_dependency(g->output(0));

	auto five = jive::create_bitconstant(lb.subregion(), 32, 5);
	auto st = store_op::create(cvf, five, {hargs[0]}, 4);
	auto callg = call_op::create(cvg, {cvf, st[0]});

	auto h = lb.end_lambda(callg);
	graph->add_export(h->output(0), {ptrtype(h->fcttype()), "h"});

	return module;
}

/**
*	This function sets up an RVSDG representing the following function:
*
* \code{.c}
*   uint32_t f()
*   {
*     uint32_t x = 5, y = 6;
*     uint32_t *p = &x, *q = &y;
*     return *p + *q;
*   }
* \endcode
*
* It uses a single memory state to sequentialize the respective memory
* operations.
*/
static inline std::unique_ptr<jlm::rvsdg_module>
setup_alloc_store_load_test()
{
	using namespace jlm;

	jive::fcttype fcttype({&jive::memtype::instance()}, {&jive::bit32, &jive::memtype::instance()});

	auto module = rvsdg_module::create(filepath(""), "", "");
	auto graph = module->graph();

	lambda_op op(fcttype, "f", linkage::external_linkage);

	lambda_builder lb;
	auto fctargs = lb.begin_lambda(graph->root(), op);

	auto size = jive::create_bitconstant(lb.subregion(), 32, 4);

	auto alloca1 = alloca_op::create(jive::bit32, size, 4);
	auto alloca2 = alloca_op::create(jive::bit32, size, 4);

	auto mux1 = memstatemux_op::create_merge({alloca1[1], fctargs[0]});
	auto mux2 = memstatemux_op::create_merge({alloca2[1], mux1});

	auto c1 = jive::create_bitconstant(lb.subregion(), 32, 5);
	auto c2 = jive::create_bitconstant(lb.subregion(), 32, 6);

	auto store1 = store_op::create(alloca1[0], c1, {mux2}, 4);
	auto store2 = store_op::create(alloca2[0], c2, store1, 4);

	auto load1 = load_op::create(alloca1[0], store2, 4);
	auto load2 = load_op::create(alloca2[0], {load1[1]}, 4);

	auto add = jive::bitadd_op::create(32, load1[0], load2[0]);

	auto fct = lb.end_lambda({add, load2[1]});

	graph->add_export(fct->output(0), {ptrtype(fct->fcttype()), "f"});

	return module;
}

