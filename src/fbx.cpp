#include "fbx.h"

//class methods
fbx_manager::fbx_manager(std::string filename, std::string filelocation) {
    
    // writing header to file
    M_file = fopen( "./test.fbx", "w");

    //printf("Creating FBX file: %s%s.fbx\n", filelocation, filename);

    fprintf(M_file, "Kaydara FBX Binary  ");
    //fprintf(M_file, "\x00\x1A\x00");
    fprintf(M_file, "%u", FBX_VERSION);

    fbx_node root_node("RootNode");

}

fbx_manager::~fbx_manager() {
    if (M_file) {
        fclose(M_file);
        M_file = nullptr;
    }
}

void fbx_manager::add_mesh() {
    

    // add mesh data to the file
}

fbx_node::fbx_node(std::string name) {
    node_name = name;
    
    chiled_node_count = 0;
    chiled_nodes = nullptr;
}
