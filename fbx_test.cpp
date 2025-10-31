#include "fbx.h"

int main() {
    fbx_manager manager("test_model", "./");

    // create a simple triangle mesh
    std::vector<double> verts = {0.0, 0.0, 0.0,
                                 1.0, 0.0, 0.0,
                                 0.0, 1.0, 0.0};
    std::vector<uint32_t> inds = {0,1,2};

    manager.addMesh("triangle", verts, inds);
    manager.addEmpty("root_empty");
    manager.addMaterial("simple_mat");

    // destructor will write file
    return 0;
}