# Third-party Export/Import Notices

FreeCrafter can export geometry to OBJ, STL, FBX, DAE, and glTF interchange formats. OBJ/STL/glTF exporters operate entirely on FreeCrafter code paths with no external licensing obligations. FBX and DAE writers are wired for Assimp when the library is present at build time; distributing those binaries requires compliance with the [Assimp license](https://github.com/assimp/assimp/blob/master/LICENSE).

SketchUp (`.skp`) export support requires the proprietary SketchUp SDK from Trimble. Because the SDK imposes additional licensing constraints, the application automatically hides the SKP option unless `FREECRAFTER_HAS_SKP_SDK` is defined and the integration layer is available. Builds without the SDK will fall back to COLLADA (`.dae`) or glTF (`.gltf`) exports for downstream SketchUp workflows.

Autodesk FBX and Trimble SketchUp are trademarks of their respective owners. Users are responsible for meeting any downstream EULA or redistribution obligations when enabling the optional integrations.
