#pragma once

void polcontract::receive_wax_transfer(const name& from, const name& to, const asset& quantity, const std::string& memo){
	const name tkcontract = get_first_receiver();

    check( quantity.amount > 0, "Must send a positive quantity" );

    check( quantity.amount < MAX_ASSET_AMOUNT, "quantity too large" );

    if( from == get_self() || to != get_self() ){
    	return;
    }

    check( quantity.symbol == WAX_SYMBOL, "was expecting WAX token" );

    update_state();

    /** pol allocation memo
     *  this catches any deposits of revenue from the dapp contract
     *  it will be split up according to the config, and the current state of the chain
     *  i.e. if the lswax price on alcor is too far from the "real" price,
     *  this case will be handled differently
     */

    if( memo == "pol allocation from waxfusion distribution" ){
        check( from == DAPP_CONTRACT, "invalid sender for this memo" );

        state3 s = state_s_3.get();
        config2 c = config_s_2.get();
        dapp_tables::state ds = dapp_state_s.get();

        int64_t liquidity_allocation = calculate_asset_share( quantity.amount, c.liquidity_allocation_1e6 );
        int64_t rental_pool_allocation = calculate_asset_share( quantity.amount, c.rental_pool_allocation_1e6 );
        int64_t wax_bucket_allocation = 0;
        int64_t buy_lswax_allocation = 0;        

        liquidity_struct lp_details = get_liquidity_info( c, ds );

        if( lp_details.is_in_range ){

            uint128_t liquidity_allocation_e18 = safeMulUInt128( uint128_t(liquidity_allocation), SCALE_FACTOR_1E18 );

            //Divide liquidity_allocation_e18 by the sum of alcors_lswax_price and real_lswax_price
            uint128_t sum_of_alcor_and_real_price = lp_details.alcors_lswax_price + lp_details.real_lswax_price;
            uint128_t intermediate_result = liquidity_allocation_e18 / sum_of_alcor_and_real_price;

            //Multiply intermediate_result by real_lswax_price to determine how much wax to spend
            uint128_t lswax_alloc_128 = safeMulUInt128( intermediate_result, lp_details.real_lswax_price );

            buy_lswax_allocation = int64_t(lswax_alloc_128);
            wax_bucket_allocation = safeSubInt64( liquidity_allocation, buy_lswax_allocation );

            transfer_tokens( DAPP_CONTRACT, asset( buy_lswax_allocation, WAX_SYMBOL), WAX_CONTRACT, std::string("wax_lswax_liquidity") );
        } else {
            //alcor price is out of range so we will just store the wax in our bucket instead
            wax_bucket_allocation = liquidity_allocation;
        }

        validate_allocations( quantity.amount, buy_lswax_allocation, wax_bucket_allocation, rental_pool_allocation );

        s.wax_available_for_rentals.amount = safeAddInt64( s.wax_available_for_rentals.amount, rental_pool_allocation );
        s.wax_bucket.amount = safeAddInt64( s.wax_bucket.amount, wax_bucket_allocation );
        state_s_3.set(s, _self);


        return;
    }

    if( memo == "voter pay" ){
    	check( from == "eosio.voters"_n, "voter pay must come from eosio.voters" );
    	transfer_tokens( DAPP_CONTRACT, quantity, WAX_CONTRACT, std::string("waxfusion_revenue") );
    	return;
    }

    if( memo == "unstake" ){

    	check( from == "eosio.stake"_n, "unstakes should come from eosio.stake" );

        state3 s = state_s_3.get();

        s.wax_available_for_rentals.amount = safeAddInt64( s.wax_available_for_rentals.amount, quantity.amount );  
        s.pending_refunds.amount = safeSubInt64( s.pending_refunds.amount, quantity.amount );

        state_s_3.set(s, _self);

    	return;    	
    }

    if( memo == "rebalance" ){
        /** this is the case when 100% of this transfer should go directly into the wax bucket,
         *  to be used for liquidity after calling the rebalance action on this contract
         *  the add_liquidity function should be called directly in this handler
         *  only the dapp contract should send with this memo
         */

        check( from == DAPP_CONTRACT, ("only " + DAPP_CONTRACT.to_string() + " should send with this memo" ).c_str() );

        state3 s = state_s_3.get();
        config2 c = config_s_2.get();
        dapp_tables::state ds = dapp_state_s.get();

        s.wax_bucket.amount = safeAddInt64( s.wax_bucket.amount, quantity.amount );

        liquidity_struct lp_details = get_liquidity_info( c, ds );

        if( lp_details.is_in_range && s.lswax_bucket > ZERO_LSWAX ){
            add_liquidity( s, lp_details );
            s.last_liquidity_addition_time = now();
        }

        state_s_3.set(s, _self);

        return;         
    }

    if( memo == "for liquidity only" ){
        /** 
        * this is the case for when 100% of the funds should be used for liquidity,
        * rather than the split used for waxfusion distributions
        * i.e. when wax team makes a deposit that should be used only for alcor
        */

        state3 s = state_s_3.get();
        config2 c = config_s_2.get();
        dapp_tables::state ds = dapp_state_s.get();

        int64_t liquidity_allocation = quantity.amount;
        int64_t wax_bucket_allocation = 0;
        int64_t buy_lswax_allocation = 0;        

        liquidity_struct lp_details = get_liquidity_info( c, ds );

        if( lp_details.is_in_range ){

            uint128_t liquidity_allocation_e18 = safeMulUInt128( uint128_t(liquidity_allocation), SCALE_FACTOR_1E18 );

            //Divide liquidity_allocation_e18 by the sum of alcors_lswax_price and real_lswax_price
            uint128_t sum_of_alcor_and_real_price = lp_details.alcors_lswax_price + lp_details.real_lswax_price;
            uint128_t intermediate_result = liquidity_allocation_e18 / sum_of_alcor_and_real_price;

            //Multiply intermediate_result by real_lswax_price to determine how much wax to spend
            uint128_t lswax_alloc_128 = safeMulUInt128( intermediate_result, lp_details.real_lswax_price ) / SCALE_FACTOR_1E18;

            buy_lswax_allocation = int64_t(lswax_alloc_128);
            wax_bucket_allocation = safeSubInt64( liquidity_allocation, buy_lswax_allocation );

            transfer_tokens( DAPP_CONTRACT, asset( buy_lswax_allocation, WAX_SYMBOL), WAX_CONTRACT, std::string("wax_lswax_liquidity") );
        } else {
            //alcor price is out of range so we will just store the wax in our bucket instead
            wax_bucket_allocation = liquidity_allocation;
        }

        validate_allocations( quantity.amount, buy_lswax_allocation, wax_bucket_allocation, 0 );

        s.wax_bucket.amount = safeAddInt64( s.wax_bucket.amount, wax_bucket_allocation );
        state_s_3.set(s, _self);

        return;           
    }

    if( memo == "for staking pool only" ){
        /** 
        * this is the case for when 100% of the funds should be used for cpu rentals/staking,
        * rather than using a portion for alcor liquidity
        * this memo is used when the dapp contract sends wax back after we rebalance
        * it can also be used in other cases
        */

        state3 s = state_s_3.get();

        s.wax_available_for_rentals.amount = safeAddInt64( s.wax_available_for_rentals.amount, quantity.amount );
        state_s_3.set(s, _self);

        return;           
    }    

    std::vector<std::string> words = parse_memo( memo );

    if( words[1] == "rent_cpu" ){
        check( tkcontract == WAX_CONTRACT, "only WAX can be sent with this memo" );
        check( words.size() >= 5, "memo for rent_cpu operation is incomplete" );

        state3 s = state_s_3.get();

        int64_t profit_made = quantity.amount;

        //memo should also include account to rent to 
        const eosio::name cpu_receiver = eosio::name( words[2] );

        //make sure a record for this combo already exists
        auto renter_receiver_idx = renters_t.get_index<"fromtocombo"_n>();
        const uint128_t renter_receiver_combo = mix64to128(from.value, cpu_receiver.value);

        auto itr = renter_receiver_idx.require_find(renter_receiver_combo, "you need to use the rentcpu action first");  

        const uint64_t days_to_rent = std::strtoull( words[3].c_str(), NULL, 0 );

        /** wax_amount_to_rent
         *  only "whole numbers" of wax can be rented
         *  therefore the memo accepts an integer that isn't yet scaled
         *  i.e. '10' in the memo == 10.00000000 WAX
         *  we need to scale by 1e8 to get the correct number to create an asset
         */

        const uint64_t wax_amount_to_rent = safeMulUInt64( std::strtoull( words[4].c_str(), NULL, 0 ), uint64_t(SCALE_FACTOR_1E8) );

        check( itr->expires == 0 && itr->amount_staked.amount == 0, "memo for increasing/extending should start with extend_rental or increase_rental" );
        check( days_to_rent >= MINIMUM_CPU_RENTAL_DAYS, ( "minimum days to rent is " + std::to_string( MINIMUM_CPU_RENTAL_DAYS ) ).c_str() );
        check( days_to_rent <= MAXIMUM_CPU_RENTAL_DAYS, ( "maximum days to rent is " + std::to_string( MAXIMUM_CPU_RENTAL_DAYS ) ).c_str() );

        check( wax_amount_to_rent >= MINIMUM_WAX_TO_RENT, ( "minimum wax amount to rent is " + asset( int64_t(MINIMUM_WAX_TO_RENT), WAX_SYMBOL ).to_string() ).c_str() );
        check( wax_amount_to_rent <= MAXIMUM_WAX_TO_RENT, ( "maximum wax amount to rent is " + asset( int64_t(MAXIMUM_WAX_TO_RENT), WAX_SYMBOL ).to_string() ).c_str() );

        //make sure there is anough wax available for this rental
        check( s.wax_available_for_rentals.amount >= int64_t(wax_amount_to_rent), "there is not enough wax in the rental pool to cover this rental" );

        //debit the wax from the rental pool
        s.wax_available_for_rentals.amount = safeSubInt64( s.wax_available_for_rentals.amount, int64_t(wax_amount_to_rent) );
        s.wax_allocated_to_rentals.amount = safeAddInt64( s.wax_allocated_to_rentals.amount, int64_t(wax_amount_to_rent) );

        int64_t amount_expected = cpu_rental_price( days_to_rent, s.cost_to_rent_1_wax.amount, int64_t(wax_amount_to_rent) );
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
        action(permission_level{get_self(), "active"_n}, "eosio"_n,"delegatebw"_n,std::tuple{ get_self(), cpu_receiver, asset( 0, WAX_SYMBOL ), asset( int64_t(wax_amount_to_rent), WAX_SYMBOL ), false}).send();

        //update the renter row (amount staked and expires)
        renter_receiver_idx.modify(itr, same_payer, [&](auto &_r){
            _r.amount_staked.amount = int64_t(wax_amount_to_rent);
            _r.expires = s.next_day_end_time + days_to_seconds( days_to_rent );
        });

        check( profit_made > 0, "error with rental cost calculation" );
        transfer_tokens( DAPP_CONTRACT, asset( profit_made, WAX_SYMBOL ), WAX_CONTRACT, "waxfusion_revenue" );

        //update the state
        state_s_3.set(s, _self);
        update_votes();
        return;
    }    

    if( words[1] == "extend_rental" ){
        check( tkcontract == WAX_CONTRACT, "only WAX can be sent with this memo" );
        check( words.size() >= 4, "memo for extend_rental operation is incomplete" );

        state3 s = state_s_3.get();

        int64_t profit_made = quantity.amount;

        //memo should also include account to rent to 
        const eosio::name cpu_receiver = eosio::name( words[2] );

        //make sure a record for this combo already exists
        auto renter_receiver_idx = renters_t.get_index<"fromtocombo"_n>();
        const uint128_t renter_receiver_combo = mix64to128(from.value, cpu_receiver.value);

        auto itr = renter_receiver_idx.require_find(renter_receiver_combo, "could not locate an existing rental for this renter/receiver combo");  

        const uint64_t days_to_rent = std::strtoull( words[3].c_str(), NULL, 0 );
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
        state_s_3.set(s, _self);
        update_votes();
        return;
    } 

    if( words[1] == "increase_rental" ){
        check( tkcontract == WAX_CONTRACT, "only WAX can be sent with this memo" );
        state3 s = state_s_3.get();
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

        check( safeAddInt64( int64_t(wax_amount_to_rent), int64_t(existing_rental_amount) ) <= int64_t(MAXIMUM_WAX_TO_RENT), ( "maximum wax amount to rent is " + std::to_string( MAXIMUM_WAX_TO_RENT ) ).c_str() );

        check( s.wax_available_for_rentals.amount >= int64_t(wax_amount_to_rent), "there is not enough wax in the rental pool to cover this rental" );

        //debit the wax from the rental pool
        s.wax_available_for_rentals.amount = safeSubInt64( s.wax_available_for_rentals.amount, int64_t(wax_amount_to_rent) );
        s.wax_allocated_to_rentals.amount = safeAddInt64( s.wax_allocated_to_rentals.amount, int64_t(wax_amount_to_rent) );

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
        action(permission_level{get_self(), "active"_n}, "eosio"_n,"delegatebw"_n,std::tuple{ get_self(), cpu_receiver, asset( 0, WAX_SYMBOL ), asset( int64_t(wax_amount_to_rent), WAX_SYMBOL ), false}).send();

        //update the renter row (amount staked and expires)
        renter_receiver_idx.modify(itr, same_payer, [&](auto &_r){
            _r.amount_staked.amount = safeAddInt64( _r.amount_staked.amount, int64_t(wax_amount_to_rent) );
        });

        check( profit_made > 0, "error with rental cost calculation" );
        transfer_tokens( DAPP_CONTRACT, asset( profit_made, WAX_SYMBOL ), WAX_CONTRACT, "waxfusion_revenue" );                

        //update the state
        state_s_3.set(s, _self);
        update_votes();
        return;
    }



    //below is handling for any cases where the memo has not been caught yet
    //need to be 100% sure that each case above has a return statement or else this will cause issues
    //anything below here should be fair game to add to the wax bucket

    state3 s = state_s_3.get();
    s.wax_bucket.amount = safeAddInt64( s.wax_bucket.amount, quantity.amount );
    state_s_3.set(s, _self);    
}

void polcontract::receive_lswax_transfer(const name& from, const name& to, const asset& quantity, const std::string& memo){
    const name tkcontract = get_first_receiver();

    check( quantity.amount > 0, "Must send a positive quantity" );
    check( quantity.amount < MAX_ASSET_AMOUNT, "quantity too large" );

    if( from == get_self() || to != get_self() ){
        return;
    }

    check( quantity.symbol == LSWAX_SYMBOL, "was expecting LSWAX token" );

    update_state();

    if( memo == "liquidity" && from == DAPP_CONTRACT ){

        state3 s = state_s_3.get();
        config2 c = config_s_2.get();
        dapp_tables::state ds = dapp_state_s.get();

        s.lswax_bucket.amount = safeAddInt64( s.lswax_bucket.amount, quantity.amount );

        liquidity_struct lp_details = get_liquidity_info( c, ds );

        if( lp_details.is_in_range && s.wax_bucket > ZERO_WAX ){
            add_liquidity( s, lp_details );
            s.last_liquidity_addition_time = now();
        }

        state_s_3.set(s, _self);

        return;
    }

    /** 
    * all LSWAX we receive should be fair game to add to the bucket, unless we come up 
    * with other reasons later
    */

    state3 s = state_s_3.get();
    s.lswax_bucket.amount = safeAddInt64( s.lswax_bucket.amount, quantity.amount );
    state_s_3.set(s, _self);

}