# distutils: language = c++

from libcpp.string cimport string

cdef extern from "../cpp_lib/mock_iqtree.h":
    string phylogenetic_analysis(string, string, string, string, int, int)

def phylogenetic_analysis_py(str aln_file, str partition_file, str tree_file, str out_prefix, int num_threads, int seed):
    cdef string result = phylogenetic_analysis(aln_file.encode('utf-8'), partition_file.encode('utf-8'), tree_file.encode('utf-8'), out_prefix.encode('utf-8'), num_threads, seed)
    return result.decode('utf-8')