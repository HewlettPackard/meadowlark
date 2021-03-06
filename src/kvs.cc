/*
 *  (c) Copyright 2016-2017, 2021 Hewlett Packard Enterprise Development Company LP.
 *
 *  This software is available to you under a choice of one of two
 *  licenses. You may choose to be licensed under the terms of the 
 *  GNU Lesser General Public License Version 3, or (at your option)  
 *  later with exceptions included below, or under the terms of the  
 *  MIT license (Expat) available in COPYING file in the source tree.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  As an exception, the copyright holders of this Library grant you permission
 *  to (i) compile an Application with the Library, and (ii) distribute the
 *  Application containing code generated by the Library and added to the
 *  Application during this compilation process under terms of your choice,
 *  provided you also meet the terms and conditions of the Application license.
 *
 */

#include <iostream>
#include <string>

#include "nvmm/global_ptr.h"
#include "nvmm/memory_manager.h"
#include "nvmm/epoch_manager.h"

#include "radixtree/kvs.h"
#include "radixtree/common.h" // TagGptr
#include "kvs_radix_tree.h"
#include "kvs_radix_tree_tiny.h"
#include "kvs_split_ordered.h"
#include "kvs_metrics_config.h"
#include "radix_tree_metrics.h"
#include "split_ordered_metrics.h"


namespace radixtree {

KeyValueStore *KeyValueStore::MakeKVS(IndexType type, nvmm::GlobalPtr location,
                                      std::string base, std::string user,
                                      size_t heap_size, nvmm::PoolId heap_id) {
    Start(base, user);

    KVSMetricsConfig kvs_metrics_config; // configuration through environment variables

    switch(type) {
    case RADIX_TREE: {
        RadixTreeMetrics* metrics = new RadixTreeMetrics(kvs_metrics_config);
        return new KVSRadixTree(location, base, user, heap_size, heap_id, metrics);
    }
    case RADIX_TREE_TINY: {
        RadixTreeMetrics* metrics = new RadixTreeMetrics(kvs_metrics_config);
        return new KVSRadixTreeTiny(location, base, user, heap_size, heap_id, metrics);
    }
    case HASH_TABLE: {
        SplitOrderedMetrics* metrics = new SplitOrderedMetrics(kvs_metrics_config);
        return new KVSSplitOrdered(location, base, user, heap_size, heap_id, metrics);
    }
    case INVALID:
    default:
        return nullptr;
    }
}

KeyValueStore *KeyValueStore::MakeKVS(std::string type_str, nvmm::GlobalPtr location,
                                      std::string base, std::string user,
                                      size_t heap_size, nvmm::PoolId heap_id) {
    IndexType type;
    if(type_str=="radixtree")
        type = RADIX_TREE;
    else if(type_str=="radixtree_tiny")
        type = RADIX_TREE_TINY;
    else if(type_str=="hashtable")
        type = HASH_TABLE;
    else
        type = INVALID;

    return MakeKVS(type, location, base, user, heap_size, heap_id);
}
    
void KeyValueStore::Start(std::string base, std::string user) {
    nvmm::StartNVMM(base, user);
}

void KeyValueStore::Reset(std::string base, std::string user) {
    nvmm::ResetNVMM(base, user);
}

void KeyValueStore::Restart(std::string base, std::string user) {
    nvmm::RestartNVMM(base, user);
}

} // namespace radixtree
