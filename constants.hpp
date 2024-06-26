#pragma once

//Numeric Limits
static constexpr int64_t MAX_ASSET_AMOUNT = 4611686018427387903;
static constexpr uint64_t MAX_ASSET_AMOUNT_U64 = 4611686018427387903;
static constexpr uint128_t MAX_U128_VALUE = (static_cast<uint128_t>(1) << 127) + ((static_cast<uint128_t>(1) << 127) - 1);

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
static constexpr uint64_t ONE_HUNDRED_PERCENT_1E6 = 100000000;
static constexpr uint64_t MAXIMUM_CPU_RENTAL_DAYS = 365 * 10; /* 10 years */
static constexpr uint64_t MAXIMUM_WAX_TO_RENT = 1000000000000000; /* 10 Million WAX */
static constexpr uint64_t MINIMUM_CPU_RENTAL_DAYS = TESTNET ? 1 : 30; /* 1 Month */
static constexpr uint64_t MINIMUM_WAX_TO_INCREASE = TESTNET ? 500000000 : 10000000000; /* 5 or 100 */
static constexpr uint64_t MINIMUM_WAX_TO_RENT = TESTNET ? 1000000000 : 50000000000; /* 10 or 500 */
static constexpr uint64_t SECONDS_PER_DAY = 86400;

//Scaling factors
static constexpr uint128_t SCALE_FACTOR_1E1 = 10;
static constexpr uint128_t SCALE_FACTOR_1E2 = 100;
static constexpr uint128_t SCALE_FACTOR_1E3 = 1000;
static constexpr uint128_t SCALE_FACTOR_1E4 = 10000;
static constexpr uint128_t SCALE_FACTOR_1E5 = 100000;
static constexpr uint128_t SCALE_FACTOR_1E6 = 1000000;
static constexpr uint128_t SCALE_FACTOR_1E7 = 10000000;
static constexpr uint128_t SCALE_FACTOR_1E8 = 100000000;
static constexpr uint128_t SCALE_FACTOR_1E9 = 1000000000;
static constexpr uint128_t SCALE_FACTOR_1E10 = 10000000000;
static constexpr uint128_t SCALE_FACTOR_1E11 = 100000000000;
static constexpr uint128_t SCALE_FACTOR_1E12 = 1000000000000;
static constexpr uint128_t SCALE_FACTOR_1E13 = 10000000000000;
static constexpr uint128_t SCALE_FACTOR_1E14 = 100000000000000;
static constexpr uint128_t SCALE_FACTOR_1E15 = 1000000000000000;
static constexpr uint128_t SCALE_FACTOR_1E16 = 10000000000000000;
static constexpr uint128_t SCALE_FACTOR_1E17 = 100000000000000000;
static constexpr uint128_t SCALE_FACTOR_1E18 = 1000000000000000000;
static constexpr uint128_t SCALE_FACTOR_1E19 = SCALE_FACTOR_1E18 * SCALE_FACTOR_1E1;
static constexpr uint128_t SCALE_FACTOR_1E20 = SCALE_FACTOR_1E18 * SCALE_FACTOR_1E2;
static constexpr uint128_t SCALE_FACTOR_1E21 = SCALE_FACTOR_1E18 * SCALE_FACTOR_1E3;
static constexpr uint128_t SCALE_FACTOR_1E22 = SCALE_FACTOR_1E18 * SCALE_FACTOR_1E4;
static constexpr uint128_t SCALE_FACTOR_1E23 = SCALE_FACTOR_1E18 * SCALE_FACTOR_1E5;
static constexpr uint128_t SCALE_FACTOR_1E24 = SCALE_FACTOR_1E18 * SCALE_FACTOR_1E6;
static constexpr uint128_t SCALE_FACTOR_1E25 = SCALE_FACTOR_1E18 * SCALE_FACTOR_1E7;
static constexpr uint128_t SCALE_FACTOR_1E26 = SCALE_FACTOR_1E18 * SCALE_FACTOR_1E8;
