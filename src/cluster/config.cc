#include <cstddef> // size_t
#include <map>
#include <iostream>
#include <fstream>

#include "cluster/config.h"
#include "yaml-cpp/yaml.h"

namespace radixtree {

void Config::PrintConfigFile(std::string path) {
    YAML::Node file = YAML::LoadFile(path);
    if(file.IsNull()) {
        std::cout << "Cannot find the config file at " << path << std::endl;
        return;
    }

    YAML::Node const &nvmm = file["nvmm"];
    if(nvmm.IsNull()) {
        std::cout << "Invalid NVMM config format" << std::endl;
        return;
    }

    for (YAML::const_iterator it=nvmm.begin(); it!=nvmm.end(); it++) {
        std::string key = it->first.as<std::string>();
        YAML::Node const &config = nvmm[key];
        switch(config.Type()) {
        case YAML::NodeType::Scalar: {
            std::string value = it->second.as<std::string>();
            std::cout << key << ": " << value << std::endl;
            break;
        }
        case YAML::NodeType::Sequence: {
            std::cout << "Sequence..." << std::endl;
            break;
        }
        case YAML::NodeType::Map: {
            std::cout << key << ": " << std::endl;
            for (YAML::const_iterator j=config.begin(); j!=config.end(); j++) {
                std::cout << " " << j->first.as<std::string>() << ": " << j->second.as<std::string>() << std::endl;
            }
            break;
        }
        case YAML::NodeType::Null:
        case YAML::NodeType::Undefined:
        default:
            std::cout << "Default..." << std::endl;
            break;
        }
    }

    YAML::Node const &kvs = file["kvs"];
    if(kvs.IsNull()) {
        std::cout << "Invalid KVS format" << std::endl;
        return;
    }

    for (YAML::const_iterator it=kvs.begin(); it!=kvs.end(); it++) {
        std::string key = it->first.as<std::string>();
        YAML::Node const &config = kvs[key];
        switch(config.Type()) {
        case YAML::NodeType::Scalar: {
            std::string value = it->second.as<std::string>();
            std::cout << key << ": " << value << std::endl;
            break;
        }
        case YAML::NodeType::Sequence: {
            std::cout << "Sequence..." << std::endl;
            break;
        }
        case YAML::NodeType::Map: {
            std::cout << key << ": " << std::endl;
            for (YAML::const_iterator j=config.begin(); j!=config.end(); j++) {
                if(j->second.Type()==YAML::NodeType::Map) {
                    for (YAML::const_iterator k=j->second.begin(); k!=j->second.end(); k++) {
                        std::cout << "   " << k->first.as<std::string>() << ": " << k->second.as<std::string>() << std::endl;
                    }
                }
                else {
                    std::cout << " " << j->first.as<std::string>() << ": " << j->second.as<std::string>() << std::endl;
                }
            }
            break;
        }
        case YAML::NodeType::Null:
        case YAML::NodeType::Undefined:
        default:
            std::cout << "Default..." << std::endl;
            break;
        }
    }
}

int Config::LoadConfigFile(std::string path) {
    int ret=0;

    YAML::Node const &file = YAML::LoadFile(path);
    if(file.IsNull()) {
        std::cout << "Cannot find the config file at " << path << std::endl;
        return -1;
    }


    YAML::Node const &nvmm = file["nvmm"];
    if(nvmm.IsNull()) {
        std::cout << "Invalid NVMM config format" << std::endl;
        return -1;
    }

    if(nvmm["shelf_base"]) {
        SetShelfBase(nvmm["shelf_base"].as<std::string>());
    }
    if(nvmm["shelf_user"]) {
        SetShelfUser(nvmm["shelf_user"].as<std::string>());
    }

    YAML::Node const &kvs = file["kvs"];
    if(kvs.IsNull()) {
        std::cout << "Invalid KVS format" << std::endl;
        return -1;
    }

    if(kvs["partition_cnt"]) {
        SetPartitionCnt(kvs["partition_cnt"].as<size_t>());
    }
    if(kvs["server_cnt"]) {
        SetServerCnt(kvs["server_cnt"].as<size_t>());
    }
    if(kvs["node_cnt"]) {
        SetNodeCnt(kvs["node_cnt"].as<size_t>());
    }
    if(kvs["starting_port"]) {
        SetStartingPort(kvs["starting_port"].as<uint64_t>());
    }
    if(kvs["kvs_type"]) {
        SetKVSType(kvs["kvs_type"].as<std::string>());
    }
    if(kvs["kvs_size"]) {
        SetKVSSize(kvs["kvs_size"].as<size_t>());
    }
    if(kvs["cache_size"]) {
        SetCacheSize(kvs["cache_size"].as<size_t>());
    }
    if(kvs["server_thread"]) {
        SetServerThread(kvs["server_thread"].as<uint64_t>());
    }
    if(kvs["replication_scheme"]) {
        std::string str = kvs["replication_scheme"].as<std::string>();
        ReplicationScheme rs = str2rs(str);
        if(rs==ReplicationScheme::INVALID) {
            std::cout << "Invalid replication_scheme" << std::endl;
            ret=-1;
        }
        else
            SetReplicationScheme(rs);
    }
    if(kvs["replication_factor"]) {
        SetReplicationFactor(kvs["replication_factor"].as<uint64_t>());
    }
    if(kvs["partitions"]) {
        YAML::Node const &config = kvs["partitions"];
        for (YAML::const_iterator i=config.begin(); i!=config.end(); i++) {
            PartitionID pid = PartitionID(i->first.as<uint64_t>());
            KVS kvs = i->second.as<KVS>();
            int res = AddPartition(pid, kvs);
            if(res==-1) {
                std::cout << "Duplicate partition id " << PartitionID(i->first.as<uint64_t>()) << std::endl;
                ret=-1;
            }
        }
    }
    // if(kvs["nodes"]) {
    //     YAML::Node const &config = kvs["nodes"];
    //     for (YAML::const_iterator i=config.begin(); i!=config.end(); i++) {
    //         int res = AddNode(NodeID(i->first.as<uint64_t>()), Port(i->second.as<uint64_t>()));
    //         if(res==-1) {
    //             std::cout << "Duplicate node id " << NodeID(i->first.as<uint64_t>()) << std::endl;
    //             ret=-1;
    //         }
    //     }
    // }
    if(kvs["servers"]) {
        YAML::Node const &config = kvs["servers"];
        for (YAML::const_iterator i=config.begin(); i!=config.end(); i++) {
            int res = AddServer(ServerID(i->first.as<uint64_t>()), Addr(i->second.as<std::string>()));
            if(res==-1) {
                std::cout << "Duplicate server id " << ServerID(i->first.as<uint64_t>()) << std::endl;
                ret=-1;
            }
        }
    }

    if(!IsValid())
        ret=-1;

    return ret;
}

int Config::SaveConfigFile(std::string path) {
    int ret=0;

    YAML::Emitter out;
    out << YAML::BeginMap; // root

    out << YAML::Key << "nvmm";
    out << YAML::Value << YAML::BeginMap; // nvmm

    out << YAML::Key << "shelf_base";
    out << YAML::Value << shelf_base_;

    out << YAML::Key << "shelf_user";
    out << YAML::Value << shelf_user_;

    out << YAML::EndMap; // nvmm

    out << YAML::Key << "kvs";
    out << YAML::Value << YAML::BeginMap; // kvs

    out << YAML::Key << "partition_cnt";
    out << YAML::Value << partition_cnt_;

    out << YAML::Key << "server_cnt";
    out << YAML::Value << server_cnt_;

    out << YAML::Key << "node_cnt";
    out << YAML::Value << node_cnt_;

    out << YAML::Key << "starting_port";
    out << YAML::Value << starting_port_;

    out << YAML::Key << "kvs_type";
    out << YAML::Value << kvs_type_;

    out << YAML::Key << "kvs_size";
    out << YAML::Value << kvs_size_;

    out << YAML::Key << "cache_size";
    out << YAML::Value << cache_size_;

    out << YAML::Key << "server_thread";
    out << YAML::Value << server_thread_;

    out << YAML::Key << "replication_scheme";
    out << YAML::Value << rs2str(replication_scheme_);

    out << YAML::Key << "replication_factor";
    out << YAML::Value << replication_factor_;

    out << YAML::Key << "partitions";
    out << YAML::Value << partition_;

    // out << YAML::Key << "nodes";
    // out << YAML::Value << node_;

    out << YAML::Key << "servers";
    out << YAML::Value << server_;


    out << YAML::EndMap; // kvs

    out << YAML::EndMap; // root


    std::ofstream fout(path);
    fout << out.c_str() << std::endl;

    return ret;
}

int Config::UpdateRoot(PartitionID pid, std::string root) {
    KVS kvs;
    kvs["shelf_base"]=GetShelfBaseForPartition(pid);
    kvs["shelf_user"]=GetShelfUserForPartition(pid);
    kvs["kvs_type"]=kvs_type_;;
    kvs["kvs_size"]=std::to_string(kvs_size_);
    kvs["kvs_root"]=root;
    kvs["cache_size"]=std::to_string(cache_size_);
    kvs["server_thread"]=std::to_string(server_thread_);

    partition_[pid]=kvs;
    return 0;
}

int Config::AddPartition(PartitionID pid, KVS kvs) {
    auto found = partition_.find(pid);
    if(found==partition_.end()) {
        partition_[pid]=kvs;
        return 0;
    }
    else
        return -1;
}

int Config::AddServer(ServerID sid, Addr ip) {
    auto found = server_.find(sid);
    if(found==server_.end()) {
        server_[sid]=ip;
        return 0;
    }
    else
        return -1;
}

// int Config::AddNode(NodeID nid, Port port) {
//     auto found = node_.find(nid);
//     if(found==node_.end()) {
//         node_[nid]=port;
//         return 0;
//     }
//     else
//         return -1;
// }

void Config::RemovePartition(PartitionID pid) {
    partition_.erase(pid);
}

void Config::RemoveServer(ServerID sid) {
    server_.erase(sid);
}

// void Config::RemoveNode(NodeID nid) {
//     node_.erase(nid);
// }

void Config::Print() {

    std::cout << "NVMM Config " << std::endl;
    std::cout << "- shelf_base: " << shelf_base_ << std::endl;
    std::cout << "- shelf_user: " << shelf_user_ << std::endl;
    std::cout << std::endl;

    std::cout << "KVS Config " << std::endl;
    std::cout << "- partition_cnt: " << partition_cnt_ << std::endl;
    std::cout << "- server_cnt: " << server_cnt_ << std::endl;
    std::cout << "- node_cnt: " << node_cnt_ << std::endl;
    std::cout << "- starting_port: " << starting_port_ << std::endl;
    std::cout << "- replication_scheme: " << (int)replication_scheme_ << std::endl;
    std::cout << "- replication_factor: " << replication_factor_ << std::endl;
    std::cout << "- kvs_type: " << kvs_type_ << std::endl;
    std::cout << "- kvs_size: " << kvs_size_ << std::endl;
    std::cout << "- cache_size: " << cache_size_ << std::endl;
    std::cout << "- server_thread: " << server_thread_ << std::endl;
    std::cout << "- partitions: " << std::endl;
    for(auto i:partition_) {
        std::cout << "  " << i.first << ":" << std::endl;
        for(auto p:i.second) {
            std::cout << "    " << p.first << ": " << p.second << std::endl;
        }
    }
    // std::cout << "- nodes: " << std::endl;
    // for(auto i:node_) {
    //     std::cout << "  " << i.first << ": " << i.second << std::endl;
    // }
    std::cout << "- servers: " << std::endl;
    for(auto i:server_) {
        std::cout << "  " << i.first << ": " << i.second << std::endl;
    }
}

bool Config::IsValid() {
    return partition_cnt_>0 && node_cnt_>0 && server_cnt_>0 &&
        partition_.size() == partition_cnt_ &&
        //node_.size() == node_cnt_ &&
        server_.size() == server_cnt_ &&
        server_cnt_ >= replication_factor_ && replication_scheme_!=ReplicationScheme::INVALID;
}

Config::ReplicationScheme Config::str2rs(std::string str) {
    if(str == "NO_REPLICATION" || str == "no_replication")
        return ReplicationScheme::NO_REPLICATION;
    else if(str == "MASTER_SLAVE" || str == "master_slave")
        return ReplicationScheme::MASTER_SLAVE;
    else if(str == "DYNAMO" || str == "dynamo")
        return ReplicationScheme::DYNAMO;
    else if(str == "MODC" || str == "modc")
        return ReplicationScheme::MODC;
    else
        return ReplicationScheme::INVALID;
}

std::string Config::rs2str(Config::ReplicationScheme rs) {
    switch(rs) {
    case ReplicationScheme::NO_REPLICATION:
        return "NO_REPLICATION";
    case ReplicationScheme::MASTER_SLAVE:
        return "MASTER_SLAVE";
    case ReplicationScheme::DYNAMO:
        return "DYNAMO";
    case ReplicationScheme::MODC:
        return "MODC";
    case ReplicationScheme::INVALID:
    default:
        return "INVALID";
    }
}

} // namespace radixtree
