#include <fbxsdk.h>


/**
 * Main function - loads the hard-coded fbx file,
 * and prints its contents in an xml format to stdout.
 */
int main(int argc, char** argv) {

    // Change the following filename to a suitable filename value.
    const char* lFilename = "test.fbx";
    
    // Initialize the SDK manager. This object handles all our memory management.
    FbxManager* lSdkManager = FbxManager::Create();
  
    FbxIOSettings* ios = FbxIOSettings::Create(lSdkManager, IOSROOT);
    lSdkManager->SetIOSettings(ios);


    FbxExporter* lExporter = FbxExporter::Create(lSdkManager, "");


    bool lExportStatus = lExporter->Initialize(lFilename, -1, lSdkManager->GetIOSettings());
    
    if(!lExportStatus) {
        printf("Call to FbxExporter::Initialize() failed.\n");
        printf("Error returned: %s\n\n", lExporter->GetStatus().GetErrorString());
        return 1;
    }

    // Create a new scene so it can be populated by the imported file.
    FbxScene* lScene = FbxScene::Create(lSdkManager,"myScene");

    // test building a simpell scene 
    FbxNode* lRootNode = lScene->GetRootNode();


    FbxNode* lTestNode = FbxNode::Create(lScene, "TestNode");

    FbxMesh* lTestMesh = FbxMesh::Create(lScene, "TestMesh");
    lTestNode->SetNodeAttribute(lTestMesh);

    lRootNode->AddChild(lTestNode);

    //divine mesh
    // Define the eight corners of the cube.
    // The cube spans from
    //    -50 to  50 along the X axis
    //      0 to 100 along the Y axis
    //    -50 to  50 along the Z axis
    FbxVector4 vertex0(-50, 0, 50);
    FbxVector4 vertex1(50, 0, 50);
    FbxVector4 vertex2(50, 100, 50);
    FbxVector4 vertex3(-50, 100, 50);
    FbxVector4 vertex4(-50, 0, -50);
    FbxVector4 vertex5(50, 0, -50);
    FbxVector4 vertex6(50, 100, -50);
    FbxVector4 vertex7(-50, 100, -50);

    // Initialize the control point array of the mesh.
    lTestMesh->InitControlPoints(24);
    FbxVector4* lControlPoints = lTestMesh->GetControlPoints();

    // Define each face of the cube.
    // Face 1
    lControlPoints[0] = vertex0;
    lControlPoints[1] = vertex1;
    lControlPoints[2] = vertex2;
    lControlPoints[3] = vertex3;
    // Face 2
    lControlPoints[4] = vertex1;
    lControlPoints[5] = vertex5;
    lControlPoints[6] = vertex6;
    lControlPoints[7] = vertex2;
    // Face 3
    lControlPoints[8] = vertex5;
    lControlPoints[9] = vertex4;
    lControlPoints[10] = vertex7;
    lControlPoints[11] = vertex6;
    // Face 4
    lControlPoints[12] = vertex4;
    lControlPoints[13] = vertex0;
    lControlPoints[14] = vertex3;
    lControlPoints[15] = vertex7;
    // Face 5
    lControlPoints[16] = vertex3;
    lControlPoints[17] = vertex2;
    lControlPoints[18] = vertex6;
    lControlPoints[19] = vertex7;
    // Face 6
    lControlPoints[20] = vertex1;
    lControlPoints[21] = vertex0;
    lControlPoints[22] = vertex4;
    lControlPoints[23] = vertex5;


    // Export the scene to the file.
    lExporter->Export(lScene);

    // Destroy the SDK manager and all the other objects it was handling.
    lSdkManager->Destroy();
    return 0;
}