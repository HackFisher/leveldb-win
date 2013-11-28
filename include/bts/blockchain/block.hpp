#pragma once
#include <bts/small_hash.hpp>
#include <bts/mini_pow.hpp>
#include <bts/blockchain/proof.hpp>
#include <bts/blockchain/transaction.hpp>
#include <bts/blockchain/asset.hpp>

namespace bts { namespace blockchain {

   /**
    *  Light-weight summary of a block that links it to
    *  all prior blocks.  This summary does not contain
    *  the nonce because that information is provided by
    *  the block_proof struct which is a header plus 
    *  proof of work.   
    */
   struct block_header
   {
      block_header()
      :version(0),block_num(-1){}

      /** digest used in proof of work calculation as 
       * the base of the proof merkle branch 
       **/
      uint160 digest()const;

      fc::unsigned_int    version;
      fc::sha224          prev;
      uint32_t            block_num;
      fc::time_point_sec  timestamp;   ///< seconds from 1970
      uint160             state_hash;  ///< ripemd160(  sha512( block_state ) )
      uint160             trx_mroot;   ///< merkle root of trx included in block, required for light client validation
   };

   uint64_t calculate_mining_reward( uint32_t blk_num );

   /**
    *  This is the minimum subset of data that must be kept to
    *  preserve the proof of work history.
    */
   struct block_proof : public block_header
   {
      /**
       *  Used to identify the previous block in block_header::prev
       */
      fc::sha224   id()const;

      mini_pow     proof_of_work()const;
      proof        pow; ///< contains the merkle branch + nonce
   };


   /**
    *  Tracks the ratio of bitshares to issued bit-assets, the
    *  unit types are defined by the location in the 
    *  block_state::issuance array
    */
   struct asset_issuance 
   {
      asset_issuance():backing(0),issued(0){}
      uint64_t  backing; /** total BitShares backing the issued currency */
      uint64_t  issued;  /** total asset issued */
   };


   /**
    *  Block state is maintained so that the initial condition of the 
    *  1 year old block can be known without having to have the full history. 
    *
    *  Storing the block state with every block for 1 year is about 100MB
    */
   struct block_state
   {
      block_state():dividend_percent(0){}
      uint160 digest()const;
      /**
       *  Dividends are expressed as a percentage of the money supply which
       *  is always less than 0.  In this case it is represented as a
       *  64 bit fixed point 0.64 fraction.
       *
       *  Dividends do not compound, but grow with a 'simple interest'
       *  formula which means that the total return can be calculated by
       *  summing the dividend_percent for each block that the balance
       *  was held.
       */
      uint64_t dividend_percent;

      /** initial condition prior to applying trx in this block */
      fc::array<asset_issuance,asset::type::count> issuance;  // 16 * 32 bytes = 512

      /**
       *  Features desired / supported by the miner. Once 75% of the past week worth
       *  of blocks supports a feature miner may start generating blocks that use
       *  the new feature / rule changes and anyone on the minority chain will
       *  be alerted that they no longer support the main chain.
       */
      std::vector<fc::unsigned_int>        supported_features;
   };


   /**
    *  This is the complete block including all transactions, 
    *  and the proof of work.
    */
   struct block : public block_proof
   {
     block_state state;
   };

   /**
    * A block complete with the IDs of the transactions included
    * in the block.  This is useful for communicating summaries when
    * the other party already has all of the trxs.
    */
   struct full_block : public block 
   {
      full_block( const block& b )
      :block(b){}
      full_block(){}
      uint160 calculate_merkle_root()const;
      std::vector<uint160>  trx_ids; 
   };

   /**
    *  A block that contains the full transactions rather than
    *  just the IDs of the transactions.
    */
   struct trx_block : public block
   {
      trx_block( const block& b )
      :block(b){}

      trx_block( const full_block& b, std::vector<signed_transaction> trs )
      :block(b),trxs( std::move(trs) ){}

      trx_block(){}
      operator full_block()const;
      uint160 calculate_merkle_root()const;
      std::vector<signed_transaction> trxs;
   };
   
   trx_block create_genesis_block();

} } // bts::blockchain

namespace fc 
{
   void to_variant( const bts::blockchain::trx_output& var,  variant& vo );
   void from_variant( const variant& var,  bts::blockchain::trx_output& vo );

   typedef fc::array<bts::blockchain::asset_issuance,bts::blockchain::asset::type::count> issuance_type; 
   void to_variant( const issuance_type& var,  variant& vo );
   void from_variant( const variant& var,  issuance_type& vo );
}

FC_REFLECT( bts::blockchain::asset_issuance,      (backing)(issued) )
FC_REFLECT( bts::blockchain::block_state,         (dividend_percent)(issuance)(supported_features) )
FC_REFLECT( bts::blockchain::block_header,        (version)(prev)(block_num)(timestamp)(state_hash)(trx_mroot) )
FC_REFLECT_DERIVED( bts::blockchain::block_proof, (bts::blockchain::block_header), (pow)     )
FC_REFLECT_DERIVED( bts::blockchain::block,       (bts::blockchain::block_proof),  (state)   )
FC_REFLECT_DERIVED( bts::blockchain::full_block,  (bts::blockchain::block),        (trx_ids) )
FC_REFLECT_DERIVED( bts::blockchain::trx_block,   (bts::blockchain::block),        (trxs) )

