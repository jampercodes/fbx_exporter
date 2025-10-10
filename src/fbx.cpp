#include "fbx.h"

// methods
fbx_node CreateNode(std::string Name) {
    fbx_node node;

}

//class methods
fbx_manager::fbx_manager(std::string filename, std::string filelocation) {
    
    // writing header to file
    M_file = fopen( "./test.fbx", "w");


    fprintf(M_file, "Kaydara FBX Binary  ");

    unsigned char magickBytes[] = {0x00, 0x1A, 0x00};
    fwrite(magickBytes, sizeof(unsigned char), sizeof(magickBytes), M_file);

    fprintf(M_file, "%u", FBX_VERSION);

    // create root node
    fbx_node root_node;

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

fbx_node::fbx_node() {
    name_len = 0;
    node_name = nullptr;

    num_properties = 0;
    property_list_len = 0;

    property_count = 0;
    chiled_node_count = 0;
    
}