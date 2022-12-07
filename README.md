# Models

因为库文件较大，gitignore中暂时忽略所有Library文件

## 第三方库

### Assimp

版本：v5.2.5

#### CMake设置

ASSIMP_BIN_INSTALL_DIR: ./Library/bin/$(Platform)/$(Configuration)  
ASSIMP_LIB_INSTALL_DIR: ./Library/bin/$(Platform)/$(Configuration)  
CMAKE_INSTALL_PREFIX: ./Library  
LIBRARY_SUFFIX: (empty string)  

**注：以上所有路径皆为相对于本仓库的相对路径**