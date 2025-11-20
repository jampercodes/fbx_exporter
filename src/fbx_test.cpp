#include <fbxsdk.h>


/**
 * Main function - loads the hard-coded fbx file,
 * and prints its contents in an xml format to stdout.
 */
int main(int argc, char** argv) {

    // Change the following filename to a suitable filename value.
    const char* lFilename = "file.fbx";
    
    // Initialize the SDK manager. This object handles all our memory management.
    FbxManager* lSdkManager = FbxManager::Create();
  
    
    // Destroy the SDK manager and all the other objects it was handling.
    lSdkManager->Destroy();
    return 0;
}