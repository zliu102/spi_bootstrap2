

/*
CREATE FUNCTION reservoir_sampler_tpch(sampleSize bigint, tablename text, otherAttribue text,groupby text) 
RETURNS TABLE (l_suppkey int, l_returnflag_int int, l_tax numeric, l_quantity int, l_partkey int, l_orderkey int, l_extendedprice int, l_discount numeric, l_linenumber int) 
AS 'MODULE_PATHNAME','reservoir_sampler_tpch' 
LANGUAGE C STRICT;
*/
/*
CREATE FUNCTION spi_bootstrap(sampleSize bigint, tablename text, otherAttribue text,groupby text) 
RETURNS TABLE (l_suppkey int, l_returnflag_int int, l_quantity int) 
AS 'MODULE_PATHNAME','spi_bootstrap' 
LANGUAGE C STRICT;
*/

/*
CREATE FUNCTION spi_bootstrap_array(sampleSize text, tablename text, otherAttribue text,groupby text) 
RETURNS TABLE (l_suppkey int, l_tax numeric, avg_l_quantity float4, std_l_quantity float4) 
AS 'MODULE_PATHNAME','spi_bootstrap_array' 
LANGUAGE C STRICT;*/

CREATE FUNCTION spi_bootstrap_array_all(sampleSize text, tablename text, otherAttribue text,groupby text) 
RETURNS TABLE (l_suppkey int, l_partkey int, avg_l_quantity float4, avg_l_orderkey float4, avg_l_extendedprice float4, avg_l_linenumber float4) 
AS 'MODULE_PATHNAME','spi_bootstrap_array_all' 
LANGUAGE C STRICT;

   
/*
, avg_l_orderkey float4, avg_l_extendedprice float4
, avg_l_orderkey float4, std_l_orderkey float4, avg_l_extendedprice float4, std_l_extendedprice float4,avg_l_linenumber float4, std_l_linenumber float4, avg_l_discount float4, std_l_discount float4

CREATE FUNCTION res_tras_crimes2_c(Datum, int64)
        RETURNS Datum
        AS 'MODULE_PATHNAME', 'res_tras_crimes'
        LANGUAGE C
        IMMUTABLE 
        PARALLEL SAFE;

CREATE FUNCTION finalize_trans_crimes2_c(Datum)
        RETURNS ArrayType
        AS 'MODULE_PATHNAME'
        LANGUAGE C
        IMMUTABLE 
        PARALLEL SAFE;

CREATE AGGREGATE reservoir_sampling2_c(int64)
(
        sfunc = res_tras_crimes2_c,
        stype = state_c,
        FINALFUNC = finalize_trans_crimes2_c,
);*/