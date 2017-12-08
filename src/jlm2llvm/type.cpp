/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#include <jlm/jlm2llvm/context.hpp>
#include <jlm/jlm2llvm/type.hpp>

#include <llvm/IR/Module.h>

#include <typeindex>
#include <unordered_map>

namespace jlm {
namespace jlm2llvm {

static inline llvm::Type *
convert_integer_type(const jive::type & type, context & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::bits::type*>(&type));
	auto & t = *static_cast<const jive::bits::type*>(&type);

	return llvm::Type::getIntNTy(ctx.llvm_module().getContext(), t.nbits());
}

static inline llvm::Type *
convert_function_type(const jive::type & type, context & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::fct::type*>(&type));
	auto & t = *static_cast<const jive::fct::type*>(&type);
	auto & lctx = ctx.llvm_module().getContext();

	using namespace llvm;

	bool isvararg = false;
	std::vector<Type*> ats;
	for (size_t n = 0; n < t.narguments()-1; n++) {
		if (is_varargtype(t.argument_type(n))) {
			isvararg = true;
			continue;
		}
		ats.push_back(convert_type(t.argument_type(n), ctx));
	}

	JLM_DEBUG_ASSERT(t.nresults() == 1 || t.nresults() == 2);
	auto rt = t.nresults() == 1 ? llvm::Type::getVoidTy(lctx) : convert_type(t.result_type(0), ctx);

	return FunctionType::get(rt, ats, isvararg);
}

static inline llvm::Type *
convert_pointer_type(const jive::type & type, context & ctx)
{
	JLM_DEBUG_ASSERT(is_ptrtype(type));
	auto & t = *static_cast<const jlm::ptrtype*>(&type);

	return llvm::PointerType::get(convert_type(t.pointee_type(), ctx), 0);
}

static inline llvm::Type *
convert_array_type(const jive::type & type, context & ctx)
{
	JLM_DEBUG_ASSERT(is_arraytype(type));
	auto & t = *static_cast<const jlm::arraytype*>(&type);

	return llvm::ArrayType::get(convert_type(t.element_type(), ctx), t.nelements());
}

static inline llvm::Type *
convert_ctl_type(const jive::type & type, context & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jive::ctl::type*>(&type));
	auto & t = *static_cast<const jive::ctl::type*>(&type);
	JLM_DEBUG_ASSERT(t.nalternatives() == 2);

	return llvm::Type::getInt1Ty(ctx.llvm_module().getContext());
}

static inline llvm::Type *
convert_fp_type(const jive::type & type, context & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const jlm::fptype*>(&type));
	auto & t = *static_cast<const jlm::fptype*>(&type);

	static std::unordered_map<fpsize, llvm::Type*(*)(llvm::LLVMContext&)> map({
	  {fpsize::half, [](llvm::LLVMContext & ctx){ return llvm::Type::getHalfTy(ctx); }}
	, {fpsize::flt, [](llvm::LLVMContext & ctx){ return llvm::Type::getFloatTy(ctx); }}
	, {fpsize::dbl, [](llvm::LLVMContext & ctx){ return llvm::Type::getDoubleTy(ctx); }}
	});

	JLM_DEBUG_ASSERT(map.find(t.size()) != map.end());
	return map[t.size()](ctx.llvm_module().getContext());
}

static inline llvm::Type *
convert_struct_type(const jive::type & type, context & ctx)
{
	JLM_DEBUG_ASSERT(dynamic_cast<const structtype*>(&type));
	auto & t = *static_cast<const structtype*>(&type);

	auto st = ctx.structtype(t.declaration());
	if (st) return st;

	st = llvm::StructType::create(ctx.llvm_module().getContext(), {}, "", t.packed());
	if (t.has_name()) st->setName(t.name());
	ctx.add_structtype(t.declaration(), st);

	std::vector<llvm::Type*> elements;
	for (size_t n = 0; n < t.declaration()->nelements(); n++)
		elements.push_back(convert_type(t.declaration()->element(n), ctx));
	st->setBody(elements);

	return st;
}

llvm::Type *
convert_type(const jive::type & type, context & ctx)
{
	static std::unordered_map<
		std::type_index
	, std::function<llvm::Type*(const jive::type&, context&)>
	> map({
	  {std::type_index(typeid(jive::bits::type)), convert_integer_type}
	, {std::type_index(typeid(jive::fct::type)), convert_function_type}
	, {std::type_index(typeid(jlm::ptrtype)), convert_pointer_type}
	, {std::type_index(typeid(jlm::arraytype)), convert_array_type}
	, {std::type_index(typeid(jive::ctl::type)), convert_ctl_type}
	, {std::type_index(typeid(jlm::fptype)), convert_fp_type}
	, {std::type_index(typeid(structtype)), convert_struct_type}
	});

	JLM_DEBUG_ASSERT(map.find(std::type_index(typeid(type))) != map.end());
	return map[std::type_index(typeid(type))](type, ctx);
}

}}
