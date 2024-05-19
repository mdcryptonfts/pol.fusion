#pragma once

struct token_a_or_b {
	int64_t 		quantity;
	eosio::symbol  	symbol;
	eosio::name  	contract;
	eosio::asset  	minAsset;
	eosio::asset  	amountToAdd;
};

struct liquidity_struct {
	bool  			is_in_range;
	uint128_t  		alcors_lswax_price;
	uint128_t  		real_lswax_price;
	bool  			aIsWax;
	token_a_or_b 	poolA;
	token_a_or_b    poolB;
	uint64_t  		poolId;
};