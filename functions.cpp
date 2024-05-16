#pragma once

void polcontract::add_liquidity(){
  state s = state_s.get();
  config2 c = config_s_2.get();

  /* only deposit once every 24h */
  if( s.last_liquidity_addition_time > now() - (60 * 60 * 24) ) return;
  if( s.wax_bucket == ZERO_WAX || s.lswax_bucket == ZERO_LSWAX ) return;

  s.last_liquidity_addition_time = now();

  /* TODO: create action setpoolid for mainnet below */

  uint64_t poolId = c.lswax_wax_pool_id;
  auto itr = pools_t.require_find( poolId, ("could not locate pool id " + std::to_string(poolId) ).c_str() );

  token_a_or_b poolA, poolB;

  bool aIsWax = (itr->tokenA.quantity.symbol == WAX_SYMBOL);
  poolA = aIsWax ? token_a_or_b{itr->tokenA.quantity.amount, WAX_SYMBOL, WAX_CONTRACT, ZERO_WAX, ZERO_WAX} : token_a_or_b{itr->tokenB.quantity.amount, LSWAX_SYMBOL, TOKEN_CONTRACT, ZERO_LSWAX, ZERO_LSWAX};
  poolB = !aIsWax ? token_a_or_b{itr->tokenA.quantity.amount, WAX_SYMBOL, WAX_CONTRACT, ZERO_WAX, ZERO_WAX} : token_a_or_b{itr->tokenB.quantity.amount, LSWAX_SYMBOL, TOKEN_CONTRACT, ZERO_LSWAX, ZERO_LSWAX};

  int64_t pool_wax = aIsWax ? poolA.quantity : poolB.quantity;
  int64_t pool_lswax = !aIsWax ? poolA.quantity : poolB.quantity;

  uint128_t pool_ratio = pool_ratio_1e18( pool_wax, pool_lswax );

  //Maximum possible based on buckets
  uint128_t max_wax_possible = safeMulUInt128( (uint128_t) s.wax_bucket.amount, SCALE_FACTOR_1E18 ) / pool_ratio;
  uint128_t max_lswax_possible = (uint128_t) s.lswax_bucket.amount;

  // Determine the limiting factor
  uint128_t wax_to_add, lswax_to_add;
  if (max_wax_possible <= max_lswax_possible) {
      // WAX is the limiting factor
      wax_to_add = (uint128_t) s.wax_bucket.amount;
      lswax_to_add = safeMulUInt128( wax_to_add, SCALE_FACTOR_1E18 ) / pool_ratio;
  } else {
      // LSWAX is the limiting factor
      lswax_to_add = (uint128_t) s.lswax_bucket.amount;
      wax_to_add = safeMulUInt128( lswax_to_add, pool_ratio ) / SCALE_FACTOR_1E18;
  }

  poolA.amountToAdd = aIsWax ? asset( (int64_t) wax_to_add, WAX_SYMBOL ) : asset( (int64_t) lswax_to_add, LSWAX_SYMBOL );
  poolB.amountToAdd = !aIsWax ? asset( (int64_t) wax_to_add, WAX_SYMBOL ) : asset( (int64_t) lswax_to_add, LSWAX_SYMBOL );

  poolA.minAsset = aIsWax ? asset( calculate_asset_share(wax_to_add, 99000000), WAX_SYMBOL ) : asset( calculate_asset_share(lswax_to_add, 99000000), LSWAX_SYMBOL );
  poolB.minAsset = !aIsWax ? asset( calculate_asset_share(wax_to_add, 99000000), WAX_SYMBOL ) : asset( calculate_asset_share(lswax_to_add, 99000000), LSWAX_SYMBOL );

  //debit the amounts from wax bucket and lswax bucket
  if(aIsWax){
    s.wax_bucket.amount = safeSubInt64( s.wax_bucket.amount, poolA.amountToAdd.amount );
    s.lswax_bucket.amount = safeSubInt64( s.lswax_bucket.amount, poolB.amountToAdd.amount );
  } else {
    s.wax_bucket.amount = safeSubInt64( s.wax_bucket.amount, poolB.amountToAdd.amount );
    s.lswax_bucket.amount = safeSubInt64( s.lswax_bucket.amount, poolA.amountToAdd.amount );
  }

  state_s.set(s, _self);

  /**
  * call the necessary actions:
  * - transfer wax to alcor "deposit"
  * - transfer lswax to alcor "deposit" 
  * - alcor `addliquid`
  */

  transfer_tokens(ALCOR_CONTRACT, poolA.amountToAdd, poolA.contract, std::string("deposit"));
  transfer_tokens(ALCOR_CONTRACT, poolB.amountToAdd, poolB.contract, std::string("deposit"));

  action(permission_level{get_self(), "active"_n}, ALCOR_CONTRACT,"addliquid"_n,
    
    std::tuple{ 
      poolId,
      _self,
      poolA.amountToAdd,
      poolB.amountToAdd,
      (int32_t) -443580,
      (int32_t) 443580,
      poolA.minAsset,
      poolB.minAsset,
      (uint32_t) 0,
    }

  ).send();

}

int64_t polcontract::cpu_rental_price(const uint64_t& days, const int64_t& price_per_day, const int64_t& amount){
  //formula is days * amount * price_per_day
  uint64_t daily_cost = safeMulUInt64( (uint64_t) amount, (uint64_t) price_per_day );
  uint128_t expected_amount = safeMulUInt128( (uint128_t) daily_cost, (uint128_t) days ) / SCALE_FACTOR_1E8;
  return (int64_t) expected_amount;
}

int64_t polcontract::cpu_rental_price_from_seconds(const uint64_t& seconds, const int64_t& price_per_day, const int64_t& amount){
  uint64_t daily_cost = safeMulUInt64( (uint64_t) amount, (uint64_t) price_per_day );
  uint128_t expected_amount = safeMulUInt128( (uint128_t) seconds, (uint128_t) daily_cost ) / safeMulUInt128( SECONDS_PER_DAY, SCALE_FACTOR_1E8 );
  return (int64_t) expected_amount;
}

uint64_t polcontract::days_to_seconds(const uint64_t& days){
  return (uint64_t) SECONDS_PER_DAY * days;
}

std::vector<std::string> polcontract::get_words(std::string memo){
  std::string delim = "|";
  std::vector<std::string> words{};
  size_t pos = 0;
  while ((pos = memo.find(delim)) != std::string::npos) {
    words.push_back(memo.substr(0, pos));
    memo.erase(0, pos + delim.length());
  }
  return words;
}

uint64_t polcontract::now(){
  return current_time_point().sec_since_epoch();
}

uint128_t polcontract::seconds_to_days_1e6(const uint64_t& seconds){
  uint128_t seconds_1e6 = safeMulUInt128( (uint128_t) seconds, SCALE_FACTOR_1E6 );
  return (uint128_t) seconds_1e6 / (uint128_t) SECONDS_PER_DAY;
}

void polcontract::transfer_tokens(const name& user, const asset& amount_to_send, const name& contract, const std::string& memo){
	action(permission_level{get_self(), "active"_n}, contract,"transfer"_n,std::tuple{ get_self(), user, amount_to_send, memo}).send();
	return;
}

void polcontract::update_state(){
	state s = state_s.get();

	if( now() >= s.next_day_end_time ){
		s.next_day_end_time += SECONDS_PER_DAY;
    state_s.set(s, _self);
	}
}

void polcontract::update_votes(){
  state s = state_s.get();

  if(s.last_vote_time + (60 * 60 * 24) <= now()){
    top21 t = top21_s.get();

    const eosio::name no_proxy;
    action(permission_level{get_self(), "active"_n}, "eosio"_n,"voteproducer"_n,std::tuple{ get_self(), no_proxy, t.block_producers }).send();

    s.last_vote_time = now();
    state_s.set(s, _self);
  }

  return;
}

void polcontract::validate_allocations( const int64_t& quantity, const int64_t& lswax_alloc, const int64_t& wax_alloc, const int64_t& rental_alloc ){
    int64_t check_1 = safeAddInt64( lswax_alloc, wax_alloc );
    int64_t check_2 = safeAddInt64( check_1, rental_alloc );
    check( check_2 <= quantity, "overallocation of funds" );
    return;
}