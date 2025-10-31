#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <cstdint>

const unsigned int FBX_VERSION = 6000;

// A simple property variant used by this minimal exporter.
struct FbxProperty {
    // type char as used by FBX binary (e.g. 'S' for string, 'I' for int, 'D' for double, lowercase for arrays in full spec)
    char type;
    std::vector<uint8_t> data; // raw bytes of the property payload
};

struct FbxTransform {
    double t[3]; // translation x,y,z
    double r[3]; // rotation x,y,z (degrees)
    double s[3]; // scale x,y,z
    FbxTransform() { t[0]=t[1]=t[2]=0.0; r[0]=r[1]=r[2]=0.0; s[0]=s[1]=s[2]=1.0; }
};

class FbxNode {
public:
    explicit FbxNode(const std::string& name = "");

    void addProperty(const FbxProperty& prop);
    void addChild(const FbxNode& child);

    // write node to stream (binary FBX node format version 6000)
    void write(FILE* f) const;

private:
    std::string name;
    std::vector<FbxProperty> properties;
    std::vector<FbxNode> children;
public:
    // const accessors used by serialization helpers
    const std::string& getName() const { return name; }
    const std::vector<FbxProperty>& getProperties() const { return properties; }
    const std::vector<FbxNode>& getChildren() const { return children; }
};

class fbx_manager {
public:
    fbx_manager(const std::string& filename, const std::string& filelocation);
    ~fbx_manager();

    // convenience helpers
    void addEmpty(const std::string& name);
    // returns material object id
    int64_t addMaterial(const std::string& name);
    // vertices: flat array of doubles [x0,y0,z0, x1,y1,z1, ...]
    // normals: optional flat array of doubles [nx0,ny0,nz0, ...]
    // uvs: optional flat array of doubles [u0,v0, u1,v1, ...]
    // materialId: optional material to assign to this mesh (pass -1 for none)
    void addMesh(const std::string& name,
                 const std::vector<double>& vertices,
                 const std::vector<uint32_t>& indices,
                 const std::vector<double>& normals = {},
                 const std::vector<double>& uvs = {},
                 const FbxTransform& xform = FbxTransform(),
                 int64_t materialId = -1);

    // finalize & write file (called automatically in destructor)
    void writeFile();

private:
    FILE* M_file;
    std::string filename;
    std::string filelocation;

    FbxNode root;
    FbxNode objectsNode;
    FbxNode connectionsNode;
    int64_t nextId = 1000; // simple id generator
    bool enableCompression = true;

    int64_t allocId() { return nextId++; }
};
