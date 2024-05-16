#pragma once

void polcontract::receive_wax_transfer(const name& from, const name& to, const asset& quantity, const std::string& memo){
	const name tkcontract = get_first_receiver();

    check( quantity.amount > 0, "Must redeem a positive quantity" );
    check( quantity.amount < MAX_ASSET_AMOUNT, "quantity too large" );

    if( from == get_self() || to != get_self() ){
    	return;
    }

    check( quantity.symbol == WAX_SYMBOL, "was expecting WAX token" );

    //redundant check which isnt necessary when not using catchall notification handler
    check( tkcontract == WAX_CONTRACT, "first receiver should be eosio.token" );

    update_state();

    if( memo == "pol allocation from waxfusion distribution" ){
        //do we need to check the sender to make sure its the dapp contract?

        state s = state_s.get();
        config2 c = config_s_2.get();

        int64_t liquidity_allocation = calculate_asset_share( quantity.amount, c.liquidity_allocation_1e6 );
        int64_t wax_bucket_allocation, buy_lswax_allocation = liquidity_allocation / 2;
        int64_t rental_pool_allocation = calculate_asset_share( quantity.amount, c.rental_pool_allocation_1e6 );

        validate_allocations( quantity.amount, buy_lswax_allocation, wax_bucket_allocation, rental_pool_allocation );

        s.wax_available_for_rentals.amount = safeAddInt64( s.wax_available_for_rentals.amount, rental_pool_allocation );
        s.wax_bucket.amount = safeAddInt64( s.wax_bucket.amount, wax_bucket_allocation );
        state_s.set(s, _self);

        transfer_tokens( DAPP_CONTRACT, asset( buy_lswax_allocation, WAX_SYMBOL), WAX_CONTRACT, std::string("wax_lswax_liquidity") );

        return;
    }

    if( memo == "voter pay" ){

    	check( from == "eosio.voters"_n, "voter pay must come from eosio.voters" );
    	transfer_tokens( DAPP_CONTRACT, quantity, WAX_CONTRACT, std::string("waxfusion_revenue") );

    	return;
    }

    if( memo == "unstake" ){

    	check( from == "eosio.stake"_n, "unstakes should come from eosio.stake" );

        state s = state_s.get();
        state2 s2 = state_s_2.get();

        s.wax_available_for_rentals.amount = safeAddInt64( s.wax_available_for_rentals.amount, quantity.amount );
        
        s2.pending_refunds.amount = safeSubInt64( s2.pending_refunds.amount, quantity.amount );

        state_s.set(s, _self);
        state_s_2.set(s2, _self);

    	return;    	
    }

    if( memo == "for liquidity only" ){
        /** 
        * this is the case for when 100% of the funds should be used for liquidity,
        * rather than the split used for waxfusion distributions
        * i.e. when wax team makes a deposit that should be used only for alcor
        */

        state s = state_s.get();

        //50% should go into the wax bucket, 50% for buying LSWAX (which gets sent back and added to LSWAX bucket)
        int64_t wax_bucket_allocation, buy_lswax_allocation = calculate_asset_share( quantity.amount, 50000000 );
        check( safeAddInt64(wax_bucket_allocation, buy_lswax_allocation) <= quantity.amount, "overallocation" );

        s.wax_bucket.amount = safeAddInt64( s.wax_bucket.amount, wax_bucket_allocation );
        state_s.set(s, _self);

        transfer_tokens( DAPP_CONTRACT, asset( buy_lswax_allocation, WAX_SYMBOL ), WAX_CONTRACT, std::string("wax_lswax_liquidity") );

        return;           
    }

    if( memo == "for staking pool only" ){
        /** 
        * this is the case for when 100% of the funds should be used for cpu rentals/staking,
        * rather than using a portion for alcor liquidity
        */

        state s = state_s.get();

        s.wax_available_for_rentals.amount = safeAddInt64( s.wax_available_for_rentals.amount, quantity.amount );
        state_s.set(s, _self);

        return;           
    }    

    std::vector<std::string> words = get_words(memo);

    if( words[1] == "rent_cpu" ){
        check( tkcontract == WAX_CONTRACT, "only WAX can be sent with this memo" );
        check( words.size() >= 5, "memo for rent_cpu operation is incomplete" );

        state s = state_s.get();
        state2 s2 = state_s_2.get();

        int64_t profit_made = quantity.amount;

        //memo should also include account to rent to 
        const eosio::name cpu_receiver = eosio::name( words[2] );

        //make sure a record for this combo already exists
        auto renter_receiver_idx = renters_t.get_index<"fromtocombo"_n>();
        const uint128_t renter_receiver_combo = mix64to128(from.value, cpu_receiver.value);

        auto itr = renter_receiver_idx.require_find(renter_receiver_combo, "you need to use the rentcpu action first");  

        const uint64_t days_to_rent = std::strtoull( words[3].c_str(), NULL, 0 );
        const uint64_t wax_amount_to_rent = safeMulUInt64( std::strtoull( words[4].c_str(), NULL, 0 ), (uint64_t) SCALE_FACTOR_1E8 );

        check( itr->expires == 0 && itr->amount_staked.amount == 0, "memo for increasing/extending should start with extend_rental or increase_rental" );
        check( days_to_rent >= ( MINIMUM_CPU_RENTAL_DAYS ), ( "minimum days to rent is " + std::to_string( MINIMUM_CPU_RENTAL_DAYS ) ).c_str() );
        check( days_to_rent <= MAXIMUM_CPU_RENTAL_DAYS, ( "maximum days to rent is " + std::to_string( MAXIMUM_CPU_RENTAL_DAYS ) ).c_str() );

        check( wax_amount_to_rent >= MINIMUM_WAX_TO_RENT, ( "minimum wax amount to rent is " + std::to_string( MINIMUM_WAX_TO_RENT ) ).c_str() );
        check( wax_amount_to_rent <= MAXIMUM_WAX_TO_RENT, ( "maximum wax amount to rent is " + std::to_string( MAXIMUM_WAX_TO_RENT ) ).c_str() );

        //make sure there is anough wax available for this rental
        check( s.wax_available_for_rentals.amount >= (int64_t) wax_amount_to_rent, "there is not enough wax in the rental pool to cover this rental" );

        //debit the wax from the rental pool
        s.wax_available_for_rentals.amount = safeSubInt64(s.wax_available_for_rentals.amount, (int64_t) wax_amount_to_rent);
        s2.wax_allocated_to_rentals.amount = safeAddInt64(s2.wax_allocated_to_rentals.amount, (int64_t) wax_amount_to_rent);


        int64_t amount_expected = cpu_rental_price( days_to_rent, s.cost_to_rent_1_wax.amount, (int64_t) wax_amount_to_rent );
        check( quantity.amount >= amount_expected, ("expected to receive " + asset( amount_expected, WAX_SYMBOL ).to_string() ).c_str() );

        //if they sent more than expected, calculate difference and refund it
        if( quantity.amount > amount_expected ){
            int64_t amount_to_refund = safeSubInt64( quantity.amount, amount_expected );

            if( amount_to_refund > 0 ){
                profit_made = safeSubInt64( quantity.amount, amount_to_refund );
                transfer_tokens( from, asset( amount_to_refund, WAX_SYMBOL ), WAX_CONTRACT, "cpu rental refund from waxfusion.io - liquid staking protocol" );
            }
        }        

        //stake cpu to the receiver
        action(permission_level{get_self(), "active"_n}, "eosio"_n,"delegatebw"_n,std::tuple{ get_self(), cpu_receiver, asset( 0, WAX_SYMBOL ), asset( (int64_t) wax_amount_to_rent, WAX_SYMBOL ), false}).send();

        //update the renter row (amount staked and expires)
        renter_receiver_idx.modify(itr, same_payer, [&](auto &_r){
            _r.amount_staked.amount = (int64_t) wax_amount_to_rent;
            _r.expires = s.next_day_end_time + days_to_seconds( days_to_rent );
        });

        check( profit_made > 0, "error with rental cost calculation" );
        transfer_tokens( DAPP_CONTRACT, asset( profit_made, WAX_SYMBOL ), WAX_CONTRACT, "waxfusion_revenue" );


        //update the state
        state_s.set(s, _self);
        state_s_2.set(s2, _self);
        update_votes();
        return;
    }    

    if( words[1] == "extend_rental" ){
        check( tkcontract == WAX_CONTRACT, "only WAX can be sent with this memo" );
        check( words.size() >= 4, "memo for extend_rental operation is incomplete" );

        state s = state_s.get();

        int64_t profit_made = quantity.amount;

        //memo should also include account to rent to 
        const eosio::name cpu_receiver = eosio::name( words[2] );

        //make sure a record for this combo already exists
        auto renter_receiver_idx = renters_t.get_index<"fromtocombo"_n>();
        const uint128_t renter_receiver_combo = mix64to128(from.value, cpu_receiver.value);

        auto itr = renter_receiver_idx.require_find(renter_receiver_combo, "could not locate an existing rental for this renter/receiver combo");  

        const uint64_t days_to_rent = std::strtoull( words[3].c_str(), NULL, 0 );
        //const uint64_t wax_amount_to_rent = std::strtoull( words[4].c_str(), NULL, 0 );
        const int64_t wax_amount_to_rent = itr->amount_staked.amount;

        check( itr->expires != 0, "you can't extend a rental if it hasnt been funded yet" );
        check( itr->expires > now(), "you can't extend a rental after it expired" );
        check( days_to_rent >= 1, "extension must be at least 1 day" );
        check( days_to_rent <= MAXIMUM_CPU_RENTAL_DAYS, ( "maximum days to rent is " + std::to_string( MAXIMUM_CPU_RENTAL_DAYS ) ).c_str() );

        int64_t amount_expected = cpu_rental_price( days_to_rent, s.cost_to_rent_1_wax.amount, wax_amount_to_rent );

        check( quantity.amount >= amount_expected, ("expected to receive " + asset( amount_expected, WAX_SYMBOL ).to_string() ).c_str() );

        //if they sent more than expected, calculate difference and refund it
        if( quantity.amount > amount_expected ){
            int64_t amount_to_refund = safeSubInt64( quantity.amount, amount_expected );

            if( amount_to_refund > 0 ){
                profit_made = safeSubInt64( quantity.amount, amount_to_refund);
                transfer_tokens( from, asset( amount_to_refund, WAX_SYMBOL ), WAX_CONTRACT, "cpu rental refund from waxfusion.io - liquid staking protocol" );
            }
        }        

        //update the renter row (expires)
        renter_receiver_idx.modify(itr, same_payer, [&](auto &_r){
            _r.expires += days_to_seconds( days_to_rent );
        });

        check( profit_made > 0, "error with rental cost calculation" );
        transfer_tokens( DAPP_CONTRACT, asset( profit_made, WAX_SYMBOL ), WAX_CONTRACT, "waxfusion_revenue" );        

        //update the state
        state_s.set(s, _self);
        update_votes();
        return;
    } 

    if( words[1] == "increase_rental" ){
        check( tkcontract == WAX_CONTRACT, "only WAX can be sent with this memo" );
        state s = state_s.get();
        state2 s2 = state_s_2.get();
        check( words.size() >= 4, "memo for increase_rental operation is incomplete" );

        int64_t profit_made = quantity.amount;

        //memo should also include account to rent to 
        const eosio::name cpu_receiver = eosio::name( words[2] );

        //make sure a record for this combo already exists
        auto renter_receiver_idx = renters_t.get_index<"fromtocombo"_n>();
        const uint128_t renter_receiver_combo = mix64to128(from.value, cpu_receiver.value);

        auto itr = renter_receiver_idx.require_find(renter_receiver_combo, "could not locate an existing rental for this renter/receiver combo");  

        const uint64_t wax_amount_to_rent = safeMulUInt64( std::strtoull( words[3].c_str(), NULL, 0 ), (uint64_t) SCALE_FACTOR_1E8 );

        check( itr->expires > now(), "this rental has already expired" );
        uint64_t seconds_remaining = itr->expires - now();

        check( itr->expires != 0, "you can't increase a rental if it hasnt been funded yet" );
        check( seconds_remaining >= SECONDS_PER_DAY, "extension must be at least 1 day" );

        check( wax_amount_to_rent >= MINIMUM_WAX_TO_INCREASE, ( "minimum wax amount to increase is " + std::to_string( MINIMUM_WAX_TO_INCREASE ) ).c_str() );
        
        int64_t existing_rental_amount = itr->amount_staked.amount;

        check( safeAddInt64( (int64_t) wax_amount_to_rent, (int64_t) existing_rental_amount ) <= (int64_t) MAXIMUM_WAX_TO_RENT, ( "maximum wax amount to rent is " + std::to_string( MAXIMUM_WAX_TO_RENT ) ).c_str() );

        check( s.wax_available_for_rentals.amount >= (int64_t) wax_amount_to_rent, "there is not enough wax in the rental pool to cover this rental" );

        //debit the wax from the rental pool
        s.wax_available_for_rentals.amount = safeSubInt64( s.wax_available_for_rentals.amount, (int64_t) wax_amount_to_rent );
        s2.wax_allocated_to_rentals.amount = safeAddInt64( s2.wax_allocated_to_rentals.amount, (int64_t) wax_amount_to_rent );

        int64_t amount_expected = cpu_rental_price_from_seconds( seconds_remaining, s.cost_to_rent_1_wax.amount, wax_amount_to_rent );

        check( quantity.amount >= amount_expected, ("expected to receive " + asset( amount_expected, WAX_SYMBOL ).to_string() ).c_str() );

        //if they sent more than expected, calculate difference and refund it
        if( quantity.amount > amount_expected ){
            int64_t amount_to_refund = safeSubInt64( quantity.amount, amount_expected );

            if( amount_to_refund > 0 ){
                profit_made = safeSubInt64( quantity.amount, amount_to_refund );
                transfer_tokens( from, asset( amount_to_refund, WAX_SYMBOL ), WAX_CONTRACT, "cpu rental refund from waxfusion.io - liquid staking protocol" );
            }
        }

        //stake cpu to the receiver
        action(permission_level{get_self(), "active"_n}, "eosio"_n,"delegatebw"_n,std::tuple{ get_self(), cpu_receiver, asset( 0, WAX_SYMBOL ), asset( (int64_t) wax_amount_to_rent, WAX_SYMBOL ), false}).send();

        //update the renter row (amount staked and expires)
        renter_receiver_idx.modify(itr, same_payer, [&](auto &_r){
            _r.amount_staked.amount = safeAddInt64( _r.amount_staked.amount, (int64_t) wax_amount_to_rent );
        });

        check( profit_made > 0, "error with rental cost calculation" );
        transfer_tokens( DAPP_CONTRACT, asset( profit_made, WAX_SYMBOL ), WAX_CONTRACT, "waxfusion_revenue" );                

        //update the state
        state_s.set(s, _self);
        state_s_2.set(s2, _self);
        update_votes();
        return;
    }

    //below is handling for any cases where the memo has not been caught yet
    //need to be 100% sure that each case above has a return statement or else this will cause issues
    //anything below here should be fair game to add to the wax bucket

    state s = state_s.get();
    s.wax_bucket.amount = safeAddInt64( s.wax_bucket.amount, quantity.amount );
    state_s.set(s, _self);    
}

void polcontract::receive_lswax_transfer(const name& from, const name& to, const asset& quantity, const std::string& memo){
    const name tkcontract = get_first_receiver();

    check( quantity.amount > 0, "Must redeem a positive quantity" );
    check( quantity.amount < MAX_ASSET_AMOUNT, "quantity too large" );

    if( from == get_self() || to != get_self() ){
        return;
    }

    check( quantity.symbol == LSWAX_SYMBOL, "was expecting LSWAX token" );

    //redundant check which isnt necessary when not using catchall notification handler
    check( tkcontract == TOKEN_CONTRACT, "first receiver should be token.fusion" );

    update_state();

    /** 
    * all LSWAX we receive should be fair game to add to the bucket, unless we come up 
    * with other reasons later
    */

    state s = state_s.get();
    s.lswax_bucket.amount = safeAddInt64( s.lswax_bucket.amount, quantity.amount );
    state_s.set(s, _self);

}