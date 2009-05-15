/*
 * adt-typesafe-dict.h
 *
 *  Created on: May 15, 2009
 *      Author: petergoodman
 *     Version: $Id$
 */

#ifndef ADTTYPESAFEDICT_H_
#define ADTTYPESAFEDICT_H_



#define TYPESAFE_HASH_TABLE(table_type, func_prefix, internal_prefix, key_type, val_type) \
    typedef uint32_t (*table_type##HashFunction)(key_type);\
    typedef char (*table_type##CollisionFunction)(key_type, key_type);\
    typedef struct internal_prefix##_Entry {\
        val_type entry;\
        key_type key;\
    } internal_prefix##_Entry;\
    typedef struct table_type {\
        internal_prefix##_Entry ** elms;\
        uint32_t num_slots,\
                 num_used_slots;\
        table_type##HashFunction key_hash_fnc;\
        table_type##CollisionFunction collision_fnc;\
        int prime_index;\
    } table_type;\
    static const unsigned int internal_prefix##_primes[] = {\
        53, 97, 193, 389,\
        769, 1543, 3079, 6151,\
        12289, 24593, 49157, 98317,\
        196613, 393241, 786433, 1572869,\
        3145739, 6291469, 12582917, 25165843,\
        50331653, 100663319, 201326611, 402653189,\
        805306457, 1610612741\
    };\
    static const int internal_prefix##_num_primes = sizeof(internal_prefix##_primes)/sizeof(internal_prefix##_primes[0]);\
    typedef struct internal_prefix##_EntryInfo {\
        internal_prefix##_Entry *entry;\
        uint32_t hash_key;\
    } internal_prefix##_EntryInfo;\
    static internal_prefix##_EntryInfo internal_prefix##_get_entry_info(table_type *H, key_type key) {\
        internal_prefix##_EntryInfo info;\
        assert_not_null(H);\
        info.hash_key = H->key_hash_fnc(key) % H->num_slots;\
        info.entry = H->elms[info.hash_key];\
        while(is_not_null(info.entry) && H->collision_fnc(key, info.entry->key)) {\
            info.hash_key = (info.hash_key+1) % H->num_slots;\
            info.entry = H->elms[info.hash_key];\
        }\
        return info;\
    }\
    static internal_prefix##_Entry **internal_prefix##_alloc_slots(uint32_t num_slots ) {\
        internal_prefix##_Entry **slots = mem_calloc(num_slots, sizeof(internal_prefix##_Entry *));\
        if(is_null(slots)) {\
            mem_error("Unable to allocate hash table slots.");\
        }\
        return slots;\
    }\
    static void internal_prefix##_grow(table_type *H ) {\
        internal_prefix##_Entry **old_elms = NULL;\
        internal_prefix##_EntryInfo info;\
        uint32_t old_size,\
                 j;\
        assert_not_null(H);\
        if(internal_prefix##_num_primes < ++(H->prime_index)) {\
            std_error("Error: Unable to grow hash table further.");\
        }\
        old_elms = H->elms;\
        old_size = H->num_slots;\
        H->num_slots = internal_prefix##_primes[H->prime_index];\
        H->elms = internal_prefix##_alloc_slots(H->num_slots);\
        for(j = 0; j < old_size; ++j) {\
            info = internal_prefix##_get_entry_info(H, old_elms[j]->key);\
            H->elms[info.hash_key] = old_elms[j];\
        }\
        mem_free(old_elms);\
        return;\
    }\
    type_type *func_prefix##_alloc(uint32_t num_slots,\
                         table_type##HashFunction key_hash_fnc,\
                         table_type##CollisionFunction val_collision_fnc) {\
        table_type *H;\
        internal_prefix##_Entry **elms;\
        int i;\
        table_type *table;\
        assert(sizeof(table_type) <= func_prefix##_struct_size);\
        assert_not_null(key_hash_fnc);\
        assert_not_null(val_collision_fnc);\
        table = mem_alloc(func_prefix##_struct_size);\
        if(is_null(table)) {\
            mem_error("Unable to allocate vector on the heap.");\
        }\
        for(i = 0; i < internal_prefix##_num_primes; ++i) {\
            if(num_slots < internal_prefix##_primes[i]) {\
                num_slots = internal_prefix##_primes[i];\
                break;\
            }\
        }\
        if(internal_prefix##_num_primes < i) {\
            std_error("Error, the hash table size given is too large.");\
        }\
        elms = internal_prefix##_alloc_slots(num_slots);\
        H = (table_type *) table;\
        H->elms = elms;\
        H->num_slots = num_slots;\
        H->num_used_slots = 0;\
        H->key_hash_fnc = key_hash_fnc;\
        H->collision_fnc = val_collision_fnc;\
        H->prime_index = i;\
        return table;\
    }\
    void func_prefix##_free(table_type *H, PDelegate free_elm_fnc ) {\
        uint32_t i;\
        assert_not_null(H);\
        assert_not_null(free_elm_fnc);\
        if(free_elm_fnc != delegate_do_nothing) {\
            for(i = 0; i < H->num_slots; ++i) {\
                if(is_not_null(H->elms[i])) {\
                    free_elm_fnc(H->elms[i] );\
                }\
            }\
        }\
        mem_free(H->elms);\
        mem_free(H);\
        H = NULL;\
        return;\
    }\
    char func_prefix##_set(table_type *H, key_type key, val_type val,\
                  PDelegate free_on_overwrite_fnc) {\
        char did_overwrite;\
        internal_prefix##_Entry *entry;\
        internal_prefix##_EntryInfo info;\
        assert_not_null(H);\
        assert_not_null(key);\
        assert_not_null(free_on_overwrite_fnc);\
        if(0.65 < (H->num_used_slots / H->num_slots)) {\
            internal_prefix##_grow(H);\
        }\
        info = internal_prefix##_get_entry_info(H, key);\
        entry = info.entry;\
        did_overwrite = 0;\
        if(is_not_null(entry)) {\
            free_on_overwrite_fnc(info.entry->entry);\
            did_overwrite = 1;\
        } else {\
            entry = mem_alloc(sizeof(internal_prefix##_Entry));\
            if(is_null(entry)) {\
                mem_error("Unable to allocate dictionary entry on the heap.");\
            }\
        }\
        entry->entry = val;\
        entry->key = key;\
        H->elms[info.hash_key] = entry;\
        return did_overwrite;\
    }\
    static void func_prefix##_unset(table_type *H, key_type key, PDelegate free_fnc) {\
        internal_prefix##_EntryInfo info = internal_prefix##_get_entry_info(H, key);\
        if(is_not_null(info.entry)) {\
            free_fnc(info.entry->entry);\
            mem_free(info.entry);\
            H->elms[info.hash_key] = NULL;\
            --(H->num_used_slots);\
        }\
        return;\
    }\
    static val_type func_prefix##_get(table_type *H, key_type key) {\
        internal_prefix##_EntryInfo info = internal_prefix##_get_entry_info(H, key);\
        return (is_null(info.entry) ? NULL : info.entry->entry);\
    }\
    static char func_prefix##_is_set(table_type *H, key_type key) {\
        internal_prefix##_EntryInfo info = internal_prefix##_get_entry_info(H, key);\
        return is_not_null(info.entry);\
    }


#endif /* ADTTYPESAFEDICT_H_ */
