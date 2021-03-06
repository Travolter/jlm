/*
 * Copyright 2014 2015 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/common.hpp>
#include <jlm/ir/basic-block.hpp>
#include <jlm/ir/cfg.hpp>
#include <jlm/ir/cfg-structure.hpp>
#include <jlm/ir/cfg-node.hpp>
#include <jlm/ir/ipgraph.hpp>
#include <jlm/ir/ipgraph-module.hpp>
#include <jlm/ir/operators/operators.hpp>
#include <jlm/llvm2jlm/context.hpp>
#include <jlm/llvm2jlm/instruction.hpp>
#include <jlm/llvm2jlm/module.hpp>
#include <jlm/llvm2jlm/type.hpp>

#include <jive/arch/addresstype.h>
#include <jive/rvsdg/type.h>

#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

namespace jlm
{

static std::vector<llvm::PHINode*>
convert_instructions(llvm::Function & function, context & ctx)
{
	std::vector<llvm::PHINode*> phis;
	llvm::ReversePostOrderTraversal<llvm::Function*> rpotraverser(&function);
	for (auto & bb : rpotraverser) {
		for (auto & instruction : *bb) {
			tacsvector_t tacs;
			if (auto result = convert_instruction(&instruction, tacs, ctx))
				ctx.insert_value(&instruction, result);

			if (auto phi = llvm::dyn_cast<llvm::PHINode>(&instruction)) {
				phis.push_back(phi);
				ctx.get(bb)->append_first(tacs);
			} else {
				ctx.get(bb)->append_last(tacs);
			}
		}
	}

	return phis;
}

static void
patch_phi_operands(const std::vector<llvm::PHINode*> & phis, context & ctx)
{
	for (const auto & phi : phis) {
		std::vector<cfg_node*> nodes;
		std::vector<const variable*> operands;
		for (size_t n = 0; n < phi->getNumOperands(); n++) {
			tacsvector_t tacs;
			auto bb = ctx.get(phi->getIncomingBlock(n));
			operands.push_back(convert_value(phi->getIncomingValue(n), tacs, ctx));
			bb->insert_before_branch(tacs);
			nodes.push_back(bb);
		}

		auto phi_tac = static_cast<const tacvariable*>(ctx.lookup_value(phi))->tac();
		phi_tac->replace(phi_op(nodes, phi_tac->result(0)->type()), operands, {phi_tac->result(0)});
	}
}

static basic_block_map
convert_basic_blocks(llvm::Function & f, jlm::cfg & cfg)
{
	basic_block_map bbmap;
	llvm::ReversePostOrderTraversal<llvm::Function*> rpotraverser(&f);
	for (auto & bb : rpotraverser)
			bbmap.insert(bb, basic_block::create(cfg));

	return bbmap;
}

std::unique_ptr<jlm::cfg>
create_cfg(llvm::Function & f, context & ctx)
{
	auto node = static_cast<const fctvariable*>(ctx.lookup_value(&f))->function();
	auto & m = ctx.module();

	std::unique_ptr<jlm::cfg> cfg(new jlm::cfg(ctx.module()));

	/* add arguments */
	size_t n = 0;
	for (const auto & arg : f.args()) {
		JLM_DEBUG_ASSERT(n < node->fcttype().narguments());
		auto & type = node->fcttype().argument_type(n++);
		auto v = ctx.module().create_variable(type, arg.getName().str());
		cfg->entry()->append_argument(v);
		ctx.insert_value(&arg, v);
	}
	if (f.isVarArg()) {
		JLM_DEBUG_ASSERT(n < node->fcttype().narguments());
		auto v = m.create_variable(node->fcttype().argument_type(n++), "_varg_");
		cfg->entry()->append_argument(v);
	}
	JLM_DEBUG_ASSERT(n < node->fcttype().narguments());
	auto memstate = m.create_variable(node->fcttype().argument_type(n++), "_s_");
	auto loopstate = m.create_variable(node->fcttype().argument_type(n++), "_l_");
	cfg->entry()->append_argument(memstate);
	cfg->entry()->append_argument(loopstate);
	JLM_DEBUG_ASSERT(n == node->fcttype().narguments());

	auto bbmap = convert_basic_blocks(f, *cfg);

	/* create entry block */
	auto entry_block = basic_block::create(*cfg);
	cfg->exit()->divert_inedges(entry_block);
	entry_block->add_outedge(bbmap[&f.getEntryBlock()]);

	/* add results */
	jlm::variable * result = nullptr;
	if (!f.getReturnType()->isVoidTy()) {
		result = m.create_variable(*convert_type(f.getReturnType(), ctx), "_r_");
		entry_block->append_last(create_undef_constant_tac(result));

		JLM_DEBUG_ASSERT(node->fcttype().nresults() == 3);
		JLM_DEBUG_ASSERT(result->type() == node->fcttype().result_type(0));
		cfg->exit()->append_result(result);
	}
	cfg->exit()->append_result(memstate);
	cfg->exit()->append_result(loopstate);

	/* convert instructions */
	ctx.set_basic_block_map(bbmap);
	ctx.set_memory_state(memstate);
	ctx.set_loop_state(loopstate);
	ctx.set_result(result);
	auto phis = convert_instructions(f, ctx);
	patch_phi_operands(phis, ctx);

	/* ensure that exit node has only one incoming edge */
	if (cfg->exit()->ninedges() > 1) {
		auto bb = basic_block::create(*cfg);
		cfg->exit()->divert_inedges(bb);
		bb->add_outedge(cfg->exit());
	}

	straighten(*cfg);
	prune(*cfg);
	return cfg;
}

static void
convert_function(llvm::Function & function, context & ctx)
{
	if (function.isDeclaration())
		return;

	auto fv = static_cast<const fctvariable*>(ctx.lookup_value(&function));

	ctx.set_node(fv->function());
	fv->function()->add_cfg(create_cfg(function, ctx));
	ctx.set_node(nullptr);
}

static const jlm::linkage &
convert_linkage(const llvm::GlobalValue::LinkageTypes & linkage)
{
	static std::unordered_map<llvm::GlobalValue::LinkageTypes, jlm::linkage> map({
	  {llvm::GlobalValue::ExternalLinkage, jlm::linkage::external_linkage}
	, {llvm::GlobalValue::AvailableExternallyLinkage, jlm::linkage::available_externally_linkage}
	, {llvm::GlobalValue::LinkOnceAnyLinkage, jlm::linkage::link_once_any_linkage}
	, {llvm::GlobalValue::LinkOnceODRLinkage, jlm::linkage::link_once_odr_linkage}
	, {llvm::GlobalValue::WeakAnyLinkage, jlm::linkage::weak_any_linkage}
	, {llvm::GlobalValue::WeakODRLinkage, jlm::linkage::weak_odr_linkage}
	, {llvm::GlobalValue::AppendingLinkage, jlm::linkage::appending_linkage}
	, {llvm::GlobalValue::InternalLinkage, jlm::linkage::internal_linkage}
	, {llvm::GlobalValue::PrivateLinkage, jlm::linkage::private_linkage}
	, {llvm::GlobalValue::ExternalWeakLinkage, jlm::linkage::external_weak_linkage}
	, {llvm::GlobalValue::CommonLinkage, jlm::linkage::common_linkage}
	});

	JIVE_DEBUG_ASSERT(map.find(linkage) != map.end());
	return map[linkage];
}

static void
declare_globals(llvm::Module & lm, context & ctx)
{
	auto & jm = ctx.module();

	/* forward declare global variables */
	for (auto & gv : lm.getGlobalList()) {
		auto name = gv.getName().str();
		auto constant = gv.isConstant();
		auto type = convert_type(gv.getType(), ctx);
		auto linkage = convert_linkage(gv.getLinkage());

		auto node = data_node::create(jm.ipgraph(), name, *type, linkage, constant);
		auto v = jm.create_global_value(node);
		ctx.insert_value(&gv, v);
	}

	/* forward declare functions */
	for (const auto & f : lm.getFunctionList()) {
		auto name = f.getName().str();
		auto linkage = convert_linkage(f.getLinkage());
		jive::fcttype fcttype(*convert_type(f.getFunctionType(), ctx));

		auto n = function_node::create(jm.ipgraph(), name, fcttype, linkage);
		ctx.insert_value(&f, ctx.module().create_variable(n));
	}
}

static std::unique_ptr<data_node_init>
create_initialization(llvm::GlobalVariable & gv, context & ctx)
{
	if (!gv.hasInitializer())
		return nullptr;

	auto init = gv.getInitializer();
	auto tacs = convert_constant(init, ctx);
	if (tacs.empty())
		return std::make_unique<data_node_init>(ctx.lookup_value(init));

	return std::make_unique<data_node_init>(std::move(tacs));
}

static void
convert_global_value(llvm::GlobalVariable & gv, context & ctx)
{
	auto v = static_cast<const gblvalue*>(ctx.lookup_value(&gv));

	ctx.set_node(v->node());
	v->node()->set_initialization(create_initialization(gv, ctx));
	ctx.set_node(nullptr);
}

static void
convert_globals(llvm::Module & lm, context & ctx)
{
	for (auto & gv : lm.getGlobalList())
		convert_global_value(gv, ctx);

	for (auto & f : lm.getFunctionList())
		convert_function(f, ctx);
}

std::unique_ptr<ipgraph_module>
convert_module(llvm::Module & m)
{
	filepath fp(m.getSourceFileName());
	auto im = ipgraph_module::create(fp, m.getTargetTriple(), m.getDataLayoutStr());

	context ctx(*im);
	declare_globals(m, ctx);
	convert_globals(m, ctx);

	return im;
}

}
