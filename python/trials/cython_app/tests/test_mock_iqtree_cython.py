import pytest
from cython_wrapper import phylogenetic_analysis_py

def test_cython():
    assert "42" == phylogenetic_analysis_py("path/to/aln_file", "path/to/partition_file", "path/to/tree_file", "output_prefix", 4, 12345)
