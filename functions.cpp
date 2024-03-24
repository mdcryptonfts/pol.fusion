#pragma once

void polcontract::add_liquidity(){
  state s = state_s.get();

  /* only deposit once every 24h */
  if( s.last_liquidity_addition_time > now() - (60 * 60 * 24) ) return;
  if( s.wax_bucket == ZERO_WAX || s.lswax_bucket == ZERO_LSWAX ) return;

  s.last_liquidity_addition_time = now();

  /* TODO: create mainnet pool and change poolId for mainnet below */

  uint64_t poolId = TESTNET ? 2 : 69420;
  auto itr = pools_t.require_find( poolId, ("could not locate pool id " + std::to_string(poolId) ).c_str() );

  int64_t pool_wax = itr->tokenA.quantity.amount;
  int64_t pool_lswax = itr->tokenB.quantity.amount;

  double pool_ratio;  

  if(pool_wax == 0 && pool_lswax == 0){
    pool_ratio = (double) 1;
  } else {
    pool_ratio = (double)pool_wax / (double)pool_lswax;
  }

  //Maximum possible based on buckets
  double max_wax_possible = (double) s.wax_bucket.amount / pool_ratio;
  double max_lswax_possible = (double) s.lswax_bucket.amount;

  //Determine the limiting factor
  double wax_to_add, lswax_to_add;
  if (max_wax_possible <= max_lswax_possible) {
      //WAX is the limiting factor
      wax_to_add = (double) s.wax_bucket.amount;
      lswax_to_add = wax_to_add / pool_ratio;
  } else {
      //LSWAX is the limiting factor
      lswax_to_add = (double) s.lswax_bucket.amount;
      wax_to_add = lswax_to_add * pool_ratio;
  }

  eosio::asset wax_to_add_asset = asset((int64_t)wax_to_add, WAX_SYMBOL);
  double tokenAMinDouble = wax_to_add * 0.99;
  eosio::asset tokenAMinAsset = asset((int64_t) tokenAMinDouble, WAX_SYMBOL);

  eosio::asset lswax_to_add_asset = asset((int64_t)lswax_to_add, LSWAX_SYMBOL);
  double tokenBMinDouble = lswax_to_add * 0.99;
  eosio::asset tokenBMinAsset = asset((int64_t) tokenBMinDouble, LSWAX_SYMBOL);


  //debit the amounts from wax bucket and lswax bucket
  s.lswax_bucket.amount = safeSubInt64( s.lswax_bucket.amount, lswax_to_add_asset.amount );
  s.wax_bucket.amount = safeSubInt64( s.wax_bucket.amount, wax_to_add_asset.amount );

  state_s.set(s, _self);

  /**
  * call the necessary actions:
  * - transfer wax to alcor "deposit"
  * - transfer lswax to alcor "deposit" 
  * - alcor `addliquid`
  */

  transfer_tokens(ALCOR_CONTRACT, wax_to_add_asset, WAX_CONTRACT, std::string("deposit"));
  transfer_tokens(ALCOR_CONTRACT, lswax_to_add_asset, TOKEN_CONTRACT, std::string("deposit"));

  action(permission_level{get_self(), "active"_n}, ALCOR_CONTRACT,"addliquid"_n,
    
    std::tuple{ 
      poolId,
      _self,
      wax_to_add_asset,
      lswax_to_add_asset,
      (int32_t) -443580,
      (int32_t) 443580,
      tokenAMinAsset,
      tokenBMinAsset,
      (uint32_t) 0,
    }

  ).send();

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

void polcontract::transfer_tokens(const name& user, const asset& amount_to_send, const name& contract, const std::string& memo){
	action(permission_level{get_self(), "active"_n}, contract,"transfer"_n,std::tuple{ get_self(), user, amount_to_send, memo}).send();
	return;
}

void polcontract::update_state(){
	state s = state_s.get();

	if( now() >= s.next_day_end_time ){
		s.next_day_end_time += SECONDS_PER_DAY;
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