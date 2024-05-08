#pragma once
#define CONTRACT_NAME "polcontract"
#define TESTNET true
#define mix64to128(a, b) (uint128_t(a) << 64 | uint128_t(b))


#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/symbol.hpp>
#include <eosio/action.hpp>
#include <string>
#include <eosio/crypto.hpp>
#include <eosio/transaction.hpp>
#include <eosio/singleton.hpp>
#include <cmath>
#include "tables.hpp"
#include "structs.hpp"
#include "constants.hpp"
#include <limits>

using namespace eosio;


CONTRACT polcontract : public contract {
	public:
		using contract::contract;
		polcontract(name receiver, name code, datastream<const char *> ds):
		contract(receiver, code, ds),
		state_s(receiver, receiver.value),
		top21_s(DAPP_CONTRACT, DAPP_CONTRACT.value)
		{}

		//Main Actions
		ACTION addliquidity();
		ACTION claimgbmvote();
		ACTION claimrefund();
		ACTION clearexpired(const int& limit);
		ACTION initstate();
		ACTION rentcpu(const eosio::name& renter, const eosio::name& cpu_receiver);
		ACTION setrentprice(const eosio::asset& cost_to_rent_1_wax);
		ACTION sync();

		//Notifications
		[[eosio::on_notify("eosio.token::transfer")]] void receive_wax_transfer(const name& from, const name& to, const asset& quantity, const std::string& memo);
		[[eosio::on_notify("token.fusion::transfer")]] void receive_lswax_transfer(const name& from, const name& to, const asset& quantity, const std::string& memo);


	private:

		//Singletons
		state_singleton state_s;
		top21_singleton top21_s;

		//Multi Index Tables
		alcor_contract::pools_table pools_t = alcor_contract::pools_table(ALCOR_CONTRACT, ALCOR_CONTRACT.value);
		refunds_table refunds_t = refunds_table(SYSTEM_CONTRACT, get_self().value);
		renters_table renters_t = renters_table(get_self(), get_self().value);


		//Functions
		void add_liquidity();
		std::vector<std::string> get_words(std::string memo);
		uint64_t now();
		void transfer_tokens(const name& user, const asset& amount_to_send, const name& contract, const std::string& memo);
		void update_state();
		void update_votes();

		//Safemath
		int64_t safeAddInt64(const int64_t& a, const int64_t& b);
		double safeDivDouble(const double& a, const double& b);
		double safeMulDouble(const double& a, const double& b);
		uint64_t safeMulUInt64(const uint64_t& a, const uint64_t& b);
		int64_t safeSubInt64(const int64_t& a, const int64_t& b);

};



