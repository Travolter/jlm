/*
 * Copyright 2020 Nico Reißmann <nico.reissmann@gmail.com>
 * See COPYING for terms of redistribution.
 */

#ifndef JLM_OPT_ALIAS_ANALYSES_STEENSGAARD_HPP
#define JLM_OPT_ALIAS_ANALYSES_STEENSGAARD_HPP

#include <jlm/opt/optimization.hpp>
#include <jlm/util/disjointset.hpp>

#include <string>

namespace jive {
	class gamma_node;
	class graph;
	class output;
	class region;
	class simple_node;
	class structural_node;
	class theta_node;
}

namespace jlm {

class delta_node;
class lambda_node;

namespace aa {

class location;

/**
* FIXME: Some documentation
*/
class locationset final {
private:
	using locdjset = typename jlm::disjointset<jlm::aa::location*>;

public:
	using const_iterator = std::unordered_map<
	  const jive::output*
	, jlm::aa::location*
	>::const_iterator;

	~locationset();

	locationset();

	locationset(const locationset &) = delete;

	locationset(locationset &&) = delete;

	locationset &
	operator=(const locationset &) = delete;

	locationset &
	operator=(locationset &&) = delete;

	locdjset::set_iterator
	begin() const
	{
		return djset_.begin();
	}

	locdjset::set_iterator
	end() const
	{
		return djset_.end();
	}

/*
	jlm::aa::location *
	location(const jive::output * output)
	{
		auto it = map_.find(output);
		JLM_DEBUG_ASSERT(it != map_.end());
		return it->second;
	}
*/
	jlm::aa::location *
	insert(const jive::output * output);

	/*
		FIXME: make private
	*/
	jlm::aa::location *
	lookup(const jive::output * output);

	jlm::aa::location *
	find_or_insert(const jive::output * output);

	jlm::aa::location *
	find(jlm::aa::location * l) const;

	jlm::aa::location *
	find(const jive::output * output);

	jlm::aa::location *
	merge(jlm::aa::location * l1, jlm::aa::location * l2);
/*
	jlm::aa::location *
	merge(const jive::output * o1, const jive::output * o2);
*/
	std::string
	to_dot() const;

	void
	clear();

	jlm::aa::location *
	any() const noexcept
	{
		return any_;
	}

private:
	jlm::aa::location *
	create_any();

	jlm::aa::location * any_;
	disjointset<jlm::aa::location*> djset_;
	std::vector<std::unique_ptr<jlm::aa::location>> locations_;
	std::unordered_map<const jive::output*, jlm::aa::location*> map_;
};

/**
* \brief FIXME: some documentation
*/
class steensgaard final : public optimization {
public:
	virtual
	~steensgaard();

	steensgaard() = default;

	steensgaard(const steensgaard &) = delete;

	steensgaard(steensgaard &&) = delete;

	steensgaard &
	operator=(const steensgaard &) = delete;

	steensgaard &
	operator=(steensgaard &&) = delete;

	virtual void
	run(rvsdg_module & module, const stats_descriptor & sd) override;

private:
	void
	reset_state();

	void
	perform_aa(const jive::graph & graph);

	void
	perform_aa(jive::region & region);

	void
	perform_aa(const lambda_node & node);

	void
	perform_aa(const delta_node & node);

	void
	perform_aa(const jive::phi_node & node);

	void
	perform_aa(const jive::gamma_node & node);

	void
	perform_aa(const jive::theta_node & node);

	void
	perform_aa(const jive::simple_node & node);

	void
	perform_aa(const jive::structural_node & node);

	void
	perform_aa_alloca(const jive::simple_node & node);

	void
	perform_aa_load(const jive::simple_node & node);

	void
	perform_aa_store(const jive::simple_node & node);

	void
	perform_aa_call(const jive::simple_node & node);

	void
	perform_aa_gep(const jive::simple_node & node);

	void
	perform_aa_bitcast(const jive::simple_node & node);

	void
	perform_aa_bits2ptr(const jive::simple_node & node);

	void
	perform_aa_ptr_constant_null(const jive::simple_node & node);

	void
	perform_aa_undef(const jive::simple_node & node);

	/**
	* \brief Peform a recursive union of location \p x and \p y.
	*
	* FIXME
	*/
	location *
	join(location * x, location * y);

	locationset lset_;
};

}}

#endif
