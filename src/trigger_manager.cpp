#include "shared_memory.hpp"
#include "bloom_filter.hpp"

extern "C" {

Datum octo_bloom_insert_trigger(PG_FUNCTION_ARGS) {
    TriggerData* trigdata = (TriggerData*) fcinfo->context;
    
    if (!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event) || 
        !TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
        !TRIGGER_FIRED_BY_INSERT(trigdata->tg_event)) {
        PG_RETURN_NULL();
    }
    
    TupleDesc tupdesc = trigdata->tg_relation->rd_att;
    HeapTuple newtuple = trigdata->tg_newtuple;
    
    // Get bloom filter for this table and column
    Oid table_oid = trigdata->tg_relation->rd_id;
    
    // For each column that has a bloom filter, add the value
    for (int i = 0; i < tupdesc->natts; ++i) {
        Form_pg_attribute attr = TupleDescAttr(tupdesc, i);
        
        OctoBloomFilter* filter = get_bloom_filter(table_oid, attr->attnum);
        if (filter && !attr->attisdropped) {
            bool isnull;
            Datum value = heap_getattr(newtuple, i + 1, tupdesc, &isnull);
            
            if (!isnull) {
                text* value_text = DatumGetTextP(value);
                filter->add(VARDATA(value_text), VARSIZE(value_text) - VARHDRSZ);
            }
        }
    }
    
    PG_RETURN_POINTER(newtuple);
}

Datum octo_bloom_update_trigger(PG_FUNCTION_ARGS) {
    TriggerData* trigdata = (TriggerData*) fcinfo->context;
    
    if (!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event) || 
        !TRIGGER_FIRED_AFTER(trigdata->tg_event) ||
        !TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event)) {
        PG_RETURN_NULL();
    }
    
    TupleDesc tupdesc = trigdata->tg_relation->rd_att;
    HeapTuple oldtuple = trigdata->tg_trigtuple;
    HeapTuple newtuple = trigdata->tg_newtuple;
    
    Oid table_oid = trigdata->tg_relation->rd_id;
    
    // For each column that has a bloom filter
    for (int i = 0; i < tupdesc->natts; ++i) {
        Form_pg_attribute attr = TupleDescAttr(tupdesc, i);
        
        OctoBloomFilter* filter = get_bloom_filter(table_oid, attr->attnum);
        if (filter && !attr->attisdropped) {
            bool old_isnull, new_isnull;
            Datum old_value = heap_getattr(oldtuple, i + 1, tupdesc, &old_isnull);
            Datum new_value = heap_getattr(newtuple, i + 1, tupdesc, &new_isnull);
            
            // Remove old value if it exists
            if (!old_isnull) {
                text* old_value_text = DatumGetTextP(old_value);
                filter->remove(VARDATA(old_value_text), VARSIZE(old_value_text) - VARHDRSZ);
            }
            
            // Add new value
            if (!new_isnull) {
                text* new_value_text = DatumGetTextP(new_value);
                filter->add(VARDATA(new_value_text), VARSIZE(new_value_text) - VARHDRSZ);
            }
        }
    }
    
    PG_RETURN_POINTER(newtuple);
}

} // extern "C"