#pragma once
#include <string>

#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT
#endif

EXPORT int character_count(const std::string& input);

EXPORT std::string phylogenetic_analysis(
    const std::string& aln_file, 
    const std::string& partition_file, 
    const std::string& tree_file, 
    const std::string& out_prefix, 
    int num_threads, 
    int seed
);