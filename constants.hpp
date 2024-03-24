#pragma once

//Numeric Limits
static constexpr int64_t MAX_ASSET_AMOUNT = 4611686018427387903;

//Symbols
static constexpr eosio::symbol LSWAX_SYMBOL = eosio::symbol("LSWAX", 8);
static constexpr eosio::symbol WAX_SYMBOL = eosio::symbol("WAX", 8);

//Contract names
static constexpr eosio::name ALCOR_CONTRACT = "swap.alcor"_n;
static constexpr eosio::name DAPP_CONTRACT = "dapp.fusion"_n;
static constexpr eosio::name SYSTEM_CONTRACT = "eosio"_n;
static constexpr eosio::name TOKEN_CONTRACT = "token.fusion"_n;
static constexpr eosio::name WAX_CONTRACT = "eosio.token"_n;

//Assets
static const eosio::asset ZERO_LSWAX = eosio::asset(0, LSWAX_SYMBOL);
static const eosio::asset ZERO_WAX = eosio::asset(0, WAX_SYMBOL);

//Other
static constexpr uint64_t MAXIMUM_CPU_RENTAL_DAYS = 365 * 10; /* 10 years */
static constexpr uint64_t MAXIMUM_WAX_TO_RENT = 10000000; /* 10 Million WAX */
static constexpr uint64_t MINIMUM_CPU_RENTAL_DAYS = TESTNET ? 1 : 30; /* 1 Month */
static constexpr uint64_t MINIMUM_WAX_TO_INCREASE = 100;
static constexpr uint64_t MINIMUM_WAX_TO_RENT = TESTNET ? 10 : 500;
static constexpr uint64_t SECONDS_PER_DAY = 86400;