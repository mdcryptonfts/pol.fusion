#pragma once

#include "polcontract.hpp"
#include "functions.cpp"
#include "integer_functions.cpp"
#include "on_notify.cpp"
#include "safe.cpp"

ACTION polcontract::addliquidity()
{
	update_state();
	add_liquidity();
}


ACTION polcontract::claimgbmvote()
{
	//Can be called by anyone
	update_state();
	action(permission_level{get_self(), "active"_n}, SYSTEM_CONTRACT,"claimgbmvote"_n,std::tuple{ get_self() }).send();
}

ACTION polcontract::claimrefund()
{
	//Can be called by anyone
	update_state();
	action(permission_level{get_self(), "active"_n}, SYSTEM_CONTRACT,"refund"_n,std::tuple{ get_self() }).send();
}

/**
* clearexpired
* unstakes and removes all expired rentals
* can be called by anyone
*/ 

ACTION polcontract::clearexpired(const int& limit)
{
	update_state();
	state2 s2 = state_s_2.get();

	//see if there is a pending refund for us in the refunds table
	auto refund_itr = refunds_t.find( get_self().value );

	if(refund_itr != refunds_t.end()){
		bool shouldContinue = false;
		/* the refund is less than 5 mins old, proceed */
		if( now() - (5 * 60) < (uint64_t) refund_itr->request_time.sec_since_epoch() ){
			shouldContinue = true;
		}

		/* the refund is > 3 days old, claim it and proceed */
		else if( now() - (60 * 60 * 24 * 3) > (uint64_t) refund_itr->request_time.sec_since_epoch()){
			action(permission_level{get_self(), "active"_n}, SYSTEM_CONTRACT,"refund"_n,std::tuple{ get_self() }).send();
			shouldContinue = true;
		}

		check( shouldContinue, "there is a pending refund > 5 mins and < 3 days old" );
	}

	//get expires index from renters table
	auto expires_idx = renters_t.get_index<"expires"_n>();

	//upper bound is 1 second ago, lower bound is 1 (to avoid paying for deletion of unfunded rentals)
	auto expires_lower = expires_idx.lower_bound( 1 );
	auto expires_upper = expires_idx.upper_bound( now() - 1 );

	int count = 0;

	/* new logic */
    auto itr = expires_lower;
    while (itr != expires_upper) {
    	if( count == limit ) break;
		if( itr->expires < now() && itr->expires != 0 ){
			s2.wax_allocated_to_rentals.amount = safeSubInt64( s2.wax_allocated_to_rentals.amount, itr->amount_staked.amount );
			s2.pending_refunds.amount = safeAddInt64( s2.pending_refunds.amount, itr->amount_staked.amount );

			//undelegatebw from the receiver
			action(permission_level{get_self(), "active"_n}, SYSTEM_CONTRACT,"undelegatebw"_n,std::tuple{ get_self(), itr->rent_to_account, ZERO_WAX, itr->amount_staked}).send();
			itr = expires_idx.erase( itr );
			count++;
		} else {
            break;
        }
    }

    check( count > 0, "no expired rentals to clear" );
    state_s_2.set(s2, _self);
}

ACTION polcontract::initconfig(){
	require_auth( _self );

	eosio::check( !config_s_2.exists(), "config2 already exists" );

	uint64_t rental_pool_allocation_1e6 = 14285714; //14.28% or 1/7th
	uint64_t liquidity_allocation_1e6 = ONE_HUNDRED_PERCENT_1E6 - rental_pool_allocation_1e6;

	config2 c{};
	c.liquidity_allocation_1e6 = liquidity_allocation_1e6;
	c.rental_pool_allocation_1e6 = rental_pool_allocation_1e6;
	c.lswax_wax_pool_id = TESTNET ? 2 : 9999999; //TODO change this when pair is created
	config_s_2.set(c, _self);
}

ACTION polcontract::initstate(){
	require_auth(_self);
	eosio::check(!state_s.exists(), "state already exists");

	state s{};
	s.wax_available_for_rentals = ZERO_WAX;
	s.next_day_end_time = now() + SECONDS_PER_DAY;
	s.cost_to_rent_1_wax = asset(1000000, WAX_SYMBOL); /* 0.01 WAX per day */
	s.last_vote_time = 0;
	s.wax_bucket = ZERO_WAX;
	s.lswax_bucket = ZERO_LSWAX;
	s.last_liquidity_addition_time = 0;
	state_s.set(s, _self);
}

ACTION polcontract::initstate2(){
	require_auth(_self);
	eosio::check(!state_s_2.exists(), "state already exists");

	eosio::asset wax_allocated_to_rentals = ZERO_WAX;

	for(auto itr = renters_t.begin(); itr != renters_t.end(); itr ++){
		wax_allocated_to_rentals.amount = safeAddInt64(wax_allocated_to_rentals.amount, itr->amount_staked.amount);
	}

	state2 s2{};
	s2.wax_allocated_to_rentals = wax_allocated_to_rentals;
	s2.pending_refunds = ZERO_WAX;
	state_s_2.set(s2, _self);
}

/**
* rebalance
* can be called by anyone
* if the lswax_bucket is empty, and the wax_bucket has more than 100 wax,
* that wax_bucket is just sitting there for no reason
* half of it should be converted to lsWAX for the next liquidity addition
*/

ACTION polcontract::rebalance(){
	update_state();
	state s = state_s.get();

	bool need_to_rebalance = false;

	if( s.wax_bucket > asset( 10000000000, WAX_SYMBOL) && s.lswax_bucket == ZERO_LSWAX ){
		need_to_rebalance = true;

        int64_t buy_lswax_allocation = calculate_asset_share( s.wax_bucket.amount, 50000000 );
        s.wax_bucket.amount = safeSubInt64( s.wax_bucket.amount, buy_lswax_allocation );
        state_s.set(s, _self);

        transfer_tokens( DAPP_CONTRACT, asset( buy_lswax_allocation, WAX_SYMBOL ), WAX_CONTRACT, std::string("wax_lswax_liquidity") );
	}

	if(!need_to_rebalance){
		check(false, "nothing to rebalance");
	}
}

/**
* rentcpu
* to announce a rental and create a table row if it doesnt exist yet
* user is the ram payer on the row
*/

ACTION polcontract::rentcpu(const eosio::name& renter, const eosio::name& cpu_receiver){
	require_auth(renter);
	update_state();

	check( is_account(cpu_receiver), (cpu_receiver.to_string() + " is not a valid account").c_str() );

	auto renter_receiver_idx = renters_t.get_index<"fromtocombo"_n>();
	const uint128_t renter_receiver_combo = mix64to128(renter.value, cpu_receiver.value);

	auto itr = renter_receiver_idx.find(renter_receiver_combo);	

	if(itr == renter_receiver_idx.end()){
		renters_t.emplace(renter, [&](auto &_r){
			_r.ID = renters_t.available_primary_key();
			_r.renter = renter;
			_r.rent_to_account = cpu_receiver;
			_r.amount_staked = ZERO_WAX;
			_r.expires = 0; 		
		});
	}
}

ACTION polcontract::setallocs(const uint64_t& liquidity_allocation_percent_1e6){
	require_auth( _self );
	eosio::check(config_s_2.exists(), "config2 doesnt exist");

	//allow 1-100%
	check( liquidity_allocation_percent_1e6 >= 1000000 && liquidity_allocation_percent_1e6 <= ONE_HUNDRED_PERCENT_1E6, "percent must be between > 1e6 && <= 100 * 1e6" );


	config2 c{};
	c.liquidity_allocation_1e6 = liquidity_allocation_percent_1e6;
	c.rental_pool_allocation_1e6 = ONE_HUNDRED_PERCENT_1E6 - liquidity_allocation_percent_1e6;
	config_s_2.set(c, _self);
}

ACTION polcontract::setrentprice(const eosio::asset& cost_to_rent_1_wax){
	require_auth( DAPP_CONTRACT );
	update_state();
	check( cost_to_rent_1_wax.amount > 0, "cost must be positive" );
	check( cost_to_rent_1_wax.symbol == WAX_SYMBOL, "symbol and precision must match WAX" );

	state s = state_s.get();
	s.cost_to_rent_1_wax = cost_to_rent_1_wax;
	state_s.set(s, _self);
}

/**
* sync
* updates the next_day_end_time in the state singleton 
* to make sure CPU rentals are timed properly 
*/ 

ACTION polcontract::sync(){
	update_state();
}