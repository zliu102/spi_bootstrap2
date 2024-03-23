/*-------------------------------------------------------------------------
 *
 * binary_search.c
 *	  PostgreSQL type definitions for BINARY_SEARCHs
 *
 * Author:	Boris Glavic
 * Portions Copyright (c) 1996-2016, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *	  contrib/binary_search/binary_search.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "spi_bootstrap2.h"

#include "c.h"
#include "catalog/pg_collation_d.h"
#include "catalog/pg_type_d.h"
#include "fmgr.h"
#include "lib/stringinfo.h"
#include "nodes/parsenodes.h"
#include "nodes/makefuncs.h"
#include "parser/parse_func.h"
#include "utils/array.h"
#include "utils/arrayaccess.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/selfuncs.h"
#include "utils/typcache.h"
#include "utils/varbit.h"
#include "postgres.h"
#include <limits.h>
#include "catalog/pg_type.h"
#include <ctype.h>

#include "access/htup_details.h"
#include "catalog/pg_type.h"
#include "funcapi.h"
#include "lib/stringinfo.h"
#include "libpq/pqformat.h"
#include "utils/builtins.h"
#include "utils/json.h"
#include "utils/jsonapi.h"
#include "utils/jsonb.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/typcache.h"


#include "access/heapam.h"
#include "utils/typcache.h"
#include <ctype.h>
#include "executor/spi.h"
#include "utils/memutils.h"
#include "utils/rel.h"
#include "miscadmin.h"
#include "access/printtup.h"
//#include "/home/oracle/datasets/postgres11ps/postgres-pbds/contrib/intarray/_int.h"
#define MAX_QUANTITIES 10 
#define MAX_GROUPS 90000

PG_MODULE_MAGIC;
/*
typedef struct {
    int l_suppkey;
    double l_tax;
    float4 quantities[MAX_QUANTITIES];
    int count;
} MyGroup;
*/
typedef struct {
    char* l_suppkey; 
    char* l_tax; 
    float4 quantities[MAX_QUANTITIES];
    int count;
} MyGroup;

typedef struct {
    MyGroup groups[MAX_GROUPS];
    int numGroups;
} GroupsContext;

// Utility function declarations
static void prepTuplestoreResult(FunctionCallInfo fcinfo);
//static int findOrCreateGroup(GroupsContext *context, int l_suppkey, double l_tax);
static int findOrCreateGroup(GroupsContext *context, char* l_suppkey, char* l_tax);
static void addQuantityToGroup(MyGroup *group, float4 quantity);
static float4 calculateRandomSampleAverage(float4 *quantities, int count);


static void
prepTuplestoreResult(FunctionCallInfo fcinfo)
{
    ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;

    /* check to see if query supports us returning a tuplestore */
    if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
        ereport(ERROR,
                (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                 errmsg("set-valued function called in context that cannot accept a set")));
    if (!(rsinfo->allowedModes & SFRM_Materialize))
        ereport(ERROR,
                (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                 errmsg("materialize mode required, but it is not allowed in this context")));

    /* let the executor know we're sending back a tuplestore */
    rsinfo->returnMode = SFRM_Materialize;

    /* caller must fill these to return a non-empty result */
    rsinfo->setResult = NULL;
    rsinfo->setDesc = NULL;
}
/*
static int findOrCreateGroup(GroupsContext *context, int l_suppkey, double l_tax) {
    static int last_l_suppkey = -1; 
    static double last_l_tax = -1.0; 
    static int last_groupIndex = -1; 
    if (l_suppkey == last_l_suppkey && l_tax == last_l_tax) {
        return last_groupIndex;
    }


    if (context->numGroups >= MAX_GROUPS) {
        ereport(ERROR, (errmsg("error")));
        return -1; 
    }

  
    int newIndex = context->numGroups;
    context->groups[context->numGroups].l_suppkey = l_suppkey;
    context->groups[context->numGroups].l_tax = l_tax;
    context->groups[context->numGroups].count = 0;


    last_l_suppkey = l_suppkey;
    last_l_tax = l_tax;
    last_groupIndex = newIndex;

    context->numGroups++; 
    return newIndex;
}*/

static int findOrCreateGroup(GroupsContext *context, char* l_suppkey, char* l_tax) {
    static char* last_l_suppkey = NULL; 
    static char* last_l_tax = NULL;
    static int last_groupIndex = -1;
    
    // 检查上一个值是否相同（这里使用 strcmp 比较字符串）
    if ((last_l_suppkey != NULL && strcmp(l_suppkey_str, last_l_suppkey) == 0) &&
        (last_l_tax != NULL && strcmp(l_tax_str, last_l_tax) == 0)) {
        return last_groupIndex;
    }

    if (context->numGroups >= MAX_GROUPS) {
        ereport(ERROR, (errmsg("Maximum number of groups exceeded")));
        return -1;
    }

    int newIndex = context->numGroups;
    // 使用 strdup 或等效方法复制字符串
    context->groups[newIndex].l_suppkey = strdup(l_suppkey_str);
    context->groups[newIndex].l_tax = strdup(l_tax_str);
    context->groups[newIndex].count = 0;

    // 更新上一个值的指针
    last_l_suppkey = context->groups[newIndex].l_suppkey;
    last_l_tax = context->groups[newIndex].l_tax;
    last_groupIndex = newIndex;

    context->numGroups++;
    return newIndex;
}


static void addQuantityToGroup(MyGroup *group, float4 quantity) {
    if (group->count >= MAX_QUANTITIES) {
        ereport(ERROR, (errmsg("error")));
        return;
    }
    group->quantities[group->count++] = quantity;
}


static float4 calculateRandomSampleAverage(float4 *quantities, int count) {
    int sampleSize = 500;
    float4 sum = 0;
    int i;
    for (i = 0; i < sampleSize; ++i) {
        int idx = rand() % count; 
        sum += quantities[idx];
    }
    return sum / sampleSize;
}

static float4 calculateStandardDeviation(float4 *quantities, int count, float4 mean) {
    float4 sum_diff_sq = 0;
    int i = 0;
    for (i = 0; i < count; i++) {
        float4 diff = quantities[i] - mean;
        sum_diff_sq += diff * diff;
    }
    float4 variance = sum_diff_sq / count;
    return sqrt(variance); 
}


PG_FUNCTION_INFO_V1(spi_bootstrap_array);

Datum spi_bootstrap_array(PG_FUNCTION_ARGS) {
    int ret;
    int i;
    Tuplestorestate *tupstore;
    TupleDesc tupdesc;
    MemoryContext oldcontext;
    MemoryContext per_query_ctx;
    ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
    // Connect to SPI
    if (SPI_connect() != SPI_OK_CONNECT) {
        ereport(ERROR, (errmsg("SPI_connect failed")));
    }

    // Prepare and execute the SQL query
    char sql[1024];
    char* sampleSize = text_to_cstring(PG_GETARG_TEXT_PP(0));
    char* tablename = text_to_cstring(PG_GETARG_TEXT_PP(1));
    char* otherAttribue = text_to_cstring(PG_GETARG_TEXT_PP(2));
    char* groupby = text_to_cstring(PG_GETARG_TEXT_PP(3));
    //prepTuplestoreResult(fcinfo);
    /*
    

    per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
    oldcontext = MemoryContextSwitchTo(CurrentMemoryContext);
    //oldcontext = MemoryContextSwitchTo(rsinfo->econtext->ecxt_per_query_memory);

    tupstore = tuplestore_begin_heap(true, false, work_mem);
    rsinfo->returnMode = SFRM_Materialize;
    rsinfo->setResult = tupstore;
    rsinfo->setDesc = tupdesc;

    MemoryContextSwitchTo(oldcontext);
*/


    per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
    oldcontext = MemoryContextSwitchTo(per_query_ctx);
    tupstore = tuplestore_begin_heap(true, false, work_mem);
    MemoryContextSwitchTo(oldcontext); //test

    snprintf(sql, sizeof(sql), "select * from reservoir_sampler_tpch(%s,'%s','%s','%s');",sampleSize,tablename,otherAttribue,groupby);
    //elog(INFO, "SPI query -- %s", sql);
    ret = SPI_execute(sql, true, 0);
    if (ret != SPI_OK_SELECT) {
        SPI_finish();
        ereport(ERROR, (errmsg("SPI_execute failed")));
    }

    // Prepare for tuplestore use
    tupdesc = CreateTemplateTupleDesc(4, false);
    TupleDescInitEntry(tupdesc, (AttrNumber) 1, "l_suppkey", INT4OID, -1, 0);
    TupleDescInitEntry(tupdesc, (AttrNumber) 2, "l_tax", NUMERICOID, -1, 0);
    TupleDescInitEntry(tupdesc, (AttrNumber) 3, "avg_l_quantity", FLOAT4OID, -1, 0);
    TupleDescInitEntry(tupdesc, (AttrNumber) 4, "std_l_quantity", FLOAT4OID, -1, 0);
    //TupleDescInitEntry(tupdesc, (AttrNumber) 3, "avg_l_quantity", INT4OID, -1, 0);
    //tupdesc = CreateTupleDescCopy(SPI_tuptable->tupdesc);
    //tupdesc = BlessTupleDesc(tupdesc);
    
    //tupstore = tuplestore_begin_heap(true, false, work_mem);
    //MemoryContextSwitchTo(oldcontext); //problem 

    
    
    
    // Initialize GroupsContext
    GroupsContext groupsContext;
    groupsContext.numGroups = 0;

    // Process SPI results
   
    for (i = 0; i < SPI_processed; i++) {
        //HeapTuple tuple = SPI_tuptable->vals[i];
        //TupleDesc tupdesc = SPI_tuptable->tupdesc;
        //elog(INFO, "SPI current id is -- %d", i);

        int attnum1 = SPI_fnumber(SPI_tuptable->tupdesc, "l_suppkey");
        int attnum2 = SPI_fnumber(SPI_tuptable->tupdesc, "l_tax");
        int attnum3 = SPI_fnumber(SPI_tuptable->tupdesc, "l_quantity");
        char* value1 = SPI_getvalue((SPI_tuptable->vals)[i], SPI_tuptable->tupdesc, attnum1);
        char* value2 = SPI_getvalue((SPI_tuptable->vals)[i], SPI_tuptable->tupdesc, attnum2);
        char* value3 = SPI_getvalue((SPI_tuptable->vals)[i], SPI_tuptable->tupdesc, attnum3);
        
        //int l_suppkey = atoi(value1);
        //int l_returnflag_int = atoi(value2);
        //double l_tax = strtod(value2, NULL); 
        int quantity = atoi(value3);

        //Datum numericValue3 = DirectFunctionCall3(numeric_in, CStringGetDatum("0.00"), ObjectIdGetDatum(InvalidOid), Int32GetDatum(-1));
        //elog(INFO, "l_tax_datum is %f",numericValue3);

        //int l_suppkey = DatumGetInt32(SPI_getbinval(tuple, tupdesc, 1, NULL));
        //int l_returnflag_int = DatumGetInt32(SPI_getbinval(tuple, tupdesc, 2, NULL));
        //int quantity = DatumGetInt32(SPI_getbinval(tuple, tupdesc, 3, NULL));
        //elog(INFO, "SPI l_suppkey -- %d", l_suppkey);
        //elog(INFO, "SPI l_returnflag_int -- %d", l_returnflag_int);
        //elog(INFO, "SPI quantity -- %d", quantity);
      
        int groupIndex = findOrCreateGroup(&groupsContext, value1, value2);
        if (groupIndex != -1) { 
            addQuantityToGroup(&groupsContext.groups[groupIndex], quantity);
        }

        //elog(INFO, "group l_suppkey is %d", group->l_suppkey);
        //elog(INFO, "group l_returnflag_int is %d", group->l_returnflag_int); 
    }
    //elog(INFO, "Finish adding");
    // Process each group: calculate random sample average and store results
    srand(time(NULL)); // Initialize random seed
    int j;
    for (j = 0; j < groupsContext.numGroups; j++) {
        //elog(INFO, "SPI j is -- %d", j);
        
        MyGroup *group = &groupsContext.groups[j];
        
        float4 avg_l_quantity = calculateRandomSampleAverage(group->quantities, group->count);

        float4 stddev = calculateStandardDeviation(group->quantities, group->count, avg_l_quantity);

        Datum values[4];
        bool nulls[4] = {false, false, false, false};

        //values[0] = Int32GetDatum(group->l_suppkey);
        //values[1] = DirectFunctionCall1(float8_numeric, Float8GetDatum(group->l_tax));
        values[0] = Int32GetDatum(atoi(group->l_suppkey));
        values[1] = DirectFunctionCall3(numeric_in, CStringGetDatum(group->l_tax), ObjectIdGetDatum(InvalidOid), Int32GetDatum(-1));
        values[2] = Float4GetDatum(avg_l_quantity);
        values[3] = Float4GetDatum(stddev);
        //values[0] = group->l_suppkey;
        //values[1] = group->l_returnflag_int;
        //values[2] = avg_quantity;
        //elog(INFO, "l_suppkey is %d",values[0]);
        //elog(INFO, "l_returnflag_int is %d",values[1]);
        //elog(INFO, "avg_l_quantity is %f",avg_l_quantity);
        
        

        tuplestore_putvalues(tupstore, tupdesc, values, nulls);
        
    }
    

    /*
    Datum values[3]; 
    bool nulls[3];
    //bool nulls[3] = {false, false, false}; 
    nulls[0] = false;
    nulls[1] = false;
    nulls[2] = false;
        
    values[0] = Int32GetDatum(1); 
    values[1] = Int32GetDatum(2); 
    //values[2] = Int32GetDatum(3);
    values[2] = Float4GetDatum(3.14); 
    elog(INFO, "here");
    elog(INFO, "l_suppkey is %d",values[0]);
    elog(INFO, "l_returnflag_int is %d",values[1]);
    elog(INFO, "avg_l_quantity is %f",3.14);
    tuplestore_putvalues(tupstore, tupdesc, values, nulls);
    //HeapTuple tuple = heap_form_tuple(tupdesc, values, nulls);
    //tuplestore_puttuple(tupstore, tuple);
    */
    tuplestore_donestoring(tupstore);
    // Cleanup

    rsinfo->setResult = tupstore;
    rsinfo->setDesc = tupdesc;
    rsinfo->returnMode = SFRM_Materialize;
    
    SPI_finish();

    PG_RETURN_NULL();
}

// Definitions of utility functions...



/*
PG_FUNCTION_INFO_V1(spi_bootstrap);
Datum
spi_bootstrap(PG_FUNCTION_ARGS)
{
    int ret;
    int row;
    Tuplestorestate *tupstore;
    TupleDesc tupdesc;
    MemoryContext oldcontext;
   
  
    int64 sampleSize = PG_GETARG_INT64(0);
    //int64 groupby_id = PG_GETARG_INT64(1);// ARRAY LATER
    char* tablename = text_to_cstring(PG_GETARG_TEXT_PP(1));
    char* otherAttribue = text_to_cstring(PG_GETARG_TEXT_PP(2));
    char* groupby = text_to_cstring(PG_GETARG_TEXT_PP(3));
    prepTuplestoreResult(fcinfo);

    ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;

    if ((ret = SPI_connect()) < 0)
    
        elog(ERROR, "range_window: SPI_connect returned %d", ret);

    // elog(NOTICE, "range_window: SPI connected.");
    char    sql[8192];
    //snprintf(sql, sizeof(sql),"SELECT b, c, a FROM (SELECT b, c, a, ROW_NUMBER() OVER (PARTITION BY b, c ORDER BY random()) AS rn FROM test2) t WHERE rn <= 3 ORDER BY b, c, a;");


    //snprintf(sql, sizeof(sql),"select %s,%s from %s order by %s",groupby,otherAttribue, tablename,groupby); //generate string from array
    snprintf(sql, sizeof(sql),"select %s,%s from %s",groupby,otherAttribue, tablename);
    //elog(INFO, "SPI query -- %s", sql);

    ret = SPI_execute(sql, true, 0);

    if (ret < 0)
     
        elog(ERROR, "range_window: SPI_exec returned %d", ret);


    //tupdesc = SPI_tuptable->tupdesc;
    tupdesc = CreateTemplateTupleDesc(3, false);
    TupleDescInitEntry(tupdesc, (AttrNumber) 1, "l_suppkey", INT4OID, -1, 0);
    TupleDescInitEntry(tupdesc, (AttrNumber) 1, "l_returnflag_int", INT4OID, -1, 0);
    //TupleDescInitEntry(tupdesc, (AttrNumber) 1, "l_tax", NUMERICOID, -1, 0);

    TupleDescInitEntry(tupdesc, (AttrNumber) 1, "l_quantity", INT4OID, -1, 0);
    // TupleDescInitEntry(tupdesc, (AttrNumber) 1, "l_partkey", INT4OID, -1, 0);
    // TupleDescInitEntry(tupdesc, (AttrNumber) 1, "l_orderkey", INT4OID, -1, 0);
    // TupleDescInitEntry(tupdesc, (AttrNumber) 1, "l_extendedprice", INT4OID, -1, 0);
    // TupleDescInitEntry(tupdesc, (AttrNumber) 1, "l_discount", NUMERICOID, -1, 0);
    // TupleDescInitEntry(tupdesc, (AttrNumber) 1, "l_linenumber", INT4OID, -1, 0);
    oldcontext = MemoryContextSwitchTo(rsinfo->econtext->ecxt_per_query_memory);
    tupstore = tuplestore_begin_heap(true, false, work_mem);
    rsinfo->setResult = tupstore;
    tupdesc = CreateTupleDescCopy(SPI_tuptable->tupdesc);
    rsinfo->setDesc = tupdesc;
    MemoryContextSwitchTo(oldcontext);
    tupdesc = BlessTupleDesc(tupdesc);

    HeapTuple reservoir[sampleSize];
    char *last_group = ""; 
    int poscnt;
    bool initialized = false;
    for(row = 0; row < SPI_processed; row++){
     
        int attnum1 = SPI_fnumber(SPI_tuptable->tupdesc, "l_suppkey");
        int attnum2 = SPI_fnumber(SPI_tuptable->tupdesc, "l_returnflag_int");
        char* value1 = SPI_getvalue((SPI_tuptable->vals)[row], SPI_tuptable->tupdesc, attnum1);
        char* value2 = SPI_getvalue((SPI_tuptable->vals)[row], SPI_tuptable->tupdesc, attnum2);
        char *current_group = strcat(value1, ",");
            current_group = strcat(current_group,value2);
        
        
        if(strcmp(current_group, last_group)){
            //check whether is different groups
            //elog(INFO, "different group");
            last_group = current_group;
            poscnt = 0;
            int i;
            if(initialized){
                for (i = 0;i<sampleSize;i++){
                    if(reservoir[i]!= 0){
                        tuplestore_puttuple(tupstore, reservoir[i]);
                    } else {
                        break;
                    }
                }
            } //if not the first group, store the reservior and renew the reservoir 
            memset(reservoir, 0, sizeof(HeapTuple) * sampleSize);
            initialized = true;
        } 
        if (poscnt < sampleSize) {
                //HeapTuple tuple = SPI_copytuple((SPI_tuptable->vals)[row]);
                HeapTuple tuple = SPI_tuptable->vals[row];
                reservoir[poscnt] = tuple;
                poscnt ++;
        } else {
            int pos = rand() % (poscnt+1);
            if (pos < sampleSize){
                //HeapTuple tuple = SPI_copytuple((SPI_tuptable->vals)[row]);
                HeapTuple tuple = SPI_tuptable->vals[row];
                reservoir[pos] = tuple; 
            }
            poscnt ++;
        }

        


    }
    int j;
    for (j = 0;j<sampleSize;j++){
        if(reservoir[j]!= 0){
            tuplestore_puttuple(tupstore, reservoir[j]);
        } else {                
            break;
        }
    } //last group
    //tuplestore_puttuple(tupstore, reservoir[0]);
   

   
    tuplestore_donestoring(tupstore);

    SPI_finish();
    //PG_RETURN_ARRAYTYPE_P(a_values);
    PG_RETURN_NULL();
}
*/
