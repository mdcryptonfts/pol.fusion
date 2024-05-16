#pragma once

int64_t polcontract::calculate_asset_share(const int64_t& quantity, const uint64_t& percentage){
	//percentage uses a scaling factor of 1e6
	//0.01% = 10,000
	//0.1% = 100,000
	//1% = 1,000,000
	//10% = 10,000,000
	//100% = 100,000,000

	//formula is ( quantity * percentage ) / ( 100 * SCALE_FACTOR_1E6 )
	if(quantity == 0) return 0;

	uint128_t result_128 = safeMulUInt128( (uint128_t) quantity, (uint128_t) percentage ) / safeMulUInt128( (uint128_t) 100, SCALE_FACTOR_1E6 );

  	return (int64_t) result_128;
}

/** pool_ratio_1e18
 *  returns a scaled ratio of the wax/lswax pool
 *  needs to be scaled back down later before sending/storing assets
 * 	if this wasn't scaled, wax_amount < lswax_amount would result in 0
 */

uint128_t polcontract::pool_ratio_1e18(const int64_t& wax_amount, const int64_t& lswax_amount){
	uint128_t wax_amount_1e18 = safeMulUInt128( (uint128_t) wax_amount, SCALE_FACTOR_1E18 );

	if( wax_amount == lswax_amount ){
		return wax_amount_1e18;
	} 

	check( wax_amount_1e18 > (uint128_t) lswax_amount, "ratio out of bounds" );

	return wax_amount_1e18 / (uint128_t) lswax_amount;
}