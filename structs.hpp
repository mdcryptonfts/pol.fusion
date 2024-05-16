#pragma once

struct token_a_or_b {
	int64_t 		quantity;
	eosio::symbol  	symbol;
	eosio::name  	contract;
	eosio::asset  	minAsset;
	eosio::asset  	amountToAdd;
};