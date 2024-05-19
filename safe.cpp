#pragma once

int64_t polcontract::safeAddInt64(const int64_t& a, const int64_t& b){
	int64_t sum;

	if (((b > 0) && (a > ( std::numeric_limits<int64_t>::max() - b ))) ||
	  ((b < 0) && (a < ( std::numeric_limits<int64_t>::min() - b )))) {
		check(false, "int64 addition would result in overflow or underflow");
	} else {
		sum = a + b;
	}

	return sum;
}

int64_t polcontract::safeDivInt64(const int64_t& a, const int64_t& b){
	if( b == 0 ){
		check( false, "cant divide by 0" );
	}

	return a / b;
}


uint128_t polcontract::safeMulUInt128(const uint128_t& a, const uint128_t& b){

    if( a == 0 || b == 0 ) return 0;

    if( a > (uint128_t) MAX_U128_VALUE || b > (uint128_t) MAX_U128_VALUE ){
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
  int64_t diff;
  if ((b > 0 && a < std::numeric_limits<int64_t>::min() + b) ||
      (b < 0 && a > std::numeric_limits<int64_t>::max() + b)) {
    check(false, "subtraction would result in overflow or underflow");
  } else {
    diff = a - b;
  }	

  return diff;
}