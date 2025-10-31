#include "fbx.h"
#include <cstring>
#include <cassert>
#include <zlib.h>
#include <limits>

static void write_uint32(FILE* f, uint32_t v) {
    fwrite(&v, sizeof(uint32_t), 1, f);
}

static void write_uint8(FILE* f, uint8_t v) {
    fwrite(&v, sizeof(uint8_t), 1, f);
}

// FbxNode implementation
FbxNode::FbxNode(const std::string& name_) : name(name_) {}

void FbxNode::addProperty(const FbxProperty& prop) {
    properties.push_back(prop);
}

void FbxNode::addChild(const FbxNode& child) {
    children.push_back(child);
}

// serialize properties into a buffer
static void serializeProperties(const std::vector<FbxProperty>& props, std::vector<uint8_t>& out) {
    for (const auto& p : props) {
        // property type (1 byte)
        out.push_back(static_cast<uint8_t>(p.type));
        // then raw payload as-is (we store the encoded bytes in p.data)
        out.insert(out.end(), p.data.begin(), p.data.end());
    }
}

// helper: build FBX array property payload (count, encoding, compressedLen, rawBytes or compressed)
static std::vector<uint8_t> buildArrayProperty(const void* rawData, size_t rawBytes, uint32_t elemCount, bool compress) {
    std::vector<uint8_t> out;
    uint32_t encoding = 0;
    uint32_t compLen = 0;

    const uint8_t* src = reinterpret_cast<const uint8_t*>(rawData);

    std::vector<uint8_t> payload;
    if (compress) {
        // compress using zlib
        uLongf destLen = compressBound((uLong)rawBytes);
        payload.resize(destLen);
        int zres = compress2(payload.data(), &destLen, src, (uLong)rawBytes, Z_BEST_SPEED);
        if (zres == Z_OK) {
            payload.resize(destLen);
            encoding = 1;
            compLen = (uint32_t)destLen;
        } else {
            // compression failed; fall back to uncompressed
            payload.assign(src, src + rawBytes);
            encoding = 0;
            compLen = 0;
        }
    } else {
        payload.assign(src, src + rawBytes);
        encoding = 0;
        compLen = 0;
    }

    // header: count(4), encoding(4), compressedLen(4)
    out.resize(12);
    memcpy(out.data() + 0, &elemCount, 4);
    memcpy(out.data() + 4, &encoding, 4);
    memcpy(out.data() + 8, &compLen, 4);
    // append payload
    out.insert(out.end(), payload.begin(), payload.end());
    return out;
}

// recursively serialize children into buffer
static void serializeChildren(const std::vector<FbxNode>& children, std::vector<uint8_t>& out) {
    for (const auto& c : children) {
        // We'll build child into a temporary buffer, compute its sizes and then write header + body here
        std::vector<uint8_t> childBody;
        // name
    uint8_t nameLen = static_cast<uint8_t>(c.getName().size());
    // properties
    serializeProperties(c.getProperties(), childBody);
    // children (recursive)
    serializeChildren(c.getChildren(), childBody);

        // compute endOffset: header (13 bytes) + nameLen + childBody.size()
        uint32_t endOffset = 13 + (uint32_t)nameLen + (uint32_t)childBody.size();

        // write header to out (little-endian assumed by platform)
        // endOffset
        out.insert(out.end(), reinterpret_cast<uint8_t*>(&endOffset), reinterpret_cast<uint8_t*>(&endOffset) + 4);
        // numProperties (4) - we will not parse this strictly, set to number of properties
    uint32_t numProps = static_cast<uint32_t>(c.getProperties().size());
        out.insert(out.end(), reinterpret_cast<uint8_t*>(&numProps), reinterpret_cast<uint8_t*>(&numProps) + 4);
        // propertyListLen (4) - length in bytes of properties
        uint32_t propListLen = 0;
    for (const auto& p : c.getProperties()) propListLen += (uint32_t)p.data.size() + 1; // +1 for type char
        out.insert(out.end(), reinterpret_cast<uint8_t*>(&propListLen), reinterpret_cast<uint8_t*>(&propListLen) + 4);
        // nameLen (1)
        out.push_back(nameLen);
        // name bytes
    out.insert(out.end(), c.getName().begin(), c.getName().end());
        // properties payload
        out.insert(out.end(), childBody.begin(), childBody.end());
        // Note: childBody already includes nested children after properties
    }
}

void FbxNode::write(FILE* f) const {
    // Write this node (top-level node). For top-level, follow same pattern but write directly to file.
    uint8_t nameLen = static_cast<uint8_t>(name.size());

    // build properties bytes
    std::vector<uint8_t> propsBytes;
    serializeProperties(properties, propsBytes);

    // build children bytes
    std::vector<uint8_t> childrenBytes;
    serializeChildren(children, childrenBytes);

    uint32_t endOffset = 13 + (uint32_t)nameLen + (uint32_t)propsBytes.size() + (uint32_t)childrenBytes.size();

    // write header
    fwrite(&endOffset, sizeof(uint32_t), 1, f);
    uint32_t numProps = static_cast<uint32_t>(properties.size());
    fwrite(&numProps, sizeof(uint32_t), 1, f);
    uint32_t propListLen = (uint32_t)propsBytes.size();
    fwrite(&propListLen, sizeof(uint32_t), 1, f);
    fwrite(&nameLen, sizeof(uint8_t), 1, f);
    // name
    if (nameLen) fwrite(name.data(), sizeof(char), nameLen, f);

    // properties
    if (!propsBytes.empty()) fwrite(propsBytes.data(), 1, propsBytes.size(), f);
    // children (already serialized)
    if (!childrenBytes.empty()) fwrite(childrenBytes.data(), 1, childrenBytes.size(), f);
}

// fbx_manager implementation
fbx_manager::fbx_manager(const std::string& filename_, const std::string& filelocation_) : M_file(nullptr), filename(filename_), filelocation(filelocation_), root("") {
    // create file for binary write
    std::string fullpath = filelocation + filename + ".fbx";
    M_file = fopen(fullpath.c_str(), "wb");
    if (!M_file) {
        // try current dir fallback
        M_file = fopen("test.fbx", "wb");
    }

    if (!M_file) return;

    // write header: "Kaydara FBX Binary  \0x00 0x1A 0x00" then version (4 bytes little endian)
    fwrite("Kaydara FBX Binary ", 1, 18, M_file); // note there's two spaces in original header
    unsigned char magic[] = {0x00, 0x1A, 0x00};
    fwrite(magic, 1, sizeof(magic), M_file);
    uint32_t version = FBX_VERSION;
    fwrite(&version, sizeof(uint32_t), 1, M_file);

    // prepare top-level Objects and Connections nodes
    objectsNode = FbxNode("Objects");
    connectionsNode = FbxNode("Connections");
}

fbx_manager::~fbx_manager() {
    // write file content (nodes + terminator) and close
    writeFile();
    if (M_file) {
        fclose(M_file);
        M_file = nullptr;
    }
}

void fbx_manager::addEmpty(const std::string& name) {
    int64_t id = allocId();
    FbxNode modelNode("Model::" + name);
    // id property (64-bit)
    FbxProperty idp;
    idp.type = 'L';
    idp.data.resize(8);
    int64_t tmpid = id;
    memcpy(idp.data.data(), &tmpid, 8);
    modelNode.addProperty(idp);

    // name property
    FbxProperty p;
    p.type = 'S';
    uint32_t len = (uint32_t)name.size();
    p.data.resize(4 + len);
    memcpy(p.data.data(), &len, 4);
    memcpy(p.data.data() + 4, name.data(), len);
    modelNode.addProperty(p);

    objectsNode.addChild(modelNode);
    // connection: Model -> (no parent) will be added later if needed
}

void fbx_manager::addMaterial(const std::string& name) {
    int64_t id = allocId();
    FbxNode mnode("Material::" + name);
    FbxProperty idp;
    idp.type = 'L';
    idp.data.resize(8);
    memcpy(idp.data.data(), &id, 8);
    mnode.addProperty(idp);

    FbxProperty p;
    p.type = 'S';
    uint32_t len = (uint32_t)name.size();
    p.data.resize(4 + len);
    memcpy(p.data.data(), &len, 4);
    memcpy(p.data.data() + 4, name.data(), len);
    mnode.addProperty(p);

    objectsNode.addChild(mnode);
}

void fbx_manager::addMesh(const std::string& name, const std::vector<double>& vertices, const std::vector<uint32_t>& indices, const std::vector<double>& normals, const std::vector<double>& uvs) {
    int64_t geomId = allocId();
    int64_t modelId = allocId();

    FbxNode gnode("Geometry::" + name);

    // id property
    FbxProperty idp;
    idp.type = 'L';
    idp.data.resize(8);
    memcpy(idp.data.data(), &geomId, 8);
    gnode.addProperty(idp);

    // name string
    FbxProperty namep;
    namep.type = 'S';
    uint32_t nlen = (uint32_t)name.size();
    namep.data.resize(4 + nlen);
    memcpy(namep.data.data(), &nlen, 4);
    memcpy(namep.data.data() + 4, name.data(), nlen);
    gnode.addProperty(namep);

    // vertices array: count = number of doubles
    if (!vertices.empty()) {
        uint32_t count = (uint32_t)vertices.size();
        bool tryCompress = enableCompression && ((size_t)count * sizeof(double) > 256);
        auto arr = buildArrayProperty(vertices.data(), count * sizeof(double), count, tryCompress);
        FbxProperty verts;
        verts.type = 'd';
        verts.data = std::move(arr);
        gnode.addProperty(verts);
    }

    // polygonVertexIndex: convert triangle list to FBX polygon indices (negate last index with ~)
    if (!indices.empty()) {
        std::vector<int32_t> polyIdx; polyIdx.reserve(indices.size());
        // assume triangles; every 3 indices is a polygon
        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t a = indices[i];
            uint32_t b = (i+1 < indices.size()) ? indices[i+1] : 0;
            uint32_t c = (i+2 < indices.size()) ? indices[i+2] : 0;
            polyIdx.push_back((int32_t)a);
            polyIdx.push_back((int32_t)b);
            polyIdx.push_back(~(int32_t)c); // mark end of polygon
        }
        uint32_t icount = (uint32_t)polyIdx.size();
        bool tryCompress = enableCompression && (icount * sizeof(int32_t) > 256);
        auto arr = buildArrayProperty(polyIdx.data(), icount * sizeof(int32_t), icount, tryCompress);
        FbxProperty idxs;
        idxs.type = 'i';
        idxs.data = std::move(arr);
        gnode.addProperty(idxs);
    }

    // normals
    if (!normals.empty()) {
        uint32_t count = (uint32_t)normals.size();
        bool tryCompress = enableCompression && (count * sizeof(double) > 256);
        auto arr = buildArrayProperty(normals.data(), count * sizeof(double), count, tryCompress);
        FbxProperty nprop;
        nprop.type = 'd';
        nprop.data = std::move(arr);
        gnode.addProperty(nprop);
    }

    // uvs (u,v pairs)
    if (!uvs.empty()) {
        uint32_t count = (uint32_t)uvs.size();
        bool tryCompress = enableCompression && (count * sizeof(double) > 256);
        auto arr = buildArrayProperty(uvs.data(), count * sizeof(double), count, tryCompress);
        FbxProperty uvprop;
        uvprop.type = 'd';
        uvprop.data = std::move(arr);
        gnode.addProperty(uvprop);
    }

    objectsNode.addChild(gnode);

    // create a Model node for the geometry
    FbxNode mnode("Model::" + name);
    FbxProperty midp;
    midp.type = 'L'; midp.data.resize(8); memcpy(midp.data.data(), &modelId, 8); mnode.addProperty(midp);
    FbxProperty mnamep; mnamep.type = 'S'; uint32_t mlen = (uint32_t)name.size(); mnamep.data.resize(4+mlen); memcpy(mnamep.data.data(), &mlen,4); memcpy(mnamep.data.data()+4, name.data(), mlen); mnode.addProperty(mnamep);

    objectsNode.addChild(mnode);

    // connection entries: store for writing later
    // Connection: Model -> Geometry
    FbxNode c1("C");
    FbxProperty ct; ct.type = 'S'; std::string typ = "OO"; ct.data.resize(4 + typ.size()); uint32_t tlen = (uint32_t)typ.size(); memcpy(ct.data.data(), &tlen, 4); memcpy(ct.data.data()+4, typ.data(), typ.size()); c1.addProperty(ct);
    FbxProperty fromp; fromp.type = 'L'; fromp.data.resize(8); memcpy(fromp.data.data(), &modelId, 8); c1.addProperty(fromp);
    FbxProperty top; top.type = 'L'; top.data.resize(8); memcpy(top.data.data(), &geomId, 8); c1.addProperty(top);
    connectionsNode.addChild(c1);

}

void fbx_manager::writeFile() {
    if (!M_file) return;

    // assemble root children: Objects then Connections
    root.addChild(objectsNode);
    root.addChild(connectionsNode);

    // write root's children
    for (const auto& child : root.getChildren()) {
        child.write(M_file);
    }

    // write 13 zero bytes terminator
    uint8_t zero13[13] = {0};
    fwrite(zero13, 1, sizeof(zero13), M_file);

    // already wrote header (version) in constructor; keep file open until destructor
}
