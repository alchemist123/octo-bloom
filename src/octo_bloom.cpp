#include "shared_memory.hpp"
#include "bloom_filter.hpp"

extern "C" {

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

// Function declarations
PG_FUNCTION_INFO_V1(octo_bloom_init);
PG_FUNCTION_INFO_V1(octo_bloom_might_contain);
PG_FUNCTION_INFO_V1(octo_bloom_exists);
PG_FUNCTION_INFO_V1(octo_bloom_attach_triggers);
PG_FUNCTION_INFO_V1(octo_bloom_status);
PG_FUNCTION_INFO_V1(octo_bloom_rebuild);
PG_FUNCTION_INFO_V1(octo_bloom_disable);

// Trigger functions
PG_FUNCTION_INFO_V1(octo_bloom_insert_trigger);
PG_FUNCTION_INFO_V1(octo_bloom_update_trigger);
PG_FUNCTION_INFO_V1(octo_bloom_delete_trigger);

void _PG_init(void);
void _PG_fini(void);

// Shared memory initialization is now in shared_memory.cpp

Datum octo_bloom_init(PG_FUNCTION_ARGS) {
    Oid table_oid = PG_GETARG_OID(0);
    text* column_name = PG_GETARG_TEXT_P(1);
    uint64_t expected_count = PG_GETARG_INT64(2);
    double false_positive_rate = PG_GETARG_FLOAT8(3);
    
    // Validate parameters
    if (expected_count == 0) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("expected_count must be greater than zero")));
    }
    
    if (false_positive_rate <= 0 || false_positive_rate >= 1) {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("false_positive_rate must be between 0 and 1")));
    }
    
    // Get attribute number from column name
    char* col_name = text_to_cstring(column_name);
    int16_t attnum = get_attnum(table_oid, col_name);
    
    if (attnum == InvalidAttrNumber) {
        ereport(ERROR,
                (errcode(ERRCODE_UNDEFINED_COLUMN),
                 errmsg("column \"%s\" does not exist", col_name)));
    }
    
    // Register bloom filter in shared memory
    if (!register_bloom_filter(table_oid, attnum, expected_count, false_positive_rate)) {
        ereport(ERROR,
                (errcode(ERRCODE_OUT_OF_MEMORY),
                 errmsg("failed to register bloom filter: out of shared memory or filter already exists")));
    }
    
    PG_RETURN_VOID();
}

Datum octo_bloom_might_contain(PG_FUNCTION_ARGS) {
    Oid table_oid = PG_GETARG_OID(0);
    text* column_name = PG_GETARG_TEXT_P(1);
    Datum value = PG_GETARG_DATUM(2);
    Oid value_type = get_fn_expr_argtype(fcinfo->flinfo, 2);
    
    char* col_name = text_to_cstring(column_name);
    int16_t attnum = get_attnum(table_oid, col_name);
    
    if (attnum == InvalidAttrNumber) {
        ereport(ERROR,
                (errcode(ERRCODE_UNDEFINED_COLUMN),
                 errmsg("column \"%s\" does not exist", col_name)));
    }
    
    OctoBloomFilter* filter = get_bloom_filter(table_oid, attnum);
    if (!filter) {
        PG_RETURN_BOOL(true); // If no filter, assume might contain
    }

    // Check if filter is valid (basic validation)
    if (filter->getBitArraySize() == 0) {
        PG_RETURN_BOOL(true); // Invalid filter
    }

    // Convert value to text for hashing
    text* value_text = DatumGetTextP(value);
    if (!value_text) {
        PG_RETURN_BOOL(true);
    }

    bool might_contain = filter->mightContain(VARDATA(value_text), VARSIZE(value_text) - VARHDRSZ);

    PG_RETURN_BOOL(might_contain);
}

Datum octo_bloom_exists(PG_FUNCTION_ARGS) {
    Oid table_oid = PG_GETARG_OID(0);
    text* column_name = PG_GETARG_TEXT_P(1);
    Datum value = PG_GETARG_DATUM(2);
    Oid value_type = get_fn_expr_argtype(fcinfo->flinfo, 2);
    
    // First check with bloom filter
    bool might_contain = DatumGetBool(octo_bloom_might_contain(fcinfo));
    
    if (!might_contain) {
        PG_RETURN_BOOL(false);
    }
    
    // If bloom filter says might contain, verify with actual query
    char* col_name = text_to_cstring(column_name);
    char* table_name = get_rel_name(table_oid);
    
    // Connect to SPI
    if (SPI_connect() != SPI_OK_CONNECT) {
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("SPI_connect failed")));
    }
    
    StringInfoData query;
    initStringInfo(&query);
    appendStringInfo(&query, "SELECT 1 FROM %s WHERE %s = $1 LIMIT 1",
                     quote_identifier(table_name),
                     quote_identifier(col_name));
    
    Oid param_types[1] = {value_type};
    Datum param_values[1] = {value};
    
    SPIPlanPtr plan = SPI_prepare(query.data, 1, param_types);
    if (plan == NULL) {
        SPI_finish();
        ereport(ERROR,
                (errcode(ERRCODE_INTERNAL_ERROR),
                 errmsg("SPI_prepare failed")));
    }
    
    bool exists = false;
    int ret = SPI_execute_plan(plan, param_values, NULL, true, 0);
    if (ret == SPI_OK_SELECT && SPI_processed > 0) {
        exists = true;
    }
    
    SPI_freeplan(plan);
    pfree(query.data);
    SPI_finish();
    
    PG_RETURN_BOOL(exists);
}

// Other function implementations would follow similar patterns...

void _PG_init(void) {
    // Extension initialization - shared memory will be initialized on first use
}

void _PG_fini(void) {
    // Cleanup code if needed
}

} // extern "C"