#pragma once

#include "polcontract.hpp"
#include "functions.cpp"
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
			//undelegatebw from the receiver
			action(permission_level{get_self(), "active"_n}, SYSTEM_CONTRACT,"undelegatebw"_n,std::tuple{ get_self(), itr->rent_to_account, ZERO_WAX, itr->amount_staked}).send();
			itr = expires_idx.erase( itr );
			count++;
		} else {
            break;
        }
    }

    check( count > 0, "no expired rentals to clear" );
}

ACTION polcontract::initconfig(){
	require_auth(_self);
	eosio::check(!config_s.exists(), "config already exists");

	double one_seventh = safeDivDouble( (double) 1, (double) 7 );
	double liquidity_allocation = (double) 1 - one_seventh;

	config c{};
	c.liquidity_allocation = liquidity_allocation;
	c.rental_pool_allocation = one_seventh;
	config_s.set(c, _self);
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