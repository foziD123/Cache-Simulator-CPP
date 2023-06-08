/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <utility>
#include <bits/stdc++.h>
#include <algorithm>

using std::vector;
using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;

/* Program is going to consist of two main classes, one represetnting the Cache and
 and the other representing the whole Memory which contains Cache levels according
 to the input */

typedef enum {
    INVALID = 0, EXIST = 1, DIRTY = 2
} BlockStatus;
typedef enum {
    NO_WRITE_ALLOCATE = 0, WRITE_ALLOCATE = 1
} WriteMissPolicy;

//---------------------L1 or L2 Cache class-----------------------------
class Cache {
public:

    unsigned block_size;
    unsigned cache_size;
    unsigned num_of_ways;
    unsigned num_of_blocks;
    unsigned num_of_sets;
    unsigned access_num;
    unsigned misses;
    unsigned cache_cyc;

    vector <vector<unsigned>> data_tags; //holds the tags that are currently saved in the Cache in a matrix : rows = sets, columns = ways.
    vector <vector<BlockStatus>> data_status; //holds the current status of each block
    vector <vector<int>> LRU_queue; //holds the #way which is least recently used in each set

    Cache(unsigned b_size, unsigned l_size, unsigned n_ways, unsigned l_cyc) {

        this->block_size = pow(2, b_size);
        this->cache_size = pow(2, l_size);
        this->num_of_ways = pow(2, n_ways);
        this->num_of_blocks = (this->cache_size) / (this->block_size);
        this->num_of_sets = (this->num_of_blocks) / (this->num_of_ways);
        this->access_num = 0;
        this->misses = 0;
        this->cache_cyc = l_cyc;

        this->data_tags.resize(num_of_sets, vector<unsigned>(num_of_ways, 0));
        this->data_status.resize(num_of_sets, vector<BlockStatus>(num_of_ways, INVALID));
        this->LRU_queue.resize(num_of_sets, vector<int>(0, 0));
    }

    ~Cache() = default;

    Cache(Cache &other) = default;

    void decodeAddress(unsigned long address, unsigned long int *addr_tag, unsigned *set_num);

    bool inCache(unsigned long int address, unsigned long int *tag, BlockStatus *status, int *way_num);

    bool addToCache(unsigned long int address, int *way_num);

    void removeFromCache(unsigned set_num, int way_num);

    void addToLRUqueue(unsigned long int address, int new_way_num);

    void removeFromLRUqueue(unsigned set_num, int deleted_way_num);

};

void Cache::decodeAddress(unsigned long address, unsigned long *addr_tag, unsigned int *set_num) {
    int offset_bits_num = log2(this->block_size);
    int set_bits_num = log2(this->num_of_sets);
    unsigned long int tag_and_set = address >> offset_bits_num; //the rest of the address without the offset.
    *set_num = tag_and_set % (this->num_of_sets); //get the number of the set the data tag should be in.
    *addr_tag = tag_and_set >> set_bits_num;

}

bool Cache::inCache(unsigned long address, unsigned long *tag, BlockStatus *status, int *way_num) {
    unsigned long addr_tag = 0;
    unsigned set_num = 0;
    this->decodeAddress(address, &addr_tag, &set_num);


    for (unsigned int i = 0; i < this->num_of_ways; ++i) {
        if (data_tags[set_num][i] == addr_tag &&
            data_status[set_num][i] != INVALID) {        //the tag exists in the cache with status EXIST or DIRTY
            *tag = addr_tag;
            *status = data_status[set_num][i];
            *way_num = i;
            return true;                                 //return true with the tag and the current status
        }
    }

    //the given tag does not exist in the cache
    tag = nullptr;
    status = nullptr;
    *way_num = -1;
    return false;
}


//return true if the there is a place to add the address , false if we need to evict.
//without LRU update.
bool Cache::addToCache(unsigned long int address, int *way_num) {
    unsigned long tag = 0;
    unsigned set_num = 0;
    this->decodeAddress(address, &tag, &set_num);
    for (unsigned int i = 0; i < this->num_of_ways; ++i) {
        if (data_status[set_num][i] == INVALID) {
            data_tags[set_num][i] = tag;
            data_status[set_num][i] = EXIST;
            *way_num = i;
            return true;                    //we find an appropriate place to the new addr.
        }
    }
    return false; //all the n ways in the wanted set num is full.
}

//delete the wanted addr by marking its status as INVALID
void Cache::removeFromCache(unsigned set_num, int way_num) {
    data_tags[set_num][way_num] = 0;
    data_status[set_num][way_num] = INVALID;
}

void Cache::addToLRUqueue(unsigned long int address, int new_way_num) {
    unsigned long tag = 0;
    unsigned set_num = 0;
    this->decodeAddress(address, &tag, &set_num);
    this->LRU_queue[set_num].push_back(new_way_num);

}

void Cache::removeFromLRUqueue(unsigned set_num, int deleted_way_num) {
    vector<int> new_queue;
    auto itr = this->LRU_queue[set_num].begin();
    for (; itr != LRU_queue[set_num].end(); ++itr) {
        if (*itr != deleted_way_num) {
            new_queue.push_back(*itr);
        }
    }

    LRU_queue[set_num] = new_queue;

}

//---------------The All Memory Class-----------------------------------
class Memory {
public:

    unsigned mem_cyc;
    unsigned mem_access_count;

    Cache *L1;
    Cache *L2;
    WriteMissPolicy policy;

    Memory(unsigned MemCyc, unsigned BSize, unsigned L1Size, unsigned L2Size, unsigned L1Assoc, unsigned L2Assoc,
           unsigned L1Cyc, unsigned L2Cyc, unsigned WrAlloc) {
        this->L1 = new Cache(BSize, L1Size, L1Assoc, L1Cyc);
        this->L2 = new Cache(BSize, L2Size, L2Assoc, L2Cyc);
        this->policy = WrAlloc ? WRITE_ALLOCATE : NO_WRITE_ALLOCATE;
        this->mem_cyc = MemCyc;
        this->mem_access_count = 0;

    }

    ~Memory() {
        delete L1;
        delete L2;
    }

    Memory(Memory &mem) = default;

    void operate(unsigned long int address, char operation);
};


void Memory::operate(unsigned long int address, char operation) {
    unsigned long int tag_L1 = 0;
    unsigned long int tag_L2 = 0;
    unsigned set_num_L1 = 0;
    unsigned set_num_L2 = 0;
    BlockStatus status_L1;
    BlockStatus status_L2;
    int way_num_L1 = 0;
    int way_num_L2 = 0;
    this->L1->decodeAddress(address, &tag_L1, &set_num_L1);
    this->L2->decodeAddress(address, &tag_L2, &set_num_L2);
    int lru_way_L1 = 0; //to evict from L1.
    int lru_way_L2 = 0; //to evict from L2.


    //check in cache L1
    L1->access_num++;
    if (L1->inCache(address, &tag_L1, &status_L1, &way_num_L1)) {                //if found in L1
        L1->removeFromLRUqueue(set_num_L1, way_num_L1);
        L1->addToLRUqueue(address, way_num_L1);
        if (operation == 'w') {
            L1->data_status[set_num_L1][way_num_L1] = DIRTY;
        }
    } else {                                                                     //not found in Cache L1 --> check in Cache L2
        L1->misses++;
        L2->access_num++;
        if (L2->inCache(address, &tag_L2, &status_L2, &way_num_L2)) {            //if found in L2
            if ((operation == 'r') ||
                (operation == 'w' && this->policy == WRITE_ALLOCATE)) {          //need to add to L1 Cache
                bool res_add = L1->addToCache(address, &way_num_L1);
                if (res_add) { //there is an empty way
                    L1->addToLRUqueue(address, way_num_L1);
                } else {                                                          //need to evict from L1
                    lru_way_L1 = *(L1->LRU_queue[set_num_L1].begin());
                    BlockStatus deleted_status = L1->data_status[set_num_L1][lru_way_L1];
                    if (deleted_status == DIRTY) {                                 //need to update in L2
                        int offset_bits_num = log2(L1->block_size);
                        int set_bits_num = log2(L1->num_of_sets);
                        unsigned int temp = L1->data_tags[set_num_L1][lru_way_L1] << set_bits_num;
                        temp = temp | set_num_L1;
                        temp = temp << offset_bits_num;

                        unsigned long int deleted_tag = 0;
                        unsigned int deleted_set = 0;
                        int deleted_way = 0;
                        L2->decodeAddress(temp, &deleted_tag, &deleted_set);
                        if (L2->inCache(temp, &deleted_tag, &deleted_status, &deleted_way)) {
                            L2->data_status[deleted_set][deleted_way] = DIRTY;
                            L2->removeFromLRUqueue(deleted_set, deleted_way);
                            L2->addToLRUqueue(temp, deleted_way);
                        }

                    }

                    //removing the lru from L1
                    L1->removeFromCache(set_num_L1, lru_way_L1);
                    L1->removeFromLRUqueue(set_num_L1, lru_way_L1);

                    bool res_add_after_evict = L1->addToCache(address, &way_num_L1);
                    if (!res_add_after_evict) {
                        cout << "errrorrrrr" << endl;
                    }
                    L1->addToLRUqueue(address, lru_way_L1);
                }
                L2->removeFromLRUqueue(set_num_L2, way_num_L2);
                L2->addToLRUqueue(address, way_num_L2);
                if (operation == 'w') {
                    L1->data_status[set_num_L1][way_num_L1] = DIRTY;
                }
            } else {         //operation = 'w' && policy = NO_WRITE_ALLOCATE
                L2->data_status[set_num_L2][way_num_L2] = DIRTY;
                L2->removeFromLRUqueue(set_num_L2, way_num_L2);
                L2->addToLRUqueue(address, way_num_L2);
            }
        } else {   //not found in L1 and L2
            L2->misses++;
            this->mem_access_count++;
            if ((operation == 'r') ||
                (operation == 'w' && this->policy == WRITE_ALLOCATE)) {

                //add to l2:
                bool res_add_L2 = L2->addToCache(address, &way_num_L2);
                if (res_add_L2) {                                          //there is an empty way in L2
                    L2->addToLRUqueue(address, way_num_L2);
                } else {                                                   //need to evict from L2.
                    lru_way_L2 = *(L2->LRU_queue[set_num_L2].begin());
                    BlockStatus deleted_status = L2->data_status[set_num_L2][lru_way_L2];

                    //snooping to L1:
                    int offset_bits_num = log2(L2->block_size);
                    int set_bits_num = log2(L2->num_of_sets);
                    unsigned int temp = L2->data_tags[set_num_L2][lru_way_L2] << set_bits_num;
                    temp = temp | set_num_L2;
                    temp = temp << offset_bits_num;

                    unsigned long int deleted_tag = 0;
                    unsigned int deleted_set = 0;
                    int deleted_way = 0;
                    L1->decodeAddress(temp, &deleted_tag, &deleted_set);
                    if (L1->inCache(temp, &deleted_tag, &deleted_status, &deleted_way)) {
                        ////////// CHECK IF THERE IS NEED TO UPDATE IN L2 LRU
                        if (deleted_status != DIRTY) {
                            L2->removeFromLRUqueue(set_num_L2, lru_way_L2);
                            L2->addToLRUqueue(address, lru_way_L2);
                        }
                        L1->removeFromCache(deleted_set, deleted_way);
                        L1->removeFromLRUqueue(deleted_set, deleted_way);
                    }

                    //removing the lru from L2
                    L2->removeFromCache(set_num_L2, lru_way_L2);

                    bool res_add_after_evict = L2->addToCache(address, &way_num_L2);
                    if (!res_add_after_evict) {
                        cout << "errrorrrrr" << endl;                  //for DEBUG: check id lru_way_L1 = way_num_L1
                    }
                }

                //add to L1:
                //same as when found in L2 and not found in L1.
                bool res_add = L1->addToCache(address, &way_num_L1);
                if (res_add) {                                                      //there is an empty way
                    L1->addToLRUqueue(address, way_num_L1);
                } else {                                                            //need to evict from L1
                    lru_way_L1 = *(L1->LRU_queue[set_num_L1].begin());
                    BlockStatus deleted_status = L1->data_status[set_num_L1][lru_way_L1];
                    if (deleted_status == DIRTY) {                                  //need to update in L2
                        int offset_bits_num = log2(L1->block_size);
                        int set_bits_num = log2(L1->num_of_sets);
                        unsigned int temp = L1->data_tags[set_num_L1][lru_way_L1] << set_bits_num;
                        temp = temp | set_num_L1;
                        temp = temp << offset_bits_num;
                        unsigned long int deleted_tag = 0;
                        unsigned int deleted_set = 0;
                        int deleted_way = 0;
                        L2->decodeAddress(temp, &deleted_tag, &deleted_set);
                        if (L2->inCache(temp, &deleted_tag, &deleted_status, &deleted_way)) {
                            L2->data_status[deleted_set][deleted_way] = DIRTY;
                            L2->removeFromLRUqueue(deleted_set, deleted_way);
                            L2->addToLRUqueue(temp, deleted_way);
                        }
                    }

                    //removing the lru from L1
                    L1->removeFromCache(set_num_L1, lru_way_L1);
                    L1->removeFromLRUqueue(set_num_L1, lru_way_L1);

                    bool res_add_after_evict = L1->addToCache(address, &way_num_L1);
                    if (!res_add_after_evict) {
                        cout << "errrorrrrr" << endl;
                    }
                    L1->addToLRUqueue(address, lru_way_L1);

                }

                L2->removeFromLRUqueue(set_num_L2, way_num_L2);
                L2->addToLRUqueue(address, way_num_L2);
                if (operation == 'w') {
                    L1->data_status[set_num_L1][way_num_L1] = DIRTY;
                }

            }

        }
    }

}


int main(int argc, char **argv) {

    if (argc < 19) {
        cerr << "Not enough arguments" << endl;
        return 0;
    }

    // Get input arguments

    // File
    // Assuming it is the first argument
    char *fileString = argv[1];
    ifstream file(fileString); //input file stream
    string line;
    if (!file || !file.good()) {
        // File doesn't exist or some other error
        cerr << "File not found" << endl;
        return 0;
    }

    unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
            L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

    for (int i = 2; i < 19; i += 2) {
        string s(argv[i]);
        if (s == "--mem-cyc") {
            MemCyc = atoi(argv[i + 1]);
        } else if (s == "--bsize") {
            BSize = atoi(argv[i + 1]);
        } else if (s == "--l1-size") {
            L1Size = atoi(argv[i + 1]);
        } else if (s == "--l2-size") {
            L2Size = atoi(argv[i + 1]);
        } else if (s == "--l1-cyc") {
            L1Cyc = atoi(argv[i + 1]);
        } else if (s == "--l2-cyc") {
            L2Cyc = atoi(argv[i + 1]);
        } else if (s == "--l1-assoc") {
            L1Assoc = atoi(argv[i + 1]);
        } else if (s == "--l2-assoc") {
            L2Assoc = atoi(argv[i + 1]);
        } else if (s == "--wr-alloc") {
            WrAlloc = atoi(argv[i + 1]);
        } else {
            cerr << "Error in arguments" << endl;
            return 0;
        }
    }

    //-----------------added-----------------------------------------
    Memory *memory = new Memory(MemCyc, BSize, L1Size, L2Size, L1Assoc, L2Assoc, L1Cyc, L2Cyc, WrAlloc);

    //---------------------------------------------------------------

    while (getline(file, line)) {

        stringstream ss(line);
        string address;
        char operation = 0; // read (R) or write (W)
        if (!(ss >> operation >> address)) {
            // Operation appears in an Invalid format
            cout << "Command Format error" << endl;
            return 0;
        }

        // DEBUG - remove this line
        // cout << "operation: " << operation;

        string cutAddress = address.substr(2); // Removing the "0x" part of the address

        // DEBUG - remove this line
        // cout << ", address (hex)" << cutAddress << endl;

        unsigned long int num = 0;
        num = strtoul(cutAddress.c_str(), NULL, 16);

        // DEBUG - remove this line
        //cout << " (dec) " << num << endl;
        memory->operate(num, operation);

    }

    double L1MissRate;
    double L2MissRate;
    double avgAccTime;

    //L1 miss rate calculation:
    L1MissRate = memory->L1->access_num ? double(memory->L1->misses) / double(memory->L1->access_num) : 0;

    //L2 miss rate calculation:
    L2MissRate = memory->L2->access_num ? double(memory->L2->misses) / double(memory->L2->access_num) : 0;

    //Avg:
    double command_count = memory->L1->access_num;
    double cyc_1 = memory->L1->access_num * memory->L1->cache_cyc;
    double cyc_2 = memory->L2->access_num * memory->L2->cache_cyc;
    double cyc_mem = memory->mem_access_count * memory->mem_cyc;
    avgAccTime = (cyc_1 + cyc_2 + cyc_mem) / command_count;


    /*cout << "L1 access:" << memory->L1->access_num << endl;
    cout << "L2 access:" << memory->L2->access_num << endl;
    cout << "mem access:" << memory->mem_access_count << endl;
    cout << "L1 misses:" << memory->L1->misses << endl;
    cout << "L2 misses:" << memory->L2->misses << endl;*/

    printf("L1miss=%.03f ", L1MissRate);
    printf("L2miss=%.03f ", L2MissRate);
    printf("AccTimeAvg=%.03f\n", avgAccTime);

    delete memory;
    return 0;
}
