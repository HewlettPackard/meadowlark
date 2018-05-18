#include "cache_api.h"
#include <malloc.h>
#include <string.h>
#include <string>
#include <iostream>
#include <cluster/config.h>

#define ERROR_TRACE		1

using namespace nvmm;
using namespace radixtree;

//memcached
bool use_slab_sizes;
uint32_t slab_sizes[MAX_NUMBER_OF_SLAB_CLASSES];
pthread_mutex_t kvs_global_lock = PTHREAD_MUTEX_INITIALIZER;
uint64_t num_hit;

//key value store
KeyValueStore *kvs;

#define HIT() (num_hit++)

item *item_get(const char *key, const size_t nkey, conn *c, const bool do_update)
{
#if CACHE_MODE == 0
	//find from cache
	//if cannot find from cache -> find from kvs
	//if find from cache -> return
	uint32_t hv;
	hv = hash(key, nkey);
	item_lock(hv);
	item *it = do_item_get(key, nkey, hv, c, do_update);
	if (it == NULL) {
		int kvs_ret;
		char val[2048];
		size_t val_len = 2048;
		kvs_ret = kvs->Get(key, nkey, val, val_len);
		if (kvs_ret != 0) {
#if ERROR_TRACE	== 1
			printf("Get fail: key cannot be found\n");
#endif
			item_unlock(hv);
			return NULL;
		}

		//Cache store
		item *it;
		if (c != NULL)
			it = item_alloc(key, nkey, 0, realtime(0), val_len + 2);
		else
			it = item_alloc(key, nkey, 0, realtime(0), val_len);
		if (it == NULL) {
#if ERROR_TRACE	== 1
			printf("MC_get1: Allocation error!\n");
#endif
			item_unlock(hv);
			return NULL;
		}

		memcpy(ITEM_data(it), val, val_len);
		if (c != NULL)
			memcpy((char*)ITEM_data(it) + val_len, "\r\n", 2);

		do_item_link(it, hv);
		item_unlock(hv);
		return it;
	} else {
		HIT();
		item_unlock(hv);
		return it;
	}
#elif CACHE_MODE == 4
        // always return from kvs
        int kvs_ret;
        char val[2048];
        size_t val_len = 2048;
        kvs_ret = kvs->Get(key, nkey, val, val_len);
        if (kvs_ret != 0) {
#if ERROR_TRACE	== 1
            printf("Get fail: key cannot be found\n");
#endif
            return NULL;
        }

        item *it;
        it = item_alloc(key, nkey, 0, realtime(0), val_len + 2);
        if (it == NULL) {
#if ERROR_TRACE	== 1
            printf("MC_get1: Allocation error!\n");
#endif
            return NULL;
        }

        memcpy(ITEM_data(it), val, val_len);
        memcpy((char*)ITEM_data(it) + val_len, "\r\n", 2);
        return it;
#elif CACHE_MODE == 5
        return item_get_internal(key, nkey, c, do_update);
#elif CACHE_MODE == 1
	//Find key from mc
	//Compare version between KVS and mc
	//If versions are same, return the value
	//Else if versions are different, update existing item and return
	uint32_t hv;
	int kvs_ret;
	hv = hash(key, nkey);
	item_lock(hv);
	item *res = do_item_get(key, nkey, hv, c, do_update);
	if (res != NULL) {
#if VERSION_MODE == 0
            HIT();
            item_unlock(hv);
            return res;
#else
            HIT();
            char val[2048];
            size_t val_len = 2048;
            Gptr skey = res->skey;
            TagGptr val_ptr_;
            val_ptr_.gptr_ = res->val_ptr.vptr;
            val_ptr_.tag_ = res->val_ptr.tag;

            do_item_remove(res);
            item_unlock(hv);
#if VERSION_MODE == 1
            kvs_ret = kvs->Get(skey, val_ptr_, val, val_len);
#else
            kvs_ret = kvs->Get(skey, val_ptr_, val, val_len, true); // force reading value from FAM
#endif
            item_lock(hv);
            res = do_item_get(key, nkey, hv, c, do_update);
            if (res == NULL) {
                goto full_null;
            }

            if (kvs_ret == 0 && val_ptr_.gptr_ == 0) {
                //This case is that the data we found is previously deleted data. 
                //Even though we return NULL now, cache data is kept for following search.
#if ERROR_TRACE == 1
                printf("MC_get: gptr is invalid or 0\n");
#endif
                do_item_remove(res);
                res = NULL;
            } else if (kvs_ret == 0) {
#if VERSION_MODE == 1
                if (res->val_ptr.tag < val_ptr_.tag_) {
                    if (c) {
                        // version mismatch
                        c->thread->stats.decr_misses++;
                    }
#else
                if (true) {
#endif
                        item *it = res;
                        if (c != NULL) {
                            if (res->nbytes != val_len + 2) {
                                res = item_alloc(key, nkey, 0, realtime(0), val_len + 2);
                                if (res == NULL) {
#if ERROR_TRACE	== 1
                                    printf("MC_get: Allocation error1!\n");
#endif
                                    item_unlock(hv);
                                    return NULL;
                                }
                                do_item_replace(it, res, hv);
                                do_item_remove(it);
                            }
                        } else {
                            if (res->nbytes != val_len) {
                                res = item_alloc(key, nkey, 0, realtime(0), val_len);
                                if (res == NULL) {
#if ERROR_TRACE	== 1
                                    printf("MC_get: Allocation error1!\n");
#endif
                                    item_unlock(hv);
                                    return NULL;
                                }
                                do_item_replace(it, res, hv);
                                do_item_remove(it);
                            }
                        }

                        res->skey = skey;
                        res->val_ptr.vptr = val_ptr_.gptr_;
                        res->val_ptr.tag = val_ptr_.tag_;
                        memcpy(ITEM_data(res), val, val_len);
                        if (c != NULL)
                            memcpy((char *)ITEM_data(res) + val_len, "\r\n", 2);
                    }
		} else {
#if ERROR_TRACE == 1
                    printf("MC_get: error occurs in only short-cut based search\n");
#endif
                    do_item_unlink(res, hv);
                    do_item_remove(res);
                    res = NULL;
		}
#endif
            } else {
            full_null:
                if (c) {
                    // get misses
                    c->thread->stats.get_misses++;                        
                }
		char val[2048];
		size_t val_len = 2048;
		Gptr skey;
		TagGptr val_ptr_;
		kvs_ret = kvs->Get(key, nkey, val, val_len, skey, val_ptr_);
		if (kvs_ret == 0) {
                    if (skey.IsValid() && val_ptr_.IsValid()) {
                        if (c != NULL)
                            res = item_alloc(key, nkey, 0, realtime(0), val_len + 2);
                        else
                            res = item_alloc(key, nkey, 0, realtime(0), val_len);
                        if (res == NULL) {
#if ERROR_TRACE	== 1
                            printf("MC_get: Allocation error2!\n");
#endif
                            item_unlock(hv);
                            return NULL;
                        }

                        do_item_link(res, hv);
                        res->skey = skey;
                        res->val_ptr.vptr = val_ptr_.gptr_;
                        res->val_ptr.tag = val_ptr_.tag_;
                        memcpy(ITEM_data(res), val, val_len);
                        if (c != NULL)
                            memcpy((char*)ITEM_data(res) + val_len, "\r\n", 2);
                    }
		}
            }
                
            item_unlock(hv);
            return res;
#elif CACHE_MODE == 2
	//Find key from mc
	//Compare version between KVS and mc
	//If versions are same, return the value
	//Else if versions are different, update existing item and return
	uint32_t hv;
	int kvs_ret;
	hv = hash(key, nkey);
	item_lock(hv);
	item *res = do_item_get(key, nkey, hv, c, DO_UPDATE);
	if (res != NULL) {
		HIT();
		char val[2048];
		size_t val_len = 2048;
		Gptr skey = res->skey;
		TagGptr val_ptr_;
		val_ptr_.gptr_ = res->val_ptr.vptr;
		val_ptr_.tag_ = res->val_ptr.tag;
		
		do_item_remove(res);
		item_unlock(hv);
		kvs_ret = kvs->Get(skey, val_ptr_, val, val_len, true);

		item_lock(hv);
		res = do_item_get(key, nkey, hv, c, DO_UPDATE);
		if (res == NULL) {
			goto short_null;
		}

		if (kvs_ret == 0 && val_ptr_.gptr_ == 0) {
			//This case is that the data we found is previously deleted data. 
			//Even though we return NULL now, cache data is kept for following search.
#if ERROR_TRACE == 1
			printf("MC_get: gptr is invalid or 0\n");
#endif
			res = NULL;
		} else if (kvs_ret == 0) {
			item *it;
			if (res->val_ptr.tag <= val_ptr_.tag_) {
				res->skey = skey;
				res->val_ptr.vptr = val_ptr_.gptr_;
				res->val_ptr.tag = val_ptr_.tag_;
				if (c != NULL)
					it = item_alloc(ITEM_key(res), res->nkey, 0, realtime(0), val_len + 2);
				else
					it = item_alloc(ITEM_key(res), res->nkey, 0, realtime(0), val_len);
				if (it == NULL) {
#if ERROR_TRACE	== 1
					printf("MC_get: Allocation error #2\n");
#endif
					item_unlock(hv);
					return NULL;
				}
				memcpy(ITEM_data(it), val, val_len);
				if (c != NULL)
					memcpy((char *)ITEM_data(it) + val_len, "\r\n", 2);
			} else {
				kvs_ret = kvs->Get(skey, val_ptr_, val, val_len, true);
				res->skey = skey;
				res->val_ptr.vptr = val_ptr_.gptr_;
				res->val_ptr.tag = val_ptr_.tag_;
				if (c != NULL)
					it = item_alloc(ITEM_key(res), res->nkey, 0, realtime(0), val_len + 2);
				else
					it = item_alloc(ITEM_key(res), res->nkey, 0, realtime(0), val_len);
				if (it == NULL) {
#if ERROR_TRACE	== 1
					printf("MC_get: Allocation error #1\n");
#endif
					item_unlock(hv);
					return NULL;
				}
				memcpy(ITEM_data(it), val, val_len);
				if (c != NULL)
					memcpy((char *)ITEM_data(it) + val_len, "\r\n", 2);
			}
			do_item_remove(res);
			res = it;
		} else {
#if ERROR_TRACE == 1
			printf("MC_get: error occurs in only short-cut based search\n");
#endif
			do_item_unlink(res, hv);
			do_item_remove(res);
			res = NULL;
		}
	} else {
short_null:
		char val[2048];
		size_t val_len = 2048;
		Gptr skey;
		TagGptr val_ptr_;
		kvs_ret = kvs->Get(key, nkey, val, val_len, skey, val_ptr_);
		if (kvs_ret == 0) {
			if (skey.IsValid() && val_ptr_.IsValid()) {
				res = item_alloc(key, nkey, 0, realtime(0), 2);

				if (res != NULL) {
					do_item_link(res, hv);
					res->skey = skey;
					res->val_ptr.vptr = val_ptr_.gptr_;
					res->val_ptr.tag = val_ptr_.tag_;
					memcpy(ITEM_data(res), "\r\n", 2);
					refcount_decr(res);
				} else {
#if ERROR_TRACE	== 1
					printf("MC_get: Allocation error #3\n");
#endif
				}

				item *it;
				if (c != NULL)
					it = item_alloc(key, nkey, 0, realtime(0), val_len + 2);
				else
					it = item_alloc(key, nkey, 0, realtime(0), val_len);
				if (it == NULL) {
#if ERROR_TRACE	== 1
					printf("MC_get: Allocation error #4\n");
#endif
					item_unlock(hv);
					return NULL;
				}
				memcpy(ITEM_data(it), val, val_len);
				if (c != NULL)
					memcpy((char *)ITEM_data(it) + val_len, "\r\n", 2);
				res = it;
			}
		}
	}
	item_unlock(hv);
	return res;
#elif CACHE_MODE == 3
	//Find key from mc
	//Compare version between KVS and mc
	//If versions are same, return the value
	//Else if versions are different, update existing item and return
	uint32_t hv;
	int kvs_ret;
	hv = hash(key, nkey);
	item_lock(hv);
	item *res = do_item_get(key, nkey, hv, c, do_update);
	if (res != NULL) {
		if (res->nbytes != 2) {
#if VERSION_MODE == 0
			HIT();
			item_unlock(hv);
			return res;
#else
			HIT();
			char val[2048];
			size_t val_len = 2048;
			Gptr skey = res->skey;
			TagGptr val_ptr_;
			val_ptr_.gptr_ = res->val_ptr.vptr;
			val_ptr_.tag_ = res->val_ptr.tag;

			do_item_remove(res);
			item_unlock(hv);
			kvs_ret = kvs->Get(skey, val_ptr_, val, val_len);

			item_lock(hv);
			res = do_item_get(key, nkey, hv, c, do_update);
			if (res == NULL) {
				goto hybrid_null;
			} else if (res->nbytes == 2) {
				item *it;
				if (res->val_ptr.tag >= val_ptr_.tag_) {
					val_len = 2048;
					kvs_ret = kvs->Get(skey, val_ptr_, val, val_len, true);
				}

				if (c != NULL)
					it = item_alloc(ITEM_key(res), res->nkey, 0, realtime(0), val_len + 2);
				else
					it = item_alloc(ITEM_key(res), res->nkey, 0, realtime(0), val_len);

				if (it == NULL) {
#if ERROR_TRACE	== 1
					printf("MC_get: Allocation error #2\n");
#endif
					item_unlock(hv);
					return NULL;
				}

				it->skey = skey;
				it->val_ptr.vptr = val_ptr_.gptr_;
				it->val_ptr.tag = val_ptr_.tag_;
				memcpy(ITEM_data(it), val, val_len);
				if (c != NULL)
					memcpy((char *)ITEM_data(it) + val_len, "\r\n", 2);

				do_item_replace(res, it, hv);
				do_item_remove(res);
				res = it;
				item_unlock(hv);
				return res;
			}

			if (kvs_ret == 0 && val_ptr_.gptr_ == 0) {
				//This case is that the data we found is previously deleted data. 
				//Even though we return NULL now, cache data is kept for following search.
#if ERROR_TRACE == 1
				printf("MC_get: gptr is invalid or 0\n");
#endif
				do_item_remove(res);
				res = NULL;
			} else if (kvs_ret == 0) {
				if (res->val_ptr.tag < val_ptr_.tag_) {
					item *it = res;
					if (c != NULL) {
						if (res->nbytes != val_len + 2) {
							res = item_alloc(key, nkey, 0, realtime(0), val_len + 2);
							if (res == NULL) {
#if ERROR_TRACE	== 1
								printf("MC_get: Allocation error1!\n");
#endif
								item_unlock(hv);
								return NULL;
							}
						}
					} else {
						if (res->nbytes != val_len) {
							res = item_alloc(key, nkey, 0, realtime(0), val_len);
							if (res == NULL) {
#if ERROR_TRACE	== 1
								printf("MC_get: Allocation error1!\n");
#endif
								item_unlock(hv);
								return NULL;
							}
						}
					}
					do_item_replace(it, res, hv);
					do_item_remove(it);

					res->skey = skey;
					res->val_ptr.vptr = val_ptr_.gptr_;
					res->val_ptr.tag = val_ptr_.tag_;
					memcpy(ITEM_data(res), val, val_len);
					if (c != NULL)
						memcpy((char *)ITEM_data(res) + val_len, "\r\n", 2);
				}
			} else {
#if ERROR_TRACE == 1
				printf("MC_get: error occurs in only short-cut based search\n");
#endif
				do_item_unlink(res, hv);
				do_item_remove(res);
				res = NULL;
			}
#endif
		} else {
			HIT();
			char val[2048];
			size_t val_len = 2048;
			Gptr skey = res->skey;
			TagGptr val_ptr_;
			val_ptr_.gptr_ = res->val_ptr.vptr;
			val_ptr_.tag_ = res->val_ptr.tag;

			do_item_remove(res);
			item_unlock(hv);
			kvs_ret = kvs->Get(skey, val_ptr_, val, val_len, true);

			item_lock(hv);
			res = do_item_get(key, nkey, hv, c, DO_UPDATE);
			if (res == NULL) {
				goto hybrid_null;
			} else if (res->nbytes != 2) {
				if (res->val_ptr.tag < val_ptr_.tag_) {
					item *it = res;
					if (c != NULL) {
						if (res->nbytes != val_len + 2) {
							res = item_alloc(key, nkey, 0, realtime(0), val_len + 2);
							if (res == NULL) {
#if ERROR_TRACE	== 1
								printf("MC_get: Allocation error1!\n");
#endif
								item_unlock(hv);
								return NULL;
							}
							do_item_replace(it, res, hv);
							do_item_remove(it);
						}
					} else {
						if (res->nbytes != val_len) {
							res = item_alloc(key, nkey, 0, realtime(0), val_len);
							if (res == NULL) {
#if ERROR_TRACE	== 1
								printf("MC_get: Allocation error1!\n");
#endif
								item_unlock(hv);
								return NULL;
							}
							do_item_replace(it, res, hv);
							do_item_remove(it);
						}
					}

					res->skey = skey;
					res->val_ptr.vptr = val_ptr_.gptr_;
					res->val_ptr.tag = val_ptr_.tag_;
					memcpy(ITEM_data(res), val, val_len);
					if (c != NULL)
						memcpy((char *)ITEM_data(res) + val_len, "\r\n", 2);
				}
				item_unlock(hv);
				return res;
			}

			if (kvs_ret == 0 && val_ptr_.gptr_ == 0) {
				//This case is that the data we found is previously deleted data. 
				//Even though we return NULL now, cache data is kept for following search.
#if ERROR_TRACE == 1
				printf("MC_get: gptr is invalid or 0\n");
#endif
				res = NULL;
			} else if (kvs_ret == 0) {
				item *it;
				if (res->val_ptr.tag > val_ptr_.tag_) {
					val_len = 2048;
					kvs_ret = kvs->Get(skey, val_ptr_, val, val_len, true);
				}

				if (c != NULL)
					it = item_alloc(ITEM_key(res), res->nkey, 0, realtime(0), val_len + 2);
				else
					it = item_alloc(ITEM_key(res), res->nkey, 0, realtime(0), val_len);
				if (it == NULL) {
#if ERROR_TRACE	== 1
					printf("MC_get: Allocation error #2\n");
#endif
					item_unlock(hv);
					return NULL;
				}

				it->skey = skey;
				it->val_ptr.vptr = val_ptr_.gptr_;
				it->val_ptr.tag = val_ptr_.tag_;
				memcpy(ITEM_data(it), val, val_len);
				if (c != NULL)
					memcpy((char *)ITEM_data(it) + val_len, "\r\n", 2);

				do_item_replace(res, it, hv);
				do_item_remove(res);
				res = it;
			} else {
#if ERROR_TRACE == 1
				printf("MC_get: error occurs in only short-cut based search\n");
#endif
				do_item_unlink(res, hv);
				do_item_remove(res);
				res = NULL;
			}
		}
	} else {
hybrid_null:
		char val[2048];
		size_t val_len = 2048;
		Gptr skey;
		TagGptr val_ptr_;
		kvs_ret = kvs->Get(key, nkey, val, val_len, skey, val_ptr_);
		if (kvs_ret == 0) {
			if (skey.IsValid() && val_ptr_.IsValid()) {
				if (c != NULL)
					res = item_alloc(key, nkey, 0, realtime(0), val_len + 2);
				else
					res = item_alloc(key, nkey, 0, realtime(0), val_len);
				if (res == NULL) {
#if ERROR_TRACE	== 1
					printf("MC_get: Allocation error2!\n");
#endif
					item_unlock(hv);
					return NULL;
				}

				do_item_link(res, hv);
				res->skey = skey;
				res->val_ptr.vptr = val_ptr_.gptr_;
				res->val_ptr.tag = val_ptr_.tag_;
				memcpy(ITEM_data(res), val, val_len);
				if (c != NULL)
					memcpy((char*)ITEM_data(res) + val_len, "\r\n", 2);
			}
		}
	}
	item_unlock(hv);
	return res;
#endif
}

enum store_item_type store_item(item *it, int comm, conn* c)
{
#if CACHE_MODE == 0
	//put to kvs first
	//put to cache second
    uint32_t hv;
    hv = hash(ITEM_key(it), it->nkey);
    item_lock(hv);
	int kvs_ret;
	if (c != NULL)
		kvs_ret = kvs->Put(ITEM_key(it), it->nkey, ITEM_data(it), it->nbytes - 2);
	else
		kvs_ret = kvs->Put(ITEM_key(it), it->nkey, ITEM_data(it), it->nbytes);

	if (kvs_ret == 0) {
		do_store_item(it, comm, c, hv);
		item_unlock(hv);
		return STORED;
	} else {
#if ERROR_TRACE == 1
		printf("KVS_Put: Put Failed\n");
#endif
		item_unlock(hv);
		return NOT_STORED;
	}
#elif CACHE_MODE == 4
	//put to kvs first
	int kvs_ret;
        kvs_ret = kvs->Put(ITEM_key(it), it->nkey, ITEM_data(it), it->nbytes - 2);
	if (kvs_ret == 0) {
            return STORED;
	} else {
#if ERROR_TRACE == 1
            printf("KVS_Put: Put Failed\n");
#endif
            return NOT_STORED;
	}
#elif CACHE_MODE == 5
        return store_item_internal(it, comm, c);
#elif ((CACHE_MODE == 1) || (CACHE_MODE==3))
	int kvs_ret;
	uint32_t hv;
	hv = hash(ITEM_key(it), it->nkey);
	item_lock(hv);
	item *res = do_item_get(ITEM_key(it), it->nkey, hv, c, DONT_UPDATE);
	if (res != NULL) {
		HIT();
		Gptr old_skey = res->skey;
		TagGptr old_val_ptr;
		old_val_ptr.gptr_ = res->val_ptr.vptr;
		old_val_ptr.tag_ = res->val_ptr.tag;

		if (c != NULL)
			kvs_ret = kvs->Put(old_skey, old_val_ptr, ITEM_data(it), it->nbytes - 2);
		else
			kvs_ret = kvs->Put(old_skey, old_val_ptr, ITEM_data(it), it->nbytes);
		if (kvs_ret == 0) {
			if (!(old_skey.IsValid()) || !(old_val_ptr.IsValid())) {
#if ERROR_TRACE == 1
				printf("Put error: case 1\n");
#endif
				refcount_decr(res);
				item_unlock(hv);
				return NOT_STORED;
			}

			if (res->val_ptr.tag != old_val_ptr.tag_ || res->val_ptr.vptr != old_val_ptr.gptr_) {
				do_item_replace(res, it, hv);
				do_item_remove(res);
				it->skey = old_skey;
				it->val_ptr.vptr = old_val_ptr.gptr_;
				it->val_ptr.tag = old_val_ptr.tag_;
			}
			item_unlock(hv);
			return STORED;
		} else {
#if ERROR_TRACE == 1
			printf("Put error: case 2\n");
#endif
			refcount_decr(res);
			item_unlock(hv);
			return NOT_STORED;
		}
	}
        else {
            if (c) {
                // put misses
                c->thread->stats.cas_misses++;                        
            }            
        }

	Gptr skey = 0;
	TagGptr val_ptr_;
	val_ptr_.gptr_ = 0;
	val_ptr_.tag_ = 0;

	if (c != NULL)
		kvs_ret = kvs->Put(ITEM_key(it), it->nkey, ITEM_data(it), it->nbytes - 2, skey, val_ptr_);
	else
		kvs_ret = kvs->Put(ITEM_key(it), it->nkey, ITEM_data(it), it->nbytes, skey, val_ptr_);
	if (kvs_ret == 0) {
		if (!(skey.IsValid()) || !(val_ptr_.IsValid())) {
#if ERROR_TRACE == 1
			printf("Put error: case 3\n");
#endif
			item_unlock(hv);
			return NOT_STORED;
		}

		it->skey = skey;
		it->val_ptr.tag = val_ptr_.tag_;
		it->val_ptr.vptr = val_ptr_.gptr_;

		do_item_link(it, hv);
		item_unlock(hv);
		return STORED;
	} else {
#if ERROR_TRACE == 1
		printf("Put error: case 4\n");
#endif
		item_unlock(hv);
		return NOT_STORED;
	}
#elif CACHE_MODE == 2
	int kvs_ret;
	uint32_t hv;
	hv = hash(ITEM_key(it), it->nkey);
	item_lock(hv);
	item *res = do_item_get(ITEM_key(it), it->nkey, hv, c, DONT_UPDATE);
	if (res != NULL) {
		HIT();
		Gptr old_skey = res->skey;
		TagGptr old_val_ptr;
		old_val_ptr.gptr_ = res->val_ptr.vptr;
		old_val_ptr.tag_ = res->val_ptr.tag;

		if (c != NULL)
			kvs_ret = kvs->Put(old_skey, old_val_ptr, ITEM_data(it), it->nbytes - 2);
		else
			kvs_ret = kvs->Put(old_skey, old_val_ptr, ITEM_data(it), it->nbytes);

		if (kvs_ret == 0) {
			if (!(old_skey.IsValid()) || !(old_val_ptr.IsValid())) {
				refcount_decr(res);
				item_unlock(hv);
#if ERROR_TRACE == 1
                                printf("Put error: cache_mode 2 case 0\n");
#endif
				return NOT_STORED;
			}

			if (res->val_ptr.tag != old_val_ptr.tag_ || res->val_ptr.vptr != old_val_ptr.gptr_) {
				res->skey = old_skey;
				res->val_ptr.vptr = old_val_ptr.gptr_;
				res->val_ptr.tag = old_val_ptr.tag_;
			}
			refcount_decr(res);
			item_unlock(hv);
			return STORED;
		} else {
			refcount_decr(res);
			item_unlock(hv);
#if ERROR_TRACE == 1
                        printf("Put error: cache_mode 2 case 1\n");
#endif
			return NOT_STORED;
		}
	}

	Gptr skey = 0;
	TagGptr val_ptr_;
	val_ptr_.gptr_ = 0;
	val_ptr_.tag_ = 0;

	if (c != NULL)
		kvs_ret = kvs->Put(ITEM_key(it), it->nkey, ITEM_data(it), it->nbytes - 2, skey, val_ptr_);
	else
		kvs_ret = kvs->Put(ITEM_key(it), it->nkey, ITEM_data(it), it->nbytes, skey, val_ptr_);

	if (kvs_ret == 0) {
		if (!(skey.IsValid()) || !(val_ptr_.IsValid())) {
			item_unlock(hv);
#if ERROR_TRACE == 1
                        printf("Put error: cache_mode 2 case 2\n");
#endif
			return NOT_STORED;
		}

		res = item_alloc(ITEM_key(it), it->nkey, 0, realtime(0), 2);
		if (res != NULL) {
			memcpy(ITEM_data(res), "\r\n", 2);

			res->skey = skey;
			res->val_ptr.tag = val_ptr_.tag_;
			res->val_ptr.vptr = val_ptr_.gptr_;

			do_item_link(res, hv);
			refcount_decr(res);
		}
		item_unlock(hv);
		return STORED;
	} else {
		item_unlock(hv);
#if ERROR_TRACE == 1
                printf("Put error: cache_mode 2 case 3\n");
#endif
		return NOT_STORED;
	}
#endif
}

void item_remove(item *item) 
{
#if CACHE_MODE == 4
    do_item_remove(item);
#else
    item_remove_internal(item);
#endif
}

void item_unlink(item *item) 
{
#if CACHE_MODE == 0
    uint32_t hv;
    hv = hash(ITEM_key(item), item->nkey);
    item_lock(hv);
    do_item_unlink(item, hv);
    kvs->Del(ITEM_key(item), item->nkey);
    item_unlock(hv);
#elif CACHE_MODE == 4
    kvs->Del(ITEM_key(item), item->nkey);
#elif CACHE_MODE == 5
    item_unlink_internal(item);
#else
    uint32_t hv;
    hv = hash(ITEM_key(item), item->nkey);
    item_lock(hv);
    do_item_unlink(item, hv);
    Gptr skey = item->skey;
    TagGptr val_ptr_;
    val_ptr_.gptr_ = item->val_ptr.vptr;
    val_ptr_.tag_ = item->val_ptr.tag;
    kvs->Del(skey, val_ptr_);
    item_unlock(hv);
#endif
}

int item_unlink_kvs(const char *key, const size_t nkey)
{
	int kvs_ret;
	kvs_ret = kvs->Del(key, nkey);
	return kvs_ret;
}

void Cache_Init(size_t cache_size)
{
    //std::cout << " hash table : " << HASHPOWER_DEFAULT << std::endl;
	settings_init();
//	settings.maxbytes = ((size_t)cache_size) * 1024 * 1024UL;
	settings.maxbytes = (size_t)cache_size;
	settings.hashpower_init = 27;
	settings.factor = 1.6;
	settings.verbose = 1;
        //settings.verbose = 3;
	settings.num_threads = 0;

	// These options are same with "-o modern" of memcached server
	settings.slab_reassign = true;
	settings.slab_automove = 1;
	settings.maxconns_fast = true;
	settings.inline_ascii_response = false;
	settings.lru_segmented = true;
	enum hashfunc_type hash_type = MURMUR3_HASH;
	bool start_lru_crawler = false;
	bool start_lru_maintainer = true;

	init_lru_crawler();
	init_lru_maintainer();

	hash_init(hash_type);
	assoc_init(settings.hashpower_init);

	slabs_init(settings.maxbytes, settings.factor, 0, use_slab_sizes ? slab_sizes : NULL);
	logger_init();
	memcached_thread_init(1);

	if (start_assoc_maintenance_thread() == -1) {
		printf("maintenance_thread start error\n");
	}

	if (start_lru_crawler && start_item_crawler_thread() != 0) {
		fprintf(stderr, "Failed to enable LRU crawler thread\n");
		exit(EXIT_FAILURE);
	}

	if (start_lru_maintainer && start_lru_maintainer_thread() != 0) {
		fprintf(stderr, "Failed to enable LRU maintainer thread\n");
		exit(EXIT_FAILURE);
	}

	if (settings.slab_reassign &&
			start_slab_maintenance_thread() == -1) {
		exit(EXIT_FAILURE);
	}
}

void Cache_Final()
{
    stop_assoc_maintenance_thread();
//  stop_item_crawler_thread();
    stop_lru_maintainer_thread();
    stop_slab_maintenance_thread();
}

nvmm::GlobalPtr str2gptr(std::string root_str) {
	std::string delimiter = ":";
	size_t loc = root_str.find(delimiter);
	if (loc==std::string::npos)
		return 0;
	std::string shelf_id = root_str.substr(1, loc-1);
	std::string offset = root_str.substr(loc+1, root_str.size()-3-shelf_id.size());
	return nvmm::GlobalPtr((unsigned char)std::stoul(shelf_id), std::stoul(offset));
}


/* 
   Public APIs
*/

// Local Mode
void KVS_Final() {
#if CACHE_MODE !=5
	if (kvs != NULL) {
        kvs->ReportMetrics();
		delete kvs;
    }
#endif
//	slabs_print();
//	double hit_rate = (double)(num_hit / 5000000) * 100;
//	std::cout << "hit_rate = " << hit_rate << std::endl;
	std::cout << "hit = " << num_hit << std::endl;
#if CACHE_MODE !=4
        Cache_Final();
#endif
}


// Local Mode
void KVS_Init(radixtree::KeyValueStore::IndexType type, std::string root, std::string base, std::string user, size_t size, size_t cache_size)
{
	num_hit = 0;
#if CACHE_MODE !=4
        Cache_Init(cache_size);
        std::cout << "Cache Size (MB): " << cache_size/1024UL/1024UL << std::endl;
#endif
	std::cout << "FAM Size (GB) : " << (size/1024UL/1024UL/1024UL) << std::endl;

#if CACHE_MODE == 0
	std::cout << "Cache mode: original memcached" << std::endl;
#elif CACHE_MODE == 1
	std::cout << "Cache mode: full" << std::endl;
#elif CACHE_MODE == 2
	std::cout << "Cache mode: short" << std::endl;
#elif CACHE_MODE == 3
	std::cout << "Cache mode: hybrid" << std::endl;
#elif CACHE_MODE == 4
	std::cout << "Cache mode: kvs only" << std::endl;
#elif CACHE_MODE == 5
	std::cout << "Cache mode: memcached only" << std::endl;
#endif

#if VERSION_MODE == 0
	std::cout << "Versioned mode: Off" << std::endl;
#else
	std::cout << "Versioned mode: On" << std::endl;
#endif

#if CACHE_MODE != 5
	KeyValueStore::Start(base, user);
	nvmm::GlobalPtr root_ptr = str2gptr(root);
	if(root_ptr==0)
		KeyValueStore::Reset(base, user);

	KeyValueStore::Start(base, user);
	kvs = KeyValueStore::MakeKVS(type, root_ptr, base, user, size);
	if (kvs == NULL) {
		printf("KVS_Init: Init error\n");
		exit(1);
	}
	std::cout << "KVS Type: " << type << std::endl;
	std::cout << "KVS_Init: KVS opened at " << kvs->Location() << std::endl;
#endif
}

void KVS_Init(std::string type, std::string root, std::string base, std::string user, size_t size, size_t cache_size)
{
	num_hit = 0;
#if CACHE_MODE !=4
        Cache_Init(cache_size);
        std::cout << "Cache Size (MB): " << cache_size/1024UL/1024UL << std::endl;
#endif
	std::cout << "FAM Size (GB) : " << (size/1024UL/1024UL/1024UL) << std::endl;

#if CACHE_MODE == 0
	std::cout << "Cache mode: original memcached" << std::endl;
#elif CACHE_MODE == 1
	std::cout << "Cache mode: full" << std::endl;
#elif CACHE_MODE == 2
	std::cout << "Cache mode: short" << std::endl;
#elif CACHE_MODE == 3
	std::cout << "Cache mode: hybrid" << std::endl;
#elif CACHE_MODE == 4
	std::cout << "Cache mode: kvs only" << std::endl;
#elif CACHE_MODE == 5
	std::cout << "Cache mode: memcached only" << std::endl;
#endif

#if VERSION_MODE == 0
	std::cout << "Versioned mode: Off" << std::endl;
#else
	std::cout << "Versioned mode: On" << std::endl;
#endif

#if CACHE_MODE != 5
	KeyValueStore::Start(base, user);
	nvmm::GlobalPtr root_ptr = str2gptr(root);
	if(root_ptr==0)
		KeyValueStore::Reset(base, user);

	KeyValueStore::Start(base, user);
	kvs = KeyValueStore::MakeKVS(type, root_ptr, base, user, size);
	if (kvs == NULL) {
		printf("KVS_Init: Init error\n");
		exit(1);
	}
	std::cout << "KVS Type: " << type << std::endl;
	std::cout << "KVS_Init: KVS opened at " << kvs->Location() << std::endl;
#endif
}

// Server Mode
// don't need Cache_Init
void KVS_Init(radixtree::KVS kvs_config)
{
	num_hit = 0;
        std::cout << "Cache Size (MB): " << std::stoul(kvs_config["cache_size"])/1024UL/1024UL << std::endl;
        std::cout << "FAM Size (GB) : " << std::stoul(kvs_config["kvs_size"])/1024UL/1024UL/1024UL << std::endl;

#if CACHE_MODE == 0
	std::cout << "Cache mode: original memcached" << std::endl;
#elif CACHE_MODE == 1
	std::cout << "Cache mode: full" << std::endl;
#elif CACHE_MODE == 2
	std::cout << "Cache mode: short" << std::endl;
#elif CACHE_MODE == 3
	std::cout << "Cache mode: hybrid" << std::endl;
#elif CACHE_MODE == 4
	std::cout << "Cache mode: kvs only" << std::endl;
#elif CACHE_MODE == 5
	std::cout << "Cache mode: memcached only" << std::endl;
#endif

#if VERSION_MODE == 0
	std::cout << "Versioned mode: Off" << std::endl;
#else
	std::cout << "Versioned mode: On" << std::endl;
#endif

#if CACHE_MODE != 5
	std::string loc=kvs_config["kvs_root"];
	nvmm::GlobalPtr location;
	if(loc.empty() || str2gptr(loc)==0) {
            std::cout << "KVS_Init: invalid location" << std::endl;
            exit(1);
	}
	else {
            location=str2gptr(loc);
	}

	std::string type=kvs_config["kvs_type"];
	std::string base=kvs_config["shelf_base"];
	std::string user=kvs_config["shelf_user"];
	std::string size=kvs_config["kvs_size"];
	kvs = KeyValueStore::MakeKVS(type, location, base, user, (size_t)stoul(size));
	if (kvs == NULL) {
            printf("KVS_Init: Init error\n");
            exit(1);
	}
	std::cout << "KVS Type: " << type << std::endl;
	std::cout << "KVS_Init: KVS opened at " << kvs->Location() << std::endl;
#endif
}

// Local Mode
int Cache_Put(char const *key, size_t const key_len, char const *val, size_t const val_len)
{
#if CACHE_MODE != 4
    item *it = item_alloc(key, key_len, 0, realtime(0), val_len);
    if (it == NULL) {
        printf("Cache_Put: item allocation error %d %d\n", key_len, val_len);
        return -1;
    }

    memcpy(ITEM_data(it), val, val_len);

    enum store_item_type ret = store_item(it, NREAD_SET, NULL);
    item_remove(it);
    if (ret == STORED || ret == EXISTS) {
        return 0;
    } else {
        printf("Cache_Put: Put error %d \n", ret);
        return -1;
    }
#else
    return kvs->Put(key, key_len, val, val_len);
#endif
}

int Cache_Get(char const *key, size_t const key_len, char *val, size_t &val_len)
{
#if CACHE_MODE != 4
    item *it = item_get(key, key_len, NULL, DO_UPDATE);
    if (it != NULL) {
        val_len = it->nbytes;
        memcpy(val, ITEM_data(it), it->nbytes);
        item_remove(it);
        return 0;
    } else {
        printf("Cache_Get: Get error\n");
        return -1;
    }
#else
    return kvs->Get(key, key_len, val, val_len);
#endif
}

int Cache_Del(char const *key, size_t const key_len)
{
#if CACHE_MODE != 4
    item *it = item_get_internal(key, key_len, NULL, DONT_UPDATE);
    if (it != NULL) {
        item_unlink(it);
        item_remove(it);
        return 0;
    }
#if CACHE_MODE == 5
    else {
        int ret;
        ret = item_unlink_kvs(key, key_len);
        return ret;
    }
#endif // CACHE_MODE == 5
#else // CACHE_MODE !=4
    return kvs->Del(key, key_len);
#endif
}

/*
   Sekwon's implementation

// Local Mode: KVS + cache
int Cache_Put(char const *key, size_t const key_len, char const *val, size_t const val_len)
{
    item *it = item_alloc(key, key_len, 0, realtime(0), val_len);
    if (it == NULL) {
        printf("Cache_Put: item allocation error\n");
        return -1;
    }

    memcpy(ITEM_data(it), val, val_len);

    enum store_item_type ret = store_item(it, NREAD_SET, NULL);
    if (ret == STORED) {
        item_remove(it);
        return 0;
    } else {
        printf("Cache_Put: Put error\n");
        return -1;
    }
}

int Cache_Get(char const *key, size_t const key_len, char *val, size_t &val_len)
{
    item *it = item_get(key, key_len, NULL, DO_UPDATE);
    if (it != NULL) {
        val_len = it->nbytes;
        memcpy(val, ITEM_data(it), it->nbytes);
        item_remove(it);
        return 0;
    } else {
        printf("Cache_Get: Get error\n");
        return -1;
    }
}

int Cache_Del(char const *key, size_t const key_len)
{
    item *it = item_get_internal(key, key_len, NULL, DONT_UPDATE);
    if (it != NULL) {
        item_unlink(it);
        item_remove(it);
        return 0;
    } else {
        int ret;
        ret = item_unlink_kvs(key, key_len);
        return ret;
    }
}


// Local Mode: cache only
int mc_Put(char const *key, size_t const key_len, char const *val, size_t const val_len)
{
    item *kv = item_alloc(key, key_len, 0, realtime(0), val_len);
    if (kv == NULL) {
        printf("mc_Put: item allocation error\n");
        return -1;
    }

    memcpy(ITEM_data(kv), val, val_len);

    enum store_item_type ret = store_item_internal(kv, NREAD_SET, NULL);
    item_remove_internal(kv);
    if (ret == STORED || ret == EXISTS) {
        return 0;
    } else {
        printf("mc_Put: Put error\n");
        return -1;
    }
}

int mc_Get(char const *key, size_t const key_len, char *val, size_t &val_len)
{
    item *res = item_get_internal(key, key_len, NULL, DO_UPDATE);
    if (res != NULL) {
        val_len = res->nbytes;
        memcpy(val, ITEM_data(res), res->nbytes);
        item_remove_internal(res);
        return 0;
    }
    else {
        printf("mc_Get: Get error\n");
        return -1;
    }
}

int mc_Del(char const *key, size_t const key_len)
{
    item *it = item_get_internal(key, key_len, NULL, DONT_UPDATE);
    if (it != NULL) {
        item_unlink_internal(it);
        item_remove_internal(it);
        return 0;
    }
}
*/ 
