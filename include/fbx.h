#pragma once

#include <stdio.h>
#include <string>

struct fbx_property {
    char type[4];
    void* data;
};

class fbx_node {
public:
    fbx_node(std::string name);
    
private:
    std::string node_name;

    unsigned int property_count;
    fbx_property* properties;

    unsigned int chiled_node_count;
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