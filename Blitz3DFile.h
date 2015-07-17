#ifndef _BLITZ3DFILE_H_
#define _BLITZ3DFILE_H_

#include <stdio.h>

/* Blitz3D structures */

typedef struct Blitz3DTexture Blitz3DTexture;
struct Blitz3DTexture;

typedef struct Blitz3DTEXSChunk Blitz3DTEXSChunk;
struct Blitz3DTEXSChunk;

typedef struct Blitz3DBrush Blitz3DBrush;
struct Blitz3DBrush;

typedef struct Blitz3DBRUSChunk Blitz3DBRUSChunk;
struct Blitz3DBRUSChunk;

typedef struct Blitz3DVertexNormal Blitz3DVertexNormal;
struct Blitz3DVertexNormal;

typedef struct Blitz3DVertexColor Blitz3DVertexColor;
struct Blitz3DVertexColor;

typedef struct Blitz3DVertex Blitz3DVertex;
struct Blitz3DVertex;

typedef struct Blitz3DVRTSChunk Blitz3DVRTSChunk;
struct Blitz3DVRTSChunk;

typedef struct Blitz3DTriangle Blitz3DTriangle;
struct Blitz3DTriangle;

typedef struct Blitz3DTRISChunk Blitz3DTRISChunk;
struct Blitz3DTRISChunk;

typedef struct Blitz3DMESHChunk Blitz3DMESHChunk;
struct Blitz3DMESHChunk;

typedef struct Blitz3DNODEChunk Blitz3DNODEChunk;
struct Blitz3DNODEChunk;

typedef struct Blitz3DBB3DChunk Blitz3DBB3DChunk;
struct Blitz3DBB3DChunk;

typedef struct B3DFile B3DFile;
struct B3DFile;

/* public functions */

B3DFile* loadB3DFile(const char* filePath);

Blitz3DBB3DChunk* getBB3DChunkFromFile(B3DFile* blitz3dFile);

char* getDirectoryFromFile(B3DFile* blitz3dFile);

Blitz3DTEXSChunk* getTEXSChunkFromBB3DChunk(Blitz3DBB3DChunk* bb3dChunk);

unsigned int getTextureArrayCountFromTEXSChunk(Blitz3DTEXSChunk* texsChunk);

Blitz3DTexture* getTextureArrayEntryFromTEXSChunk(Blitz3DTEXSChunk* texsChunk, unsigned int index);

char* getFileFromTexture(Blitz3DTexture* texture);

Blitz3DBRUSChunk* getBRUSChunkFromBB3DChunk(Blitz3DBB3DChunk* bb3dChunk);

int getNumberOfTexturesFromBRUSChunk(Blitz3DBRUSChunk* brusChunk);

unsigned int getBrushArrayCountFromBRUSChunk(Blitz3DBRUSChunk* brusChunk);

Blitz3DBrush* getBrushArrayEntryFromBRUSChunk(Blitz3DBRUSChunk* brusChunk, unsigned int index);

int getTextureIdArrayEntryFromBrush(Blitz3DBrush* brush, unsigned int index);

Blitz3DNODEChunk* getNODEChunkFromBB3DChunk(Blitz3DBB3DChunk* bb3dChunk);

Blitz3DMESHChunk* getMESHChunkFromNODEChunk(Blitz3DNODEChunk* nodeChunk);

unsigned int getNODEChunkArrayCountFromNodeChunk(Blitz3DNODEChunk* nodeChunk);

Blitz3DNODEChunk* getNODEChunkArrayEntryFromNODEChunk(Blitz3DNODEChunk* nodeChunk, unsigned int index);

Blitz3DVRTSChunk* getVRTSChunkFromMESHChunk(Blitz3DMESHChunk* meshChunk);

unsigned int getTRISChunkArrayCountFromMESHChunk(Blitz3DMESHChunk* meshChunk);

Blitz3DTRISChunk* getTRISChunkArrayEntryFromMESHChunk(Blitz3DMESHChunk* meshChunk, unsigned int index);

float* getVertexArrayFromVRTSChunk(Blitz3DVRTSChunk* vrtsChunk);

float* getNormalArrayFromVRTSChunk(Blitz3DVRTSChunk* vrtsChunk);

float* getColorArrayFromVRTSChunk(Blitz3DVRTSChunk* vrtsChunk);

unsigned int getTexCoordArrayCountFromVRTSChunk(Blitz3DVRTSChunk* vrtsChunk);

float* getTexCoordArrayEntryFromVRTSChunk(Blitz3DVRTSChunk* vrtsChunk, unsigned int index);

unsigned int getTexCoordArrayComponentCountFromVRTSChunk(Blitz3DVRTSChunk* vrtsChunk);

int normalArrayPresentInVRTSChunk(Blitz3DVRTSChunk* vrtsChunk);

int colorArrayPresentInVRTSChunk(Blitz3DVRTSChunk* vrtsChunk);

unsigned getTriangleCountFromTRISChunk(Blitz3DTRISChunk* trisChunk);

int* getTriangleIndexArrayFromTRISChunk(Blitz3DTRISChunk* trisChunk);

#endif
