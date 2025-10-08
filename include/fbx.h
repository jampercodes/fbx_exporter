#include <stdio.h>

struct fbx_property {
    char type[4];
    void* data;
};

class fbx_node {
public:
    fbx_node(char *name);
    
private:
    char* node_name;

    unsigned int property_count;
    fbx_property* properties;

    unsigned int chiled_node_count;
    fbx_node* chiled_nodes;
};

class fbx_manager {
public:
    fbx_manager(char *filename, char *filelocation);
    ~fbx_manager();
    
    void add_mesh();

private:
    FILE* M_file;
};


const unsigned int FBX_VERSION = 6000;


fbx_node root_node;