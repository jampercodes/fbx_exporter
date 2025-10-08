#include "../Include/fbx.h"

//class methods
fbx_manager::fbx_manager(char *filename, char *filelocation) {
    
    // writing header to file
    M_file = fopen(*filelocation + *filename + ".fbx", "w");

    std::printf("Creating FBX file: %s%s.fbx\n", filelocation, filename);

    fprintf(M_file, "Kaydara FBX Binary  ");
    fprintf(M_file, "\x00\x1A\x00");
    fprintf(M_file, "%u", FBX_VERSION);


    //init the root node
    root_node = fbx_node();
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

void fbx_node::fbx_node(char *name) {
    node_name = name;
    
    chiled_node_count = 0;
    chiled_nodes = nullptr;
}
