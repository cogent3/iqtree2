import pytest
from pybind_wrapper import phylogenetic_analysis

def test_pybind11():
    assert "42" == phylogenetic_analysis("path/to/aln_file", "path/to/partition_file", "path/to/tree_file", "output_prefix", 4, 12345)
