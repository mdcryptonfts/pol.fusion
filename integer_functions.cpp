#pragma once

// determine how much of an asset x% is = to
int64_t polcontract::calculate_asset_share(const int64_t& quantity, const uint64_t& percentage){
	//percentage uses a scaling factor of 1e6
	//0.01% = 10,000
	//0.1% = 100,000
	//1% = 1,000,000
	//10% = 10,000,000
	//100% = 100,000,000

	//formula is ( quantity * percentage ) / ( 100 * SCALE_FACTOR_1E6 )
	if(quantity == 0) return 0;

	uint128_t divisor = safeMulUInt128( uint128_t(quantity), uint128_t(percentage) );
	
	//since 100 * 1e6 = 1e8, we will just / SCALE_FACTOR_1E8 here to avoid extra unnecessary math
	uint128_t result_128 = divisor / SCALE_FACTOR_1E8;

  return int64_t(result_128);
}

// determine how much of an e18 amount x% is = to
uint128_t polcontract::calculate_share_from_e18(const uint128_t& amount, const uint64_t& percentage){
	//percentage uses a scaling factor of 1e6
	//0.01% = 10,000
	//0.1% = 100,000
	//1% = 1,000,000
	//10% = 10,000,000
	//100% = 100,000,000

	//formula is ( amount * percentage ) / ( 100 * SCALE_FACTOR_1E6 )
	if(amount == 0) return 0;

	/** larger amounts might cause overflow when scaling too large, so we will scale
	 *  according to the size of the amount
	 */

	uint128_t scale_factor = max_scale_with_room( amount );
	uint128_t amount_scaled = safeMulUInt128( scale_factor, amount );

	uint128_t percentage_share = 
		safeMulUInt128( amount_scaled, uint128_t(percentage) ) / safeMulUInt128( scale_factor, SCALE_FACTOR_1E8 );

  return uint128_t(percentage_share);
}

int64_t polcontract::internal_liquify(const int64_t& quantity, dapp_tables::state s){
	//contract should have already validated quantity before calling this

    /** need to account for initial period where the values are still 0
     *  also if lswax has not compounded yet, the result will be 1:1
     */
	
    if( (s.liquified_swax.amount == 0 && s.swax_currently_backing_lswax.amount == 0)
    	||
    	(s.liquified_swax.amount == s.swax_currently_backing_lswax.amount)
     ){
      return quantity;
    } else {
      	uint128_t result_128 = safeMulUInt128( uint128_t(s.liquified_swax.amount), uint128_t(quantity) / uint128_t(s.swax_currently_backing_lswax.amount) );
      	return int64_t(result_128);
    }		
}

int64_t polcontract::internal_unliquify(const int64_t& quantity, dapp_tables::state s){
//contract should have already validated quantity before calling this

  uint128_t result_128 = safeMulUInt128( uint128_t(s.swax_currently_backing_lswax.amount), uint128_t(quantity) ) / uint128_t(s.liquified_swax.amount);
  return int64_t(result_128);  
} 

/** pool_ratio_1e18
 *  returns a scaled ratio of the wax/lswax pool on alcor
 *  needs to be scaled back down later before sending/storing assets
 * 	if this wasn't scaled, wax_amount < lswax_amount would result in 0
 */

uint128_t polcontract::pool_ratio_1e18(const int64_t& wax_amount, const int64_t& lswax_amount){
	uint128_t wax_amount_1e18 = safeMulUInt128( uint128_t(wax_amount), SCALE_FACTOR_1E18 );

	if( wax_amount == lswax_amount ){
		return SCALE_FACTOR_1E18;
	}

	check( wax_amount_1e18 > uint128_t(lswax_amount), "ratio out of bounds" );

	return wax_amount_1e18 / uint128_t(lswax_amount);
}

