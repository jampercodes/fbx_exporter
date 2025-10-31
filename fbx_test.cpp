#include "fbx.h"

int main() {
    fbx_manager manager("test_model", "./");

    // create a simple triangle mesh
    std::vector<double> verts = {0.0, 0.0, 0.0,
                                 1.0, 0.0, 0.0,
                                 0.0, 1.0, 0.0};
    std::vector<uint32_t> inds = {0,1,2};

    // create material and get id
    int64_t matId = manager.addMaterial("simple_mat");

    // simple normals per-vertex (nx,ny,nz) for 3 verts
    std::vector<double> norms = {0.0,0.0,1.0,
                                 0.0,0.0,1.0,
                                 0.0,0.0,1.0};
    // simple UVs (u,v) per-vertex
    std::vector<double> uvs = {0.0,0.0,
                               1.0,0.0,
                               0.0,1.0};

    FbxTransform xf;
    xf.t[0]=0.0; xf.t[1]=0.0; xf.t[2]=0.0;
    xf.r[0]=0.0; xf.r[1]=0.0; xf.r[2]=0.0;
    xf.s[0]=1.0; xf.s[1]=1.0; xf.s[2]=1.0;

    manager.addMesh("triangle", verts, inds, norms, uvs, xf, matId);
    manager.addEmpty("root_empty");

    // destructor will write file
    return 0;
}