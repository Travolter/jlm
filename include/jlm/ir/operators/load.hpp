/*
 * Copyright 2017 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_IR_OPERATORS_LOAD_HPP
#define JLM_IR_OPERATORS_LOAD_HPP

#include <jive/rvsdg/graph.h>
#include <jive/rvsdg/simple-normal-form.h>
#include <jive/rvsdg/simple-node.h>

#include <jlm/ir/tac.hpp>
#include <jlm/ir/types.hpp>

namespace jlm {

/* load normal form */

class load_normal_form final : public jive::simple_normal_form {
public:
	virtual
	~load_normal_form() noexcept;

	load_normal_form(
		const std::type_info & opclass,
		jive::node_normal_form * parent,
		jive::graph * graph) noexcept;

	virtual bool
	normalize_node(jive::node * node) const override;

	virtual std::vector<jive::output*>
	normalized_create(
		jive::region * region,
		const jive::simple_op & op,
		const std::vector<jive::output*> & operands) const override;

	inline void
	set_load_mux_reducible(bool enable) noexcept
	{
		enable_load_mux_ = enable;
	}

	inline bool
	get_load_mux_reducible() const noexcept
	{
		return enable_load_mux_;
	}

	inline void
	set_load_alloca_reducible(bool enable) noexcept
	{
		enable_load_alloca_ = enable;
	}

	inline bool
	get_load_alloca_reducible() const noexcept
	{
		return enable_load_alloca_;
	}

	inline void
	set_multiple_origin_reducible(bool enable) noexcept
	{
		enable_multiple_origin_ = enable;
	}

	inline bool
	get_multiple_origin_reducible() const noexcept
	{
		return enable_multiple_origin_;
	}

	inline void
	set_load_store_state_reducible(bool enable) noexcept
	{
		enable_load_store_state_ = enable;
	}

	inline bool
	get_load_store_state_reducible() const noexcept
	{
		return enable_load_store_state_;
	}

	inline void
	set_load_store_alloca_reducible(bool enable) noexcept
	{
		enable_load_store_alloca_ = enable;
	}

	inline bool
	get_load_store_alloca_reducible() const noexcept
	{
		return enable_load_store_alloca_;
	}

private:
	bool enable_load_mux_;
	bool enable_load_alloca_;
	bool enable_multiple_origin_;
	bool enable_load_store_state_;
	bool enable_load_store_alloca_;
};

/* load operator */

class load_op final : public jive::simple_op {
public:
	virtual
	~load_op() noexcept;

	inline
	load_op(
		const jlm::ptrtype & ptype,
		size_t nstates,
		size_t alignment)
	: simple_op()
	, nstates_(nstates)
	, aport_(ptype)
	, vport_(ptype.pointee_type())
	, alignment_(alignment)
	{}

	virtual bool
	operator==(const operation & other) const noexcept override;

	virtual size_t
	narguments() const noexcept override;

	virtual const jive::port &
	argument(size_t index) const noexcept override;

	virtual size_t
	nresults() const noexcept override;

	virtual const jive::port &
	result(size_t index) const noexcept override;

	virtual std::string
	debug_string() const override;

	virtual std::unique_ptr<jive::operation>
	copy() const override;

	inline const jive::valuetype &
	pointee_type() const noexcept
	{
		return *static_cast<const jive::valuetype*>(&vport_.type());
	}

	inline size_t
	nstates() const noexcept
	{
		return nstates_;
	}

	inline size_t
	alignment() const noexcept
	{
		return alignment_;
	}

	static jlm::load_normal_form *
	normal_form(jive::graph * graph) noexcept
	{
		return static_cast<jlm::load_normal_form*>(graph->node_normal_form(typeid(load_op)));
	}

private:
	size_t nstates_;
	jive::port aport_;
	jive::port vport_;
	size_t alignment_;
};

static inline bool
is_load_op(const jive::operation & op) noexcept
{
	return dynamic_cast<const jlm::load_op*>(&op) != nullptr;
}

static inline std::unique_ptr<jlm::tac>
create_load_tac(
	const variable * address,
	const variable * state,
	size_t alignment,
	jlm::variable * result)
{
	auto pt = dynamic_cast<const jlm::ptrtype*>(&address->type());
	if (!pt) throw std::logic_error("Expected pointer type.");

	jlm::load_op op(*pt, 1, alignment);
	return create_tac(op, {address, state}, {result});
}

static inline jive::output *
create_load(
	jive::output * address,
	const std::vector<jive::output*> & states,
	size_t alignment)
{
	auto pt = dynamic_cast<const jlm::ptrtype*>(&address->type());
	if (!pt) throw std::logic_error("Expected pointer type.");

	std::vector<jive::output*> operands({address});
	operands.insert(operands.end(), states.begin(), states.end());

	jlm::load_op op(*pt, states.size(), alignment);
	return jive::create_normalized(address->region(), op, operands)[0];
}

}

#endif
