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

// small helpers to create typed properties
static FbxProperty makeStringProp(const std::string& s) {
    FbxProperty p; p.type = 'S'; uint32_t len = (uint32_t)s.size(); p.data.resize(4 + len); memcpy(p.data.data(), &len, 4); memcpy(p.data.data() + 4, s.data(), len); return p;
}

static FbxProperty makeIntProp(int32_t v) {
    FbxProperty p; p.type = 'I'; p.data.resize(4); memcpy(p.data.data(), &v, 4); return p;
}

static FbxProperty makeLongProp(int64_t v) {
    FbxProperty p; p.type = 'L'; p.data.resize(8); memcpy(p.data.data(), &v, 8); return p;
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

int64_t fbx_manager::addMaterial(const std::string& name) {
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
    return id;
}

void fbx_manager::addMesh(const std::string& name, const std::vector<double>& vertices, const std::vector<uint32_t>& indices, const std::vector<double>& normals, const std::vector<double>& uvs, const FbxTransform& xform, int64_t materialId) {
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
    std::vector<int32_t> polyIdx;
    if (!indices.empty()) {
        polyIdx.reserve(indices.size());
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

    // For LayerElement mapping ByPolygonVertex, create arrays that follow polygon-vertex order
    size_t polyVertCount = polyIdx.size();
    // normals: remap per polygon-vertex if normals provided per control point
    if (!normals.empty() && polyVertCount > 0) {
        std::vector<double> normalsByPV; normalsByPV.resize(polyVertCount * 3);
        for (size_t k = 0; k < polyVertCount; ++k) {
            int32_t v = polyIdx[k];
            int32_t cp = (v < 0) ? ~v : v;
            // bounds check
            size_t base = (size_t)cp * 3;
            if (base + 2 < normals.size()) {
                normalsByPV[k*3 + 0] = normals[base + 0];
                normalsByPV[k*3 + 1] = normals[base + 1];
                normalsByPV[k*3 + 2] = normals[base + 2];
            } else {
                normalsByPV[k*3 + 0] = 0.0;
                normalsByPV[k*3 + 1] = 0.0;
                normalsByPV[k*3 + 2] = 1.0;
            }
        }
        uint32_t count = (uint32_t)normalsByPV.size();
        bool tryCompress = enableCompression && (count * sizeof(double) > 256);
        auto arr = buildArrayProperty(normalsByPV.data(), count * sizeof(double), count, tryCompress);
        FbxProperty nprop; nprop.type = 'd'; nprop.data = std::move(arr); gnode.addProperty(nprop);
    }

    // uvs: remap per polygon-vertex (u,v pairs)
    if (!uvs.empty() && polyVertCount > 0) {
        std::vector<double> uvsByPV; uvsByPV.resize(polyVertCount * 2);
        for (size_t k = 0; k < polyVertCount; ++k) {
            int32_t v = polyIdx[k];
            int32_t cp = (v < 0) ? ~v : v;
            size_t base = (size_t)cp * 2;
            if (base + 1 < uvs.size()) {
                uvsByPV[k*2 + 0] = uvs[base + 0];
                uvsByPV[k*2 + 1] = uvs[base + 1];
            } else {
                uvsByPV[k*2 + 0] = 0.0;
                uvsByPV[k*2 + 1] = 0.0;
            }
        }
        uint32_t count = (uint32_t)uvsByPV.size();
        bool tryCompress = enableCompression && (count * sizeof(double) > 256);
        auto arr = buildArrayProperty(uvsByPV.data(), count * sizeof(double), count, tryCompress);
        FbxProperty uvprop; uvprop.type = 'd'; uvprop.data = std::move(arr); gnode.addProperty(uvprop);
    }

    objectsNode.addChild(gnode);

    // create a Model node for the geometry
    FbxNode mnode("Model::" + name);
    FbxProperty midp;
    midp.type = 'L'; midp.data.resize(8); memcpy(midp.data.data(), &modelId, 8); mnode.addProperty(midp);
    FbxProperty mnamep; mnamep.type = 'S'; uint32_t mlen = (uint32_t)name.size(); mnamep.data.resize(4+mlen); memcpy(mnamep.data.data(), &mlen,4); memcpy(mnamep.data.data()+4, name.data(), mlen); mnode.addProperty(mnamep);

    // add transform info in a Properties70 child using P entries (Lcl Translation/Rotation/Scaling)
    FbxNode props70("Properties70");
    char buf[256];
    // Translation
    snprintf(buf, sizeof(buf), "P|Lcl Translation|Lcl Translation|Lcl Translation|Vector3D|%g|%g|%g", xform.t[0], xform.t[1], xform.t[2]);
    props70.addProperty(makeStringProp(std::string(buf)));
    // Rotation
    snprintf(buf, sizeof(buf), "P|Lcl Rotation|Lcl Rotation|Lcl Rotation|Vector3D|%g|%g|%g", xform.r[0], xform.r[1], xform.r[2]);
    props70.addProperty(makeStringProp(std::string(buf)));
    // Scaling
    snprintf(buf, sizeof(buf), "P|Lcl Scaling|Lcl Scaling|Lcl Scaling|Vector3D|%g|%g|%g", xform.s[0], xform.s[1], xform.s[2]);
    props70.addProperty(makeStringProp(std::string(buf)));
    mnode.addChild(props70);

    objectsNode.addChild(mnode);

    // connection entries: Model -> Geometry
    FbxNode c1("C");
    FbxProperty ct; ct.type = 'S'; std::string typ = "OO"; ct.data.resize(4 + typ.size()); uint32_t tlen = (uint32_t)typ.size(); memcpy(ct.data.data(), &tlen, 4); memcpy(ct.data.data()+4, typ.data(), typ.size()); c1.addProperty(ct);
    FbxProperty fromp; fromp.type = 'L'; fromp.data.resize(8); memcpy(fromp.data.data(), &modelId, 8); c1.addProperty(fromp);
    FbxProperty top; top.type = 'L'; top.data.resize(8); memcpy(top.data.data(), &geomId, 8); c1.addProperty(top);
    connectionsNode.addChild(c1);

    // If material provided, add Connection: Model <- Material (Material -> Model)
    if (materialId != -1) {
        FbxNode cmat("C");
        FbxProperty ctt; ctt.type = 'S'; std::string ttyp = "OO"; ctt.data.resize(4 + ttyp.size()); uint32_t ttlen = (uint32_t)ttyp.size(); memcpy(ctt.data.data(), &ttlen, 4); memcpy(ctt.data.data()+4, ttyp.data(), ttyp.size()); cmat.addProperty(ctt);
        FbxProperty fromm; fromm.type = 'L'; fromm.data.resize(8); memcpy(fromm.data.data(), &materialId, 8); cmat.addProperty(fromm);
        FbxProperty tom; tom.type = 'L'; tom.data.resize(8); memcpy(tom.data.data(), &modelId, 8); cmat.addProperty(tom);
        connectionsNode.addChild(cmat);
    }

    // Add LayerElement nodes for normals and UVs (mapping: ByVertice, reference: Direct)
    if (!normals.empty()) {
        FbxNode len("LayerElementNormal");
        // mapping
        FbxProperty map; map.type = 'S'; std::string mapv = "ByVertice"; map.data.resize(4 + mapv.size()); uint32_t mpsz = (uint32_t)mapv.size(); memcpy(map.data.data(), &mpsz, 4); memcpy(map.data.data()+4, mapv.data(), mapv.size()); len.addProperty(map);
        // reference
        FbxProperty ref; ref.type = 'S'; std::string refv = "Direct"; ref.data.resize(4 + refv.size()); uint32_t rpsz = (uint32_t)refv.size(); memcpy(ref.data.data(), &rpsz, 4); memcpy(ref.data.data()+4, refv.data(), refv.size()); len.addProperty(ref);
        // direct array
        FbxProperty nd; nd.type = 'd'; auto narr = buildArrayProperty(normals.data(), normals.size()*sizeof(double), (uint32_t)normals.size(), enableCompression); nd.data = std::move(narr); len.addProperty(nd);
        gnode.addChild(len);
    }

    if (!uvs.empty()) {
        FbxNode leuv("LayerElementUV");
        FbxProperty map; map.type = 'S'; std::string mapv = "ByVertice"; map.data.resize(4 + mapv.size()); uint32_t mpsz = (uint32_t)mapv.size(); memcpy(map.data.data(), &mpsz, 4); memcpy(map.data.data()+4, mapv.data(), mapv.size()); leuv.addProperty(map);
        FbxProperty ref; ref.type = 'S'; std::string refv = "Direct"; ref.data.resize(4 + refv.size()); uint32_t rpsz = (uint32_t)refv.size(); memcpy(ref.data.data(), &rpsz, 4); memcpy(ref.data.data()+4, refv.data(), refv.size()); leuv.addProperty(ref);
        FbxProperty uvd; uvd.type = 'd'; auto uarr = buildArrayProperty(uvs.data(), uvs.size()*sizeof(double), (uint32_t)uvs.size(), enableCompression); uvd.data = std::move(uarr); leuv.addProperty(uvd);
        gnode.addChild(leuv);
    }
}
void fbx_manager::writeFile() {
    if (!M_file) return;

    // Build minimal header/definitions before Objects to improve importer compatibility
    // FBXHeaderExtension (minimal)
    FbxNode headerExt("FBXHeaderExtension");
    headerExt.addProperty(makeIntProp(1003)); // FBXHeaderVersion (example)
    headerExt.addProperty(makeIntProp((int32_t)FBX_VERSION));

    // Definitions: count object types present
    FbxNode defs("Definitions");
    // count how many ObjectType children we'll emit
    int geometryCount = 0, modelCount = 0, materialCount = 0;
    for (const auto& o : objectsNode.getChildren()) {
        const std::string& nm = o.getName();
        if (nm.rfind("Geometry::", 0) == 0) ++geometryCount;
        else if (nm.rfind("Model::", 0) == 0) ++modelCount;
        else if (nm.rfind("Material::", 0) == 0) ++materialCount;
    }
    int types = 0;
    if (geometryCount) ++types;
    if (modelCount) ++types;
    if (materialCount) ++types;
    defs.addProperty(makeIntProp(types));
    // ObjectType children
    if (geometryCount) {
        FbxNode ot("ObjectType");
        ot.addProperty(makeStringProp("Geometry"));
        ot.addProperty(makeIntProp(geometryCount));
        defs.addChild(ot);
    }
    if (modelCount) {
        FbxNode ot("ObjectType");
        ot.addProperty(makeStringProp("Model"));
        ot.addProperty(makeIntProp(modelCount));
        defs.addChild(ot);
    }
    if (materialCount) {
        FbxNode ot("ObjectType");
        ot.addProperty(makeStringProp("Material"));
        ot.addProperty(makeIntProp(materialCount));
        defs.addChild(ot);
    }

    // assemble root children in canonical order: HeaderExtension, Definitions, Objects, Connections
    root.addChild(headerExt);
    root.addChild(defs);
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
