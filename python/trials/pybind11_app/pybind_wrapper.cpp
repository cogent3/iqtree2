
#include <pybind11/pybind11.h>
#include "../Cpp_lib/mock_iqtree.h"

PYBIND11_MODULE(pybind_wrapper, m) {
        m.doc() = "mocked iqtree example";

        m.def("phylogenetic_analysis", &phylogenetic_analysis, "A function to perform phylogenetic analysis");
}
