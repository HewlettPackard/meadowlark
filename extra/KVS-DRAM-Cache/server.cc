//#include "memcached.h"
#include <cluster/config.h>
#include <cluster/cluster.h>
#include "cache_api.h"
#include <string>
#include <vector>
#include <iostream>

#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/uio.h>
#include <ctype.h>
#include <stdarg.h>

/* some POSIX systems need the following definition
 * to get mlockall flags out of sys/mman.h.  */
#ifndef _P1003_1B_VISIBLE
#define _P1003_1B_VISIBLE
#endif
/* need this to get IOV_MAX on some platforms. */
#ifndef __need_IOV_MAX
#define __need_IOV_MAX
#endif
#include <pwd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <limits.h>
#include <sysexits.h>
#include <stddef.h>


struct event_base *main_base;

int main (int argc, char **argv) {
    radixtree::Config config;
    std::string config_path;
    radixtree::Cluster cluster;
    radixtree::NodeID nid=0;
    size_t idx=0;
    bool use_cluster=false;

    int c;
    bool lock_memory = false;
    bool do_daemonize = false;
    bool preallocate = false;
    int maxcore = 0;
    char *username = NULL;
    char *pid_file = NULL;
    struct passwd *pw;
    struct rlimit rlim;
    char *buf;
    char unit = '\0';
    int size_max = 0;
    int retval = EXIT_SUCCESS;
    /* listening sockets */
    static int *l_socket = NULL;

    /* udp socket */
    static int *u_socket = NULL;
    bool protocol_specified = false;
    bool tcp_specified = false;
    bool udp_specified = false;
    bool start_lru_maintainer = false;
    bool start_lru_crawler = false;
    enum hashfunc_type hash_type = JENKINS_HASH;
    uint32_t tocrawl;
    uint32_t slab_sizes[MAX_NUMBER_OF_SLAB_CLASSES];
    bool use_slab_sizes = false;
    char *slab_sizes_unparsed = NULL;
    bool slab_chunk_size_changed = false;

    char *subopts, *subopts_orig;
    char *subopts_value;
    enum {
        MAXCONNS_FAST = 0,
        HASHPOWER_INIT,
        SLAB_REASSIGN,
        SLAB_AUTOMOVE,
        SLAB_AUTOMOVE_RATIO,
        SLAB_AUTOMOVE_WINDOW,
        TAIL_REPAIR_TIME,
        HASH_ALGORITHM,
        LRU_CRAWLER,
        LRU_CRAWLER_SLEEP,
        LRU_CRAWLER_TOCRAWL,
        LRU_MAINTAINER,
        HOT_LRU_PCT,
        WARM_LRU_PCT,
        HOT_MAX_FACTOR,
        WARM_MAX_FACTOR,
        TEMPORARY_TTL,
        IDLE_TIMEOUT,
        WATCHER_LOGBUF_SIZE,
        WORKER_LOGBUF_SIZE,
        SLAB_SIZES,
        SLAB_CHUNK_MAX,
        TRACK_SIZES,
        NO_INLINE_ASCII_RESP,
        MODERN
    };
    char *const subopts_tokens[] = {
        [MAXCONNS_FAST] = "maxconns_fast",
        [HASHPOWER_INIT] = "hashpower",
        [SLAB_REASSIGN] = "slab_reassign",
        [SLAB_AUTOMOVE] = "slab_automove",
        [SLAB_AUTOMOVE_RATIO] = "slab_automove_ratio",
        [SLAB_AUTOMOVE_WINDOW] = "slab_automove_window",
        [TAIL_REPAIR_TIME] = "tail_repair_time",
        [HASH_ALGORITHM] = "hash_algorithm",
        [LRU_CRAWLER] = "lru_crawler",
        [LRU_CRAWLER_SLEEP] = "lru_crawler_sleep",
        [LRU_CRAWLER_TOCRAWL] = "lru_crawler_tocrawl",
        [LRU_MAINTAINER] = "lru_maintainer",
        [HOT_LRU_PCT] = "hot_lru_pct",
        [WARM_LRU_PCT] = "warm_lru_pct",
        [HOT_MAX_FACTOR] = "hot_max_factor",
        [WARM_MAX_FACTOR] = "warm_max_factor",
        [TEMPORARY_TTL] = "temporary_ttl",
        [IDLE_TIMEOUT] = "idle_timeout",
        [WATCHER_LOGBUF_SIZE] = "watcher_logbuf_size",
        [WORKER_LOGBUF_SIZE] = "worker_logbuf_size",
        [SLAB_SIZES] = "slab_sizes",
        [SLAB_CHUNK_MAX] = "slab_chunk_max",
        [TRACK_SIZES] = "track_sizes",
        [NO_INLINE_ASCII_RESP] = "no_inline_ascii_resp",
        [MODERN] = "modern",
        NULL
    };

    if (!sanitycheck()) {
        return EX_OSERR;
    }

    /* handle SIGINT and SIGTERM */
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    /* init settings */
    settings_init();
	
    /* Run regardless of initializing it later */
    init_lru_crawler();
    init_lru_maintainer();

    /* set stderr non-buffering (for running under, say, daemontools) */
    setbuf(stderr, NULL);

    /* process arguments */
    while (-1 != (c = getopt(argc, argv,
          "a:"  /* access mask for unix socket */
          "A"  /* enable admin shutdown command */
          "p:"  /* TCP port number to listen on */
          "s:"  /* unix socket path to listen on */
          "U:"  /* UDP port number to listen on */
          "m:"  /* max memory to use for items in megabytes */
          "M"   /* return error on memory exhausted */
          "c:"  /* max simultaneous connections */
          "k"   /* lock down all paged memory */
          "hiV" /* help, licence info, version */
          "r"   /* maximize core file limit */
          "v"   /* verbose */
          "d"   /* daemon mode */
          "l:"  /* interface to listen on */
          "u:"  /* user identity to run as */
          "P:"  /* save PID in file */
          "f:"  /* factor? */
          "n:"  /* minimum space allocated for key+value+flags */
          "t:"  /* threads */
          "D:"  /* prefix delimiter? */
          "L"   /* Large memory pages */
          "R:"  /* max requests per event */
          "C"   /* Disable use of CAS */
          "b:"  /* backlog queue limit */
          "B:"  /* Binding protocol */
          "I:"  /* Max item size */
          "S"   /* Sasl ON */
          "F"   /* Disable flush_all */
          "X"   /* Disable dump commands */
          "o:"  /* Extended generic options */
          "K:"  /* KVS cluster config location */
          "N:"  /* KVS node ID*/
          "Z:"  /* KVS partition idx within this node*/
        ))) {
        switch (c) {
        case 'A':
            /* enables "shutdown" command */
            settings.shutdown_command = true;
            break;

        case 'a':
            /* access for unix domain socket, as octal mask (like chmod)*/
            settings.access= strtol(optarg,NULL,8);
            break;

        case 'U':
            settings.udpport = atoi(optarg);
            udp_specified = true;
            break;
        case 'p':
            settings.port = atoi(optarg);
            tcp_specified = true;
            break;
        case 's':
            settings.socketpath = optarg;
            break;
        case 'm':
            settings.maxbytes = ((size_t)atoi(optarg)) * 1024 * 1024;
            break;
        case 'M':
            settings.evict_to_free = 0;
            break;
        case 'c':
            settings.maxconns = atoi(optarg);
            if (settings.maxconns <= 0) {
                fprintf(stderr, "Maximum connections must be greater than 0\n");
                return 1;
            }
            break;
        case 'h':
            usage();
            exit(EXIT_SUCCESS);
        case 'i':
            usage_license();
            exit(EXIT_SUCCESS);
        case 'V':
            printf(PACKAGE " " VERSION "\n");
            exit(EXIT_SUCCESS);
        case 'k':
            lock_memory = true;
            break;
        case 'v':
            settings.verbose++;
            break;
        case 'l':
            if (settings.inter != NULL) {
                if (strstr(settings.inter, optarg) != NULL) {
                    break;
                }
                size_t len = strlen(settings.inter) + strlen(optarg) + 2;
                char *p = malloc(len);
                if (p == NULL) {
                    fprintf(stderr, "Failed to allocate memory\n");
                    return 1;
                }
                snprintf(p, len, "%s,%s", settings.inter, optarg);
                free(settings.inter);
                settings.inter = p;
            } else {
                settings.inter= strdup(optarg);
            }
            break;
        case 'd':
            do_daemonize = true;
            break;
        case 'r':
            maxcore = 1;
            break;
        case 'R':
            settings.reqs_per_event = atoi(optarg);
            if (settings.reqs_per_event == 0) {
                fprintf(stderr, "Number of requests per event must be greater than 0\n");
                return 1;
            }
            break;
        case 'u':
            username = optarg;
            break;
        case 'P':
            pid_file = optarg;
            break;
        case 'f':
            settings.factor = atof(optarg);
            if (settings.factor <= 1.0) {
                fprintf(stderr, "Factor must be greater than 1\n");
                return 1;
            }
            break;
        case 'n':
            settings.chunk_size = atoi(optarg);
            if (settings.chunk_size == 0) {
                fprintf(stderr, "Chunk size must be greater than 0\n");
                return 1;
            }
            break;
        case 't':
            settings.num_threads = atoi(optarg);
            if (settings.num_threads <= 0) {
                fprintf(stderr, "Number of threads must be greater than 0\n");
                return 1;
            }
            /* There're other problems when you get above 64 threads.
             * In the future we should portably detect # of cores for the
             * default.
             */
            if (settings.num_threads > 64) {
                fprintf(stderr, "WARNING: Setting a high number of worker"
                                "threads is not recommended.\n"
                                " Set this value to the number of cores in"
                                " your machine or less.\n");
            }
            break;
        case 'D':
            if (! optarg || ! optarg[0]) {
                fprintf(stderr, "No delimiter specified\n");
                return 1;
            }
            settings.prefix_delimiter = optarg[0];
            settings.detail_enabled = 1;
            break;
        case 'L' :
            if (enable_large_pages() == 0) {
                preallocate = true;
            } else {
                fprintf(stderr, "Cannot enable large pages on this system\n"
                    "(There is no Linux support as of this version)\n");
                return 1;
            }
            break;
        case 'C' :
            settings.use_cas = false;
            break;
        case 'b' :
            settings.backlog = atoi(optarg);
            break;
        case 'B':
            protocol_specified = true;
            if (strcmp(optarg, "auto") == 0) {
                settings.binding_protocol = negotiating_prot;
            } else if (strcmp(optarg, "binary") == 0) {
                settings.binding_protocol = binary_prot;
            } else if (strcmp(optarg, "ascii") == 0) {
                settings.binding_protocol = ascii_prot;
            } else {
                fprintf(stderr, "Invalid value for binding protocol: %s\n"
                        " -- should be one of auto, binary, or ascii\n", optarg);
                exit(EX_USAGE);
            }
            break;
        case 'I':
            buf = strdup(optarg);
            unit = buf[strlen(buf)-1];
            if (unit == 'k' || unit == 'm' ||
                unit == 'K' || unit == 'M') {
                buf[strlen(buf)-1] = '\0';
                size_max = atoi(buf);
                if (unit == 'k' || unit == 'K')
                    size_max *= 1024;
                if (unit == 'm' || unit == 'M')
                    size_max *= 1024 * 1024;
                settings.item_size_max = size_max;
            } else {
                settings.item_size_max = atoi(buf);
            }
            free(buf);
            if (settings.item_size_max < 1024) {
                fprintf(stderr, "Item max size cannot be less than 1024 bytes.\n");
                return 1;
            }
            if (settings.item_size_max > (settings.maxbytes / 4)) {
                fprintf(stderr, "Cannot set item size limit higher than 1/4 of memory max.\n");
                return 1;
            }
            if (settings.item_size_max > (1024 * 1024 * 1024)) {
                fprintf(stderr, "Cannot set item size limit higher than a gigabyte.\n");
                return 1;
            }
            if (settings.item_size_max > 1024 * 1024) {
                if (!slab_chunk_size_changed) {
                    // Ideal new default is 16k, but needs stitching.
                    settings.slab_chunk_size_max = 524288;
                }
            }
            break;
        case 'S': /* set Sasl authentication to true. Default is false */
#ifndef ENABLE_SASL
            fprintf(stderr, "This server is not built with SASL support.\n");
            exit(EX_USAGE);
#endif
            settings.sasl = true;
            break;
       case 'F' :
            settings.flush_enabled = false;
            break;
       case 'X' :
            settings.dump_enabled = false;
            break;
        case 'o': /* It's sub-opts time! */
            subopts_orig = subopts = strdup(optarg); /* getsubopt() changes the original args */

            while (*subopts != '\0') {

            switch (getsubopt(&subopts, subopts_tokens, &subopts_value)) {
            case MAXCONNS_FAST:
                settings.maxconns_fast = true;
                break;
            case HASHPOWER_INIT:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing numeric argument for hashpower\n");
                    return 1;
                }
                settings.hashpower_init = atoi(subopts_value);
                if (settings.hashpower_init < 12) {
                    fprintf(stderr, "Initial hashtable multiplier of %d is too low\n",
                        settings.hashpower_init);
                    return 1;
                } else if (settings.hashpower_init > 64) {
                    fprintf(stderr, "Initial hashtable multiplier of %d is too high\n"
                        "Choose a value based on \"STAT hash_power_level\" from a running instance\n",
                        settings.hashpower_init);
                    return 1;
                }
                break;
            case SLAB_REASSIGN:
                settings.slab_reassign = true;
                break;
            case SLAB_AUTOMOVE:
                if (subopts_value == NULL) {
                    settings.slab_automove = 1;
                    break;
                }
                settings.slab_automove = atoi(subopts_value);
                if (settings.slab_automove < 0 || settings.slab_automove > 2) {
                    fprintf(stderr, "slab_automove must be between 0 and 2\n");
                    return 1;
                }
                break;
            case SLAB_AUTOMOVE_RATIO:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing slab_automove_ratio argument\n");
                    return 1;
                }
                settings.slab_automove_ratio = atof(subopts_value);
                if (settings.slab_automove_ratio <= 0 || settings.slab_automove_ratio > 1) {
                    fprintf(stderr, "slab_automove_ratio must be > 0 and < 1\n");
                    return 1;
                }
                break;
            case SLAB_AUTOMOVE_WINDOW:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing slab_automove_window argument\n");
                    return 1;
                }
                settings.slab_automove_window = atoi(subopts_value);
                if (settings.slab_automove_window < 3) {
                    fprintf(stderr, "slab_automove_window must be > 2\n");
                    return 1;
                }
                break;
            case TAIL_REPAIR_TIME:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing numeric argument for tail_repair_time\n");
                    return 1;
                }
                settings.tail_repair_time = atoi(subopts_value);
                if (settings.tail_repair_time < 10) {
                    fprintf(stderr, "Cannot set tail_repair_time to less than 10 seconds\n");
                    return 1;
                }
                break;
            case HASH_ALGORITHM:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing hash_algorithm argument\n");
                    return 1;
                };
                if (strcmp(subopts_value, "jenkins") == 0) {
                    hash_type = JENKINS_HASH;
                } else if (strcmp(subopts_value, "murmur3") == 0) {
                    hash_type = MURMUR3_HASH;
                } else {
                    fprintf(stderr, "Unknown hash_algorithm option (jenkins, murmur3)\n");
                    return 1;
                }
                break;
            case LRU_CRAWLER:
                start_lru_crawler = true;
                break;
            case LRU_CRAWLER_SLEEP:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing lru_crawler_sleep value\n");
                    return 1;
                }
                settings.lru_crawler_sleep = atoi(subopts_value);
                if (settings.lru_crawler_sleep > 1000000 || settings.lru_crawler_sleep < 0) {
                    fprintf(stderr, "LRU crawler sleep must be between 0 and 1 second\n");
                    return 1;
                }
                break;
            case LRU_CRAWLER_TOCRAWL:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing lru_crawler_tocrawl value\n");
                    return 1;
                }
                if (!safe_strtoul(subopts_value, &tocrawl)) {
                    fprintf(stderr, "lru_crawler_tocrawl takes a numeric 32bit value\n");
                    return 1;
                }
                settings.lru_crawler_tocrawl = tocrawl;
                break;
            case LRU_MAINTAINER:
                start_lru_maintainer = true;
                settings.lru_segmented = true;
                break;
            case HOT_LRU_PCT:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing hot_lru_pct argument\n");
                    return 1;
                }
                settings.hot_lru_pct = atoi(subopts_value);
                if (settings.hot_lru_pct < 1 || settings.hot_lru_pct >= 80) {
                    fprintf(stderr, "hot_lru_pct must be > 1 and < 80\n");
                    return 1;
                }
                break;
            case WARM_LRU_PCT:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing warm_lru_pct argument\n");
                    return 1;
                }
                settings.warm_lru_pct = atoi(subopts_value);
                if (settings.warm_lru_pct < 1 || settings.warm_lru_pct >= 80) {
                    fprintf(stderr, "warm_lru_pct must be > 1 and < 80\n");
                    return 1;
                }
                break;
            case HOT_MAX_FACTOR:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing hot_max_factor argument\n");
                    return 1;
                }
                settings.hot_max_factor = atof(subopts_value);
                if (settings.hot_max_factor <= 0) {
                    fprintf(stderr, "hot_max_factor must be > 0\n");
                    return 1;
                }
                break;
            case WARM_MAX_FACTOR:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing warm_max_factor argument\n");
                    return 1;
                }
                settings.warm_max_factor = atof(subopts_value);
                if (settings.warm_max_factor <= 0) {
                    fprintf(stderr, "warm_max_factor must be > 0\n");
                    return 1;
                }
                break;
            case TEMPORARY_TTL:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing temporary_ttl argument\n");
                    return 1;
                }
                settings.temp_lru = true;
                settings.temporary_ttl = atoi(subopts_value);
                break;
            case IDLE_TIMEOUT:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing numeric argument for idle_timeout\n");
                    return 1;
                }
                settings.idle_timeout = atoi(subopts_value);
                break;
            case WATCHER_LOGBUF_SIZE:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing watcher_logbuf_size argument\n");
                    return 1;
                }
                if (!safe_strtoul(subopts_value, &settings.logger_watcher_buf_size)) {
                    fprintf(stderr, "could not parse argument to watcher_logbuf_size\n");
                    return 1;
                }
                settings.logger_watcher_buf_size *= 1024; /* kilobytes */
                break;
            case WORKER_LOGBUF_SIZE:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing worker_logbuf_size argument\n");
                    return 1;
                }
                if (!safe_strtoul(subopts_value, &settings.logger_buf_size)) {
                    fprintf(stderr, "could not parse argument to worker_logbuf_size\n");
                    return 1;
                }
                settings.logger_buf_size *= 1024; /* kilobytes */
            case SLAB_SIZES:
                slab_sizes_unparsed = subopts_value;
                break;
            case SLAB_CHUNK_MAX:
                if (subopts_value == NULL) {
                    fprintf(stderr, "Missing slab_chunk_max argument\n");
                }
                if (!safe_strtol(subopts_value, &settings.slab_chunk_size_max)) {
                    fprintf(stderr, "could not parse argument to slab_chunk_max\n");
                }
                slab_chunk_size_changed = true;
                break;
            case TRACK_SIZES:
                item_stats_sizes_init();
                break;
            case NO_INLINE_ASCII_RESP:
                settings.inline_ascii_response = false;
                break;
            case MODERN:
                /* Modernized defaults. Need to add equivalent no_* flags
                 * before making truly default. */
                // chunk default should come after stitching is fixed.
                //settings.slab_chunk_size_max = 16384;

                // With slab_reassign, pages are always 1MB, so anything larger
                // than .5m ends up using 1m anyway. With this we at least
                // avoid having several slab classes that use 1m.
                if (!slab_chunk_size_changed) {
                    settings.slab_chunk_size_max = 524288;
                }
                settings.slab_reassign = true;
                settings.slab_automove = 1;
                settings.maxconns_fast = true;
                settings.inline_ascii_response = false;
                settings.lru_segmented = true;
                hash_type = MURMUR3_HASH;
                start_lru_crawler = true;
                start_lru_maintainer = true;
                break;
            default:
                printf("Illegal suboption \"%s\"\n", subopts_value);
                return 1;
            }

            }
            free(subopts_orig);
            break;
        case 'K':
            config_path = std::string(optarg);
            config.LoadConfigFile(config_path);
            cluster.Init(config);
            use_cluster=true;
            break;
        case 'N':
            nid=radixtree::NodeID(atoi(optarg));
            break;
        case 'Z':
            idx=atoi(optarg);
            break;
        default:
            fprintf(stderr, "Illegal argument \"%c\"\n", c);
            return 1;
        }
    }

    if(use_cluster) {
        // TODO: udp?
        tcp_specified = true;

        // based on partition idx, get port number
        settings.hashpower_init = 27;
	settings.factor = 1.6;
        settings.port = cluster.FindPorts(nid)[idx];
        //std::cout << "Port: " << settings.port << std::endl;

        // based on partition idx, get KVS info
        radixtree::KVS kvs = cluster.FindPartitions(nid)[idx];
        for(auto i:kvs) {
            std::cout << i.first << ": " << i.second << std::endl;
        }

        // set cache size
        settings.maxbytes = std::stoull(kvs["cache_size"]);
        //std::cout << "Cache Size (MB): " << settings.maxbytes/1024/1024  << std::endl;

        // set threads
        settings.num_threads = std::stoull(kvs["server_thread"]);
        if (settings.num_threads <= 0) {
            fprintf(stderr, "Number of threads must be greater than 0\n");
            return 1;
        }
        /* There're other problems when you get above 64 threads.
         * In the future we should portably detect # of cores for the
         * default.
         */
        if (settings.num_threads > 64) {
            fprintf(stderr, "WARNING: Setting a high number of worker"
                    "threads is not recommended.\n"
                    " Set this value to the number of cores in"
                    " your machine or less.\n");
        }

        KVS_Init(kvs);
    }

    if (settings.slab_chunk_size_max > settings.item_size_max) {
        fprintf(stderr, "slab_chunk_max (bytes: %d) cannot be larger than -I (item_size_max %d)\n",
                settings.slab_chunk_size_max, settings.item_size_max);
        exit(EX_USAGE);
    }

    if (settings.item_size_max % settings.slab_chunk_size_max != 0) {
        fprintf(stderr, "-I (item_size_max: %d) must be evenly divisible by slab_chunk_max (bytes: %d)\n",
                settings.item_size_max, settings.slab_chunk_size_max);
        exit(EX_USAGE);
    }

    if (settings.slab_page_size % settings.slab_chunk_size_max != 0) {
        fprintf(stderr, "slab_chunk_max (bytes: %d) must divide evenly into %d (slab_page_size)\n",
                settings.slab_chunk_size_max, settings.slab_page_size);
        exit(EX_USAGE);
    }

    // Reserve this for the new default. If factor size hasn't changed, use
    // new default.
    /*if (settings.slab_chunk_size_max == 16384 && settings.factor == 1.25) {
        settings.factor = 1.08;
    }*/

    if (slab_sizes_unparsed != NULL) {
        if (_parse_slab_sizes(slab_sizes_unparsed, slab_sizes)) {
            use_slab_sizes = true;
        } else {
            exit(EX_USAGE);
        }
    }

    if (settings.hot_lru_pct + settings.warm_lru_pct > 80) {
        fprintf(stderr, "hot_lru_pct + warm_lru_pct cannot be more than 80%% combined\n");
        exit(EX_USAGE);
    }

    if (settings.temp_lru && !start_lru_maintainer) {
        fprintf(stderr, "temporary_ttl requires lru_maintainer to be enabled\n");
        exit(EX_USAGE);
    }

    if (hash_init(hash_type) != 0) {
        fprintf(stderr, "Failed to initialize hash_algorithm!\n");
        exit(EX_USAGE);
    }

    /*
     * Use one workerthread to serve each UDP port if the user specified
     * multiple ports
     */
    if (settings.inter != NULL && strchr(settings.inter, ',')) {
        settings.num_threads_per_udp = 1;
    } else {
        settings.num_threads_per_udp = settings.num_threads;
    }

    if (settings.sasl) {
        if (!protocol_specified) {
            settings.binding_protocol = binary_prot;
        } else {
            if (settings.binding_protocol != binary_prot) {
                fprintf(stderr, "ERROR: You cannot allow the ASCII protocol while using SASL.\n");
                exit(EX_USAGE);
            }
        }
    }

    if (tcp_specified && !udp_specified) {
        settings.udpport = settings.port;
    } else if (udp_specified && !tcp_specified) {
        settings.port = settings.udpport;
    }

    if (maxcore != 0) {
        struct rlimit rlim_new;
        /*
         * First try raising to infinity; if that fails, try bringing
         * the soft limit to the hard.
         */
        if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
            rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
            if (setrlimit(RLIMIT_CORE, &rlim_new)!= 0) {
                /* failed. try raising just to the old max */
                rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
                (void)setrlimit(RLIMIT_CORE, &rlim_new);
            }
        }
        /*
         * getrlimit again to see what we ended up with. Only fail if
         * the soft limit ends up 0, because then no core files will be
         * created at all.
         */

        if ((getrlimit(RLIMIT_CORE, &rlim) != 0) || rlim.rlim_cur == 0) {
            fprintf(stderr, "failed to ensure corefile creation\n");
            exit(EX_OSERR);
        }
    }

    /*
     * If needed, increase rlimits to allow as many connections
     * as needed.
     */

    if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) {
        fprintf(stderr, "failed to getrlimit number of files\n");
        exit(EX_OSERR);
    } else {
        rlim.rlim_cur = settings.maxconns;
        rlim.rlim_max = settings.maxconns;
        if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) {
            fprintf(stderr, "failed to set rlimit for open files. Try starting as root or requesting smaller maxconns value.\n");
            exit(EX_OSERR);
        }
    }

    /* lose root privileges if we have them */
    if (getuid() == 0 || geteuid() == 0) {
        if (username == 0 || *username == '\0') {
            fprintf(stderr, "can't run as root without the -u switch\n");
            exit(EX_USAGE);
        }
        if ((pw = getpwnam(username)) == 0) {
            fprintf(stderr, "can't find the user %s to switch to\n", username);
            exit(EX_NOUSER);
        }
        if (setgid(pw->pw_gid) < 0 || setuid(pw->pw_uid) < 0) {
            fprintf(stderr, "failed to assume identity of user %s\n", username);
            exit(EX_OSERR);
        }
    }

    /* Initialize Sasl if -S was specified */
    if (settings.sasl) {
        init_sasl();
    }

    /* daemonize if requested */
    /* if we want to ensure our ability to dump core, don't chdir to / */
    if (do_daemonize) {
        if (sigignore(SIGHUP) == -1) {
            perror("Failed to ignore SIGHUP");
        }
        if (daemonize(maxcore, settings.verbose) == -1) {
            fprintf(stderr, "failed to daemon() in order to daemonize\n");
            exit(EXIT_FAILURE);
        }
    }

    /* lock paged memory if needed */
    if (lock_memory) {
#ifdef HAVE_MLOCKALL
        int res = mlockall(MCL_CURRENT | MCL_FUTURE);
        if (res != 0) {
            fprintf(stderr, "warning: -k invalid, mlockall() failed: %s\n",
                    strerror(errno));
        }
#else
        fprintf(stderr, "warning: -k invalid, mlockall() not supported on this platform.  proceeding without.\n");
#endif
    }

    /* initialize main thread libevent instance */
    main_base = event_init();

    /* initialize other stuff */
    logger_init();
    stats_init();
    assoc_init(settings.hashpower_init);		//Create hash table
    conn_init();
    slabs_init(settings.maxbytes, settings.factor, preallocate,
            use_slab_sizes ? slab_sizes : NULL);

    /*
     * ignore SIGPIPE signals; we can use errno == EPIPE if we
     * need that information
     */
    if (sigignore(SIGPIPE) == -1) {
        perror("failed to ignore SIGPIPE; sigaction");
        exit(EX_OSERR);
    }
    /* start up worker threads if MT mode */
    memcached_thread_init(settings.num_threads);

    if (start_assoc_maintenance_thread() == -1) {
        exit(EXIT_FAILURE);
    }

    if (start_lru_crawler && start_item_crawler_thread() != 0) {
        fprintf(stderr, "Failed to enable LRU crawler thread\n");
        exit(EXIT_FAILURE);
    }

    if (start_lru_maintainer && start_lru_maintainer_thread() != 0) {
        fprintf(stderr, "Failed to enable LRU maintainer thread\n");
        return 1;
    }

    if (settings.slab_reassign &&
        start_slab_maintenance_thread() == -1) {
        exit(EXIT_FAILURE);
    }

    if (settings.idle_timeout && start_conn_timeout_thread() == -1) {
        exit(EXIT_FAILURE);
    }

    /* initialise clock event */
    clock_handler(0, 0, 0);

    /* create unix mode sockets after dropping privileges */
    if (settings.socketpath != NULL) {
        errno = 0;
        if (server_socket_unix(settings.socketpath,settings.access)) {
            vperror("failed to listen on UNIX socket: %s", settings.socketpath);
            exit(EX_OSERR);
        }
    }

    /* create the listening socket, bind it, and init */
    if (settings.socketpath == NULL) {
        const char *portnumber_filename = getenv("MEMCACHED_PORT_FILENAME");
        char *temp_portnumber_filename = NULL;
        size_t len;
        FILE *portnumber_file = NULL;

        if (portnumber_filename != NULL) {
            len = strlen(portnumber_filename)+4+1;
            temp_portnumber_filename = malloc(len);
            snprintf(temp_portnumber_filename,
                     len,
                     "%s.lck", portnumber_filename);

            portnumber_file = fopen(temp_portnumber_filename, "a");
            if (portnumber_file == NULL) {
                fprintf(stderr, "Failed to open \"%s\": %s\n",
                        temp_portnumber_filename, strerror(errno));
            }
        }

        errno = 0;
        if (settings.port && server_sockets(settings.port, tcp_transport,
                                           portnumber_file)) {
            vperror("failed to listen on TCP port %d", settings.port);
            exit(EX_OSERR);
        }

        /*
         * initialization order: first create the listening sockets
         * (may need root on low ports), then drop root if needed,
         * then daemonize if needed, then init libevent (in some cases
         * descriptors created by libevent wouldn't survive forking).
         */

        /* create the UDP listening socket and bind it */
        errno = 0;
        if (settings.udpport && server_sockets(settings.udpport, udp_transport,
                                              portnumber_file)) {
            vperror("failed to listen on UDP port %d", settings.udpport);
            exit(EX_OSERR);
        }

        if (portnumber_file) {
            fclose(portnumber_file);
            rename(temp_portnumber_filename, portnumber_filename);
            free(temp_portnumber_filename);
        }
    }

    /* Give the sockets a moment to open. I know this is dumb, but the error
     * is only an advisory.
     */
    usleep(1000);
    if (stats_state.curr_conns + stats_state.reserved_fds >= settings.maxconns - 1) {
        fprintf(stderr, "Maxconns setting is too low, use -c to increase.\n");
        exit(EXIT_FAILURE);
    }

    if (pid_file != NULL) {
        save_pid(pid_file);
    }

    /* Drop privileges no longer needed */
    drop_privileges();

    /* Initialize the uriencode lookup table. */
    uriencode_init();

    /* enter the event loop */
    if (event_base_loop(main_base, 0) != 0) {
        retval = EXIT_FAILURE;
    }

	stop_assoc_maintenance_thread();

    /* remove the PID file if we're a daemon */
    if (do_daemonize)
        remove_pidfile(pid_file);
    /* Clean up strdup() call for bind() address */
    if (settings.inter)
      free(settings.inter);
    if (l_socket)
      free(l_socket);
    if (u_socket)
      free(u_socket);

    return retval;
}
