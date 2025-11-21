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
    FbxVector4 vertex0(500, 0, 500);
    FbxVector4 vertex1(-500, 0, 500);
    FbxVector4 vertex2(500, 0, -500);
    FbxVector4 vertex3(-500, 0, -500);

    lTestMesh->InitControlPoints(4);

    lTestMesh->SetControlPointAt(vertex0, 0);
    lTestMesh->SetControlPointAt(vertex1, 1);
    lTestMesh->SetControlPointAt(vertex2, 2);
    lTestMesh->SetControlPointAt(vertex3, 3);
    

    // Export the scene to the file.
    lExporter->Export(lScene);

    // Destroy the SDK manager and all the other objects it was handling.
    lSdkManager->Destroy();
    return 0;
}