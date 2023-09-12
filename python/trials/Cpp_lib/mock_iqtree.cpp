
#include "mock_iqtree.h"

int character_count(const std::string& input) {
    return input.size();
}

std::string phylogenetic_analysis(
    const std::string& alignment_file, 
    const std::string& partition_file, 
    const std::string& tree_file, 
    const std::string& out_prefix, 
    int num_threads, 
    int seed
) {
    return "42";
}