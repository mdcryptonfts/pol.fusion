#pragma once

int64_t polcontract::safeAddInt64(const int64_t& a, const int64_t& b){
	const int64_t combinedValue = a + b;

	if(MAX_ASSET_AMOUNT - a < b){
		/** if the remainder is less than what we're adding, it means there 
		 *  will be overflow
		 */

		check(false, "overflow error");
	}	

	return combinedValue;
}

int64_t polcontract::safeDivInt64(const int64_t& a, const int64_t& b){
	if( b == 0 ){
		check( false, "cant divide by 0" );
	}

	return a / b;
}


uint128_t polcontract::safeMulUInt128(const uint128_t& a, const uint128_t& b){

    if( a == 0 || b == 0 ) return 0;

    if( a > (uint128_t) MAX_ASSET_AMOUNT_U64 || b > (uint128_t) MAX_ASSET_AMOUNT_U64 ){
    	check( false, "uint128_t multiplication input is outside of range" );
    }

    check( a <= MAX_U128_VALUE / b, "uint128_t multiplication would result in overflow" );

    uint128_t result = a * b;

    check( result <= MAX_U128_VALUE, "uint128_t multiplication result is outside of the acceptable range" );

    return result;
}

uint64_t polcontract::safeMulUInt64(const uint64_t& a, const uint64_t& b){

    if( a == 0 || b == 0 ) return 0;

    if( a > MAX_ASSET_AMOUNT_U64 || b > MAX_ASSET_AMOUNT_U64 ){
    	check( false, "multiplication input is outside of range" );
    }

    check( a <= MAX_ASSET_AMOUNT_U64 / b, "multiplication would result in overflow" );

    uint64_t result = a * b;

    check( result <= MAX_ASSET_AMOUNT_U64, "multiplication result is outside of the acceptable range" );

    return result;
}

int64_t polcontract::safeSubInt64(const int64_t& a, const int64_t& b){
	if(a == 0){
		check( b == 0, "subtraction would result in negative number" );
		return 0;
	}

	const int64_t remainder = a - b;

	if(b > a){
		check(false, "subtraction would result in negative number");
	}	

	return remainder;
}