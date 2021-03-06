# Introduction 
TAMods Server

## Dependencies

You need to provide Boost v1.68.0 and the Nlohmann JSON library.

You should set the following environment variables for these:
- TAMODS_BOOST_INCLUDE_PATH - Path to Boost (i.e. the main Boost directory)
- TAMODS_BOOST_LIB_PATH  - Path to compiled Boost libraries (usually <Boost dir>\stage\lib)
- TAMODS_NLOHMANNJSON_INCLUDE_PATH - Path to Nlohmann JSON include directory

## Releasing

To release a new version:

1. Bump the version in buildconfig.h and in the TAMods-Server resource file (to set the DLL's product version)
2. Build for release
3. Clone the release branch of https://github.com/mcoot/tamodsupdate/tree/release
4. Add the DLL to that repo as TAMods-Server-<version>.dll
5. Commit and push that to release
6. In TAServer, update the version it grabs

You don't need to touch version.xml for this (you do for the clientside TAMods release process).