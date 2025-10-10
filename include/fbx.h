#pragma once

#include <stdio.h>
#include <string>
#include <inttypes.h>

struct fbx_property {
    char type[4];
    void* data;
};

class fbx_node {
public:
    fbx_node();
private:
    uint32_t end_offset;
    uint32_t num_properties;
    uint32_t property_list_len;

    uint8_t name_len;

    char* node_name;

    int property_count;
    fbx_property* properties;

    int chiled_node_count;
    fbx_node* chiled_nodes;
};

class fbx_manager {
public:
    fbx_manager(std::string filename, std::string filelocation);
    ~fbx_manager();
    
    void add_mesh();

private:
    FILE* M_file;
};


const unsigned int FBX_VERSION = 6000;


extern fbx_node root_node;

fbx_node CreateNode(std::string Name);
void AddChildNode(fbx_node ParentNode, fbx_node Node);