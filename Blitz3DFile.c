#include "Blitz3DFile.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "Stack.h"

/* Blitz3D defines */

#define BLITZ3D_TAG_BB3D_LITTLE_ENDIAN 0x44334242
#define BLITZ3D_TAG_TEXS_LITTLE_ENDIAN 0x53584554
#define BLITZ3D_TAG_BRUS_LITTLE_ENDIAN 0x53555242
#define BLITZ3D_TAG_NODE_LITTLE_ENDIAN 0x45444F4E
#define BLITZ3D_TAG_MESH_LITTLE_ENDIAN 0x4853454D
#define BLITZ3D_TAG_VRTS_LITTLE_ENDIAN 0x53545256
#define BLITZ3D_TAG_TRIS_LITTLE_ENDIAN 0x53495254

#define BLITZ3D_VERTEX_FLAG_NORMAL 1
#define BLITZ3D_VERTEX_FLAG_COLOR 2

/* Blitz3D structures */

struct Blitz3DTexture {
    char* file;

    int flags, blend;
    float x_pos, y_pos;
    float x_scale, y_scale;
    float rotation;
};

struct Blitz3DTEXSChunk {
    Blitz3DTexture** textureArray;

    unsigned int textureCount;
};

struct Blitz3DBrush {
    char* name;
    int* texture_id;

    float red, green, blue, alpha;
    float shininess;
    int blend, fx;
};

struct Blitz3DBRUSChunk {
    Blitz3DBrush** brushArray;

    unsigned int brushCount;

    int n_texs;
};

struct Blitz3DVertexNormal {
    float nx, ny, nz;
};

struct Blitz3DVertexColor {
    float red, green, blue, alpha;
};

struct Blitz3DVertex {
    Blitz3DVertexNormal* vertexNormal;
    Blitz3DVertexColor* vertexColor;
    float** tex_coords;

    float x, y, z;
};

struct Blitz3DVRTSChunk {
    float* vertexArray;
    float* normalArray;
    float* colorArray;
    float** texCoordArrays;

    unsigned int vertexCount;

    int flags;
    int tex_coord_sets;
    int tex_coord_set_size;
};

struct Blitz3DTriangle {
    int vertex_id[3];
};

struct Blitz3DTRISChunk {
    int* indexArray;

    unsigned int triangleCount;

    int brush_id;
};

struct Blitz3DMESHChunk {
    Blitz3DVRTSChunk* vrtsChunk;
    Blitz3DTRISChunk** trisChunkArray;

    unsigned int trisChunkCount;

    int brush_id;
};

struct Blitz3DNODEChunk {
    Blitz3DMESHChunk* meshChunk;
    Blitz3DNODEChunk** nodeChunkArray;
    char* name;

    unsigned int nodeChunkCount;

    float position[3];
    float scale[3];
    float rotation[4];
};

struct Blitz3DBB3DChunk {
    Blitz3DTEXSChunk* texsChunk;
    Blitz3DBRUSChunk* brusChunk;
    Blitz3DNODEChunk* nodeChunk;

    int version;
};

struct B3DFile {
    Blitz3DBB3DChunk* bb3dChunk;
    /*char* filePath;*/
};

/* binary read functions */

char* readStringFromBinaryFile(FILE* fp) {
    int position = 0;
    int capacity = 8;
    char* buffer;
    char* output;
    char temp;

    buffer = (char*)malloc(capacity);

    while ( (temp = fgetc(fp)) != '\0' ) {
        if (position >= capacity) {
            char* temporaryBuffer = (char*)malloc(2 * capacity);
            memcpy(temporaryBuffer, buffer, capacity);

            free(buffer);
            buffer = temporaryBuffer;
            capacity *= 2;
        }

        buffer[position] = temp;
        position++;
    }

    output = (char*)malloc(position + 1);
    memcpy(output, buffer, position);
    free(buffer);

    output[position] = '\0';
    return output;
}

int read32BitIntegerFromBinaryFile(FILE* stream, uint32_t* address, int littleEndian) {
    int byteValues[4];

    byteValues[0] = fgetc(stream);
    byteValues[1] = fgetc(stream);
    byteValues[2] = fgetc(stream);
    byteValues[3] = fgetc(stream);

    
    if ((byteValues[0] == EOF) || (byteValues[1] == EOF)
        || (byteValues[2] == EOF) || (byteValues[3] == EOF)) return -1;

    *address = 0;

    if (littleEndian) {
        *address |= (byteValues[0] << 0x00);
        *address |= (byteValues[1] << 0x08);
        *address |= (byteValues[2] << 0x10);
        *address |= (byteValues[3] << 0x18);
    }
    else {
        *address |= (byteValues[0] << 0x18);
        *address |= (byteValues[1] << 0x10);
        *address |= (byteValues[2] << 0x08);
        *address |= (byteValues[3] << 0x00);
    }

    return 0;
}

/* Blitz3D binary read functions */

void skipBlitz3DChunk(FILE* fp) {
    int size;

    read32BitIntegerFromBinaryFile(fp, &size, 1);

    fseek(fp, size, SEEK_CUR);
}

Blitz3DTEXSChunk* readBlitz3DTEXSChunk(FILE* fp) {
    Blitz3DTEXSChunk* output;
    Stack* textureStack;
    int size, startingPoint, iter;

    output = (Blitz3DTEXSChunk*)malloc(sizeof(Blitz3DTEXSChunk));
    textureStack = createStack();

    read32BitIntegerFromBinaryFile(fp, &size, 1);

    printf("size = %d\n", size);
    startingPoint = ftell(fp);

    /* reading stuff here */

    while (ftell(fp) < startingPoint + size) {
        Blitz3DTexture* texture = (Blitz3DTexture*)malloc(sizeof(Blitz3DTexture));

        texture->file = readStringFromBinaryFile(fp);

        read32BitIntegerFromBinaryFile(fp, &(texture->flags), 1);
        read32BitIntegerFromBinaryFile(fp, &(texture->blend), 1);

        read32BitIntegerFromBinaryFile(fp, (int*)&(texture->x_pos), 1);
        read32BitIntegerFromBinaryFile(fp, (int*)&(texture->y_pos), 1);
        read32BitIntegerFromBinaryFile(fp, (int*)&(texture->x_scale), 1);
        read32BitIntegerFromBinaryFile(fp, (int*)&(texture->y_scale), 1);
        read32BitIntegerFromBinaryFile(fp, (int*)&(texture->rotation), 1);

        pushOntoStack(textureStack, (void*)texture);
    }

    output->textureCount = getStackCount(textureStack);
    output->textureArray = (Blitz3DTexture**)malloc(output->textureCount * sizeof(Blitz3DTexture*));

    for (iter = 0; iter < output->textureCount; iter++) {
        output->textureArray[output->textureCount - iter - 1] = (Blitz3DTexture*)popOffOfStack(textureStack);
    }

    fseek(fp, size + startingPoint - ftell(fp), SEEK_CUR);

    freeStack(textureStack);

    return output;
}

Blitz3DBRUSChunk* readBlitz3DBRUSChunk(FILE* fp) {
    Blitz3DBRUSChunk* output;
    Stack* brushStack;
    int size, startingPoint, iter;

    output = (Blitz3DBRUSChunk*)malloc(sizeof(Blitz3DBRUSChunk));
    brushStack = createStack();

    read32BitIntegerFromBinaryFile(fp, &size, 1);

    printf("size = %d\n", size);
    startingPoint = ftell(fp);

    /* reading stuff here */

    read32BitIntegerFromBinaryFile(fp, &(output->n_texs), 1);

    printf("nTexs = %d\n", output->n_texs);

    while (ftell(fp) < startingPoint + size) {
        Blitz3DBrush* brush = (Blitz3DBrush*)malloc(sizeof(Blitz3DBrush));

        brush->name = readStringFromBinaryFile(fp);

        read32BitIntegerFromBinaryFile(fp, (int*)&(brush->red), 1);
        read32BitIntegerFromBinaryFile(fp, (int*)&(brush->green), 1);
        read32BitIntegerFromBinaryFile(fp, (int*)&(brush->blue), 1);
        read32BitIntegerFromBinaryFile(fp, (int*)&(brush->alpha), 1);

        read32BitIntegerFromBinaryFile(fp, (int*)&(brush->shininess), 1);

        read32BitIntegerFromBinaryFile(fp, &(brush->blend), 1);
        read32BitIntegerFromBinaryFile(fp, &(brush->fx), 1);

        brush->texture_id = (int*)malloc(output->n_texs * sizeof(int));

        for (iter = 0; iter < output->n_texs; iter++) {
            read32BitIntegerFromBinaryFile(fp, &(brush->texture_id[iter]), 1);
        }

        pushOntoStack(brushStack, (void*)brush);
    }

    output->brushCount = getStackCount(brushStack);
    output->brushArray = (Blitz3DBrush**)malloc(output->brushCount * sizeof(Blitz3DBrush*));

    for (iter = 0; iter < output->brushCount; iter++) {
        output->brushArray[output->brushCount - iter - 1] = (Blitz3DBrush*)popOffOfStack(brushStack);
    }

    fseek(fp, size + startingPoint - ftell(fp), SEEK_CUR);

    freeStack(brushStack);

    return output;
}

Blitz3DVRTSChunk* readBlitz3DVRTSChunk(FILE* fp) {
    Blitz3DVRTSChunk* output;
    Stack* vertexStack;
    int size, startingPoint, iter, texCoordIter, texComponentIter;

    output = (Blitz3DVRTSChunk*)calloc(1, sizeof(Blitz3DVRTSChunk));
    vertexStack = createStack();

    read32BitIntegerFromBinaryFile(fp, &size, 1);

    printf("size = %d\n", size);
    startingPoint = ftell(fp);

    read32BitIntegerFromBinaryFile(fp, &(output->flags), 1);
    read32BitIntegerFromBinaryFile(fp, &(output->tex_coord_sets), 1);
    read32BitIntegerFromBinaryFile(fp, &(output->tex_coord_set_size), 1);

    printf("flags = %d\n", output->flags);
    printf("tex_coord_sets = %d\n", output->tex_coord_sets);
    printf("tex_coord_set_size = %d\n", output->tex_coord_set_size);

    while (ftell(fp) < startingPoint + size) {
        Blitz3DVertex* vertex = (Blitz3DVertex*)malloc(sizeof(Blitz3DVertex));

        read32BitIntegerFromBinaryFile(fp, (int*)&(vertex->x), 1);
        read32BitIntegerFromBinaryFile(fp, (int*)&(vertex->y), 1);
        read32BitIntegerFromBinaryFile(fp, (int*)&(vertex->z), 1);

        if (output->flags & BLITZ3D_VERTEX_FLAG_NORMAL) {
            Blitz3DVertexNormal* normal = (Blitz3DVertexNormal*)malloc(sizeof(Blitz3DVertexNormal));

            read32BitIntegerFromBinaryFile(fp, (int*)&(normal->nx), 1);
            read32BitIntegerFromBinaryFile(fp, (int*)&(normal->ny), 1);
            read32BitIntegerFromBinaryFile(fp, (int*)&(normal->nz), 1);

            vertex->vertexNormal = normal;
        }

        if (output->flags & BLITZ3D_VERTEX_FLAG_COLOR) {
            Blitz3DVertexColor* color = (Blitz3DVertexColor*)malloc(sizeof(Blitz3DVertexColor));

            read32BitIntegerFromBinaryFile(fp, (int*)&(color->red), 1);
            read32BitIntegerFromBinaryFile(fp, (int*)&(color->green), 1);
            read32BitIntegerFromBinaryFile(fp, (int*)&(color->blue), 1);
            read32BitIntegerFromBinaryFile(fp, (int*)&(color->alpha), 1);

            vertex->vertexColor = color;
        }

        vertex->tex_coords = (float**)malloc(output->tex_coord_sets * sizeof(float*));

        for (iter = 0; iter < output->tex_coord_sets; iter++) {
            vertex->tex_coords[iter] = (float*)malloc(output->tex_coord_set_size * sizeof(float));

            for (texCoordIter = 0; texCoordIter < output->tex_coord_set_size; texCoordIter++) {
                read32BitIntegerFromBinaryFile(fp, (int*)&(vertex->tex_coords[iter][texCoordIter]), 1);
            }
        }

        pushOntoStack(vertexStack, (void*)vertex);
    }

    output->vertexCount = getStackCount(vertexStack);

    output->vertexArray = (float*)malloc(output->vertexCount * 3 * sizeof(float));
    if (output->flags & BLITZ3D_VERTEX_FLAG_NORMAL) {
        output->normalArray = (float*)malloc(output->vertexCount * 3 * sizeof(float));
    }
    if (output->flags & BLITZ3D_VERTEX_FLAG_COLOR) {
        output->colorArray = (float*)malloc(output->vertexCount * 4 * sizeof(float));
    }

    output->texCoordArrays = (float**)malloc(output->tex_coord_sets * sizeof(float));
    for (iter = 0; iter < output->tex_coord_sets; iter++) {
        output->texCoordArrays[iter] = (float*)malloc(output->vertexCount * output->tex_coord_set_size * sizeof(float));
    }

    for (iter = 0; iter < output->vertexCount; iter++) {
        Blitz3DVertex* vertex = (Blitz3DVertex*)popOffOfStack(vertexStack);

        output->vertexArray[3 * (output->vertexCount - iter - 1) + 0] = vertex->x;
        output->vertexArray[3 * (output->vertexCount - iter - 1) + 1] = vertex->y;
        output->vertexArray[3 * (output->vertexCount - iter - 1) + 2] = vertex->z;

        if (output->flags & BLITZ3D_VERTEX_FLAG_NORMAL) {
            output->normalArray[3 * (output->vertexCount - iter - 1) + 0] = vertex->vertexNormal->nx;
            output->normalArray[3 * (output->vertexCount - iter - 1) + 1] = vertex->vertexNormal->ny;
            output->normalArray[3 * (output->vertexCount - iter - 1) + 2] = vertex->vertexNormal->nz;
        }

        if (output->flags & BLITZ3D_VERTEX_FLAG_COLOR) {
            output->colorArray[4 * (output->vertexCount - iter - 1) + 0] = vertex->vertexColor->red;
            output->colorArray[4 * (output->vertexCount - iter - 1) + 1] = vertex->vertexColor->green;
            output->colorArray[4 * (output->vertexCount - iter - 1) + 2] = vertex->vertexColor->blue;
            output->colorArray[4 * (output->vertexCount - iter - 1) + 3] = vertex->vertexColor->alpha;
        }

        for (texCoordIter = 0; texCoordIter < output->tex_coord_sets; texCoordIter++) {
            for (texComponentIter = 0; texComponentIter < output->tex_coord_set_size; texComponentIter++) {
                output->texCoordArrays[texCoordIter][output->tex_coord_set_size * (output->vertexCount - iter - 1) + texComponentIter] =
                    vertex->tex_coords[texCoordIter][texComponentIter];
            }
        }

        free(vertex);
    }

    fseek(fp, size + startingPoint - ftell(fp), SEEK_CUR);

    freeStack(vertexStack);

    return output;
}

Blitz3DTRISChunk* readBlitz3DTRISChunk(FILE* fp) {
    Blitz3DTRISChunk* output;
    Stack* triangleStack;
    int size, startingPoint, iter;

    output = (Blitz3DTRISChunk*)calloc(1, sizeof(Blitz3DTRISChunk));
    triangleStack = createStack();

    read32BitIntegerFromBinaryFile(fp, &size, 1);

    printf("size = %d\n", size);
    startingPoint = ftell(fp);

    read32BitIntegerFromBinaryFile(fp, &(output->brush_id), 1);
    printf("brush_id = %d\n", output->brush_id);

    while (ftell(fp) < startingPoint + size) {
        Blitz3DTriangle* triangle = (Blitz3DTriangle*)malloc(sizeof(Blitz3DTriangle));

        read32BitIntegerFromBinaryFile(fp, &(triangle->vertex_id[0]), 1);
        read32BitIntegerFromBinaryFile(fp, &(triangle->vertex_id[1]), 1);
        read32BitIntegerFromBinaryFile(fp, &(triangle->vertex_id[2]), 1);

        pushOntoStack(triangleStack, (void*)triangle);
    }

    output->triangleCount = getStackCount(triangleStack);

    output->indexArray = (int*)malloc(output->triangleCount * 3 * sizeof(int));

    for (iter = 0; iter < output->triangleCount; iter++) {
        Blitz3DTriangle* triangle = (Blitz3DTriangle*)popOffOfStack(triangleStack);

        output->indexArray[3 * (output->triangleCount - iter - 1) + 0] = triangle->vertex_id[0];
        output->indexArray[3 * (output->triangleCount - iter - 1) + 1] = triangle->vertex_id[1];
        output->indexArray[3 * (output->triangleCount - iter - 1) + 2] = triangle->vertex_id[2];

        free(triangle);
    }

    fseek(fp, size + startingPoint - ftell(fp), SEEK_CUR);

    freeStack(triangleStack);

    return output;
}

Blitz3DMESHChunk* readBlitz3DMESHChunk(FILE* fp) {
    Blitz3DMESHChunk* output;
    Stack* trisStack;
    int size, startingPoint, iter, id;

    output = (Blitz3DMESHChunk*)malloc(sizeof(Blitz3DMESHChunk));
    trisStack = createStack();

    read32BitIntegerFromBinaryFile(fp, &size, 1);

    printf("size = %d\n", size);
    startingPoint = ftell(fp);

    read32BitIntegerFromBinaryFile(fp, &(output->brush_id), 1);
    printf("brush_id = %d\n", output->brush_id);

    read32BitIntegerFromBinaryFile(fp, &id, 1);

    if (id == BLITZ3D_TAG_VRTS_LITTLE_ENDIAN) {
        printf("VRTS chunk\n");

        output->vrtsChunk = readBlitz3DVRTSChunk(fp);
    }

    while (ftell(fp) < startingPoint + size) {
        read32BitIntegerFromBinaryFile(fp, &id, 1);

        if (id == BLITZ3D_TAG_TRIS_LITTLE_ENDIAN) {
            Blitz3DTRISChunk* tris;

            printf("TRIS chunk\n");

            tris = readBlitz3DTRISChunk(fp);
            pushOntoStack(trisStack, (void*)tris);
        }
    }

    output->trisChunkCount = getStackCount(trisStack);
    output->trisChunkArray = (Blitz3DTRISChunk**)malloc(output->trisChunkCount * sizeof(Blitz3DTRISChunk*));

    for (iter = 0; iter < output->trisChunkCount; iter++) {
        output->trisChunkArray[output->trisChunkCount - iter - 1] = (Blitz3DTRISChunk*)popOffOfStack(trisStack);
    }

    fseek(fp, size + startingPoint - ftell(fp), SEEK_CUR);

    freeStack(trisStack);

    return output;
}

Blitz3DNODEChunk* readBlitz3DNODEChunk(FILE* fp) {
    Blitz3DNODEChunk* output;
    Stack* nodeStack;
    int size, startingPoint, iter, id;

    output = (Blitz3DNODEChunk*)calloc(1, sizeof(Blitz3DNODEChunk));
    nodeStack = createStack();

    read32BitIntegerFromBinaryFile(fp, &size, 1);

    printf("size = %d\n", size);
    startingPoint = ftell(fp);

    output->name = readStringFromBinaryFile(fp);

    read32BitIntegerFromBinaryFile(fp, (int*)&(output->position[0]), 1);
    read32BitIntegerFromBinaryFile(fp, (int*)&(output->position[1]), 1);
    read32BitIntegerFromBinaryFile(fp, (int*)&(output->position[2]), 1);

    read32BitIntegerFromBinaryFile(fp, (int*)&(output->scale[0]), 1);
    read32BitIntegerFromBinaryFile(fp, (int*)&(output->scale[1]), 1);
    read32BitIntegerFromBinaryFile(fp, (int*)&(output->scale[2]), 1);

    read32BitIntegerFromBinaryFile(fp, (int*)&(output->rotation[0]), 1);
    read32BitIntegerFromBinaryFile(fp, (int*)&(output->rotation[1]), 1);
    read32BitIntegerFromBinaryFile(fp, (int*)&(output->rotation[2]), 1);
    read32BitIntegerFromBinaryFile(fp, (int*)&(output->rotation[3]), 1);

    while (ftell(fp) < startingPoint + size) {
        read32BitIntegerFromBinaryFile(fp, &id, 1);

        if (id == BLITZ3D_TAG_NODE_LITTLE_ENDIAN) {
            Blitz3DNODEChunk* node;

            printf("NODE chunk (child)\n");

            node = readBlitz3DNODEChunk(fp);
            pushOntoStack(nodeStack, (void*)node);
        }
        else if (id == BLITZ3D_TAG_MESH_LITTLE_ENDIAN) {
            printf("MESH chunk\n");

            output->meshChunk = readBlitz3DMESHChunk(fp);
        }
        else {
            skipBlitz3DChunk(fp);
        }
    }

    output->nodeChunkCount = getStackCount(nodeStack);
    output->nodeChunkArray = (Blitz3DNODEChunk**)malloc(output->nodeChunkCount * sizeof(Blitz3DNODEChunk*));

    for (iter = 0; iter < output->nodeChunkCount; iter++) {
        output->nodeChunkArray[output->nodeChunkCount - iter - 1] = (Blitz3DNODEChunk*)popOffOfStack(nodeStack);
    }

    fseek(fp, size + startingPoint - ftell(fp), SEEK_CUR);

    freeStack(nodeStack);

    return output;
}

Blitz3DBB3DChunk* readBlitz3DBB3DChunk(FILE* fp) {
    Blitz3DBB3DChunk* output;
    int id, size, version;

    read32BitIntegerFromBinaryFile(fp, &size, 1);
    read32BitIntegerFromBinaryFile(fp, &version, 1);

    printf("size = %d\n", size);
    printf("version = %d\n", version);

    output = (Blitz3DBB3DChunk*)calloc(1, sizeof(Blitz3DBB3DChunk));
    output->version = version;

    /* attempt to handle TEXS chunk */

    read32BitIntegerFromBinaryFile(fp, &id, 1);

    if (id == BLITZ3D_TAG_TEXS_LITTLE_ENDIAN) {
        printf("TEXS chunk\n");
        output->texsChunk = readBlitz3DTEXSChunk(fp);
    }

    /* attempt to handle BRUS chunk */

    read32BitIntegerFromBinaryFile(fp, &id, 1);

    if (id == BLITZ3D_TAG_BRUS_LITTLE_ENDIAN) {
        printf("BRUS chunk\n");
        output->brusChunk = readBlitz3DBRUSChunk(fp);
    }

    /* attempt to handle NODE chunk */

    read32BitIntegerFromBinaryFile(fp, &id, 1);

    if (id == BLITZ3D_TAG_NODE_LITTLE_ENDIAN) {
        printf("NODE chunk\n");
        output->nodeChunk = readBlitz3DNODEChunk(fp);
    }

    return output;
}

/* public functions */

B3DFile* loadB3DFile(const char* filePath) {
    /*Blitz3DBB3DChunk* fileData;*/
    B3DFile* output;
    char id[4];

    FILE* fp = fopen(filePath, "rb");
    if (fp == NULL) return NULL;

    id[0] = fgetc(fp);
    id[1] = fgetc(fp);
    id[2] = fgetc(fp);
    id[3] = fgetc(fp);

    if (id[0] != 'B' || id[1] != 'B' || id[2] != '3' || id[3] != 'D') {
        fprintf(stderr, "provided file, %s, is not Blitz3D format\n", filePath);
        return NULL;
    }

    output = (B3DFile*)malloc(sizeof(B3DFile));
    output->bb3dChunk = readBlitz3DBB3DChunk(fp);

    printf("\n---final results---\n");
    printf("file version = %d\n", output->bb3dChunk->version);

    printf("texture count = %d\n", output->bb3dChunk->texsChunk->textureCount);

    printf("first texture flags = %d\n", output->bb3dChunk->texsChunk->textureArray[0]->flags);
    printf("second texture flags = %d\n", output->bb3dChunk->texsChunk->textureArray[1]->flags);

    printf("first texture blend = %d\n", output->bb3dChunk->texsChunk->textureArray[0]->blend);
    printf("second texture blend = %d\n", output->bb3dChunk->texsChunk->textureArray[1]->blend);

    printf("first texture file = %s\n", output->bb3dChunk->texsChunk->textureArray[0]->file);
    printf("second texture file = %s\n", output->bb3dChunk->texsChunk->textureArray[1]->file);

    printf("brush count = %d\n", output->bb3dChunk->brusChunk->brushCount);
    printf("brush n_texs = %d\n", output->bb3dChunk->brusChunk->n_texs);

    printf("first brush name = %s\n", output->bb3dChunk->brusChunk->brushArray[0]->name);
    printf("second brush name = %s\n", output->bb3dChunk->brusChunk->brushArray[1]->name);
    printf("third brush name = %s\n", output->bb3dChunk->brusChunk->brushArray[2]->name);

    printf("first brush first texture = %d\n", output->bb3dChunk->brusChunk->brushArray[0]->texture_id[0]);
    printf("first brush second texture = %d\n", output->bb3dChunk->brusChunk->brushArray[0]->texture_id[1]);

    printf("second brush first texture = %d\n", output->bb3dChunk->brusChunk->brushArray[1]->texture_id[0]);
    printf("second brush second texture = %d\n", output->bb3dChunk->brusChunk->brushArray[1]->texture_id[1]);

    printf("third brush first texture = %d\n", output->bb3dChunk->brusChunk->brushArray[2]->texture_id[0]);
    printf("third brush second texture = %d\n", output->bb3dChunk->brusChunk->brushArray[2]->texture_id[1]);

    printf("rotation[0] of root = %g\n", output->bb3dChunk->nodeChunk->rotation[0]);
    printf("rotation[1] of root = %g\n", output->bb3dChunk->nodeChunk->rotation[1]);

    printf("chilren node count = %d\n", output->bb3dChunk->nodeChunk->nodeChunkCount);

    printf("rotation[0] of first child = %g\n", output->bb3dChunk->nodeChunk->nodeChunkArray[0]->rotation[0]);
    printf("rotation[1] of first child = %g\n", output->bb3dChunk->nodeChunk->nodeChunkArray[0]->rotation[1]);

    printf("first mesh TRIS chunk count = %d\n", output->bb3dChunk->nodeChunk->nodeChunkArray[0]->meshChunk->trisChunkCount);

    printf("triangles in first TRIS chunk = %d\n", output->bb3dChunk->nodeChunk->nodeChunkArray[0]->meshChunk->trisChunkArray[0]->triangleCount);

    printf("first vertex of first triangle = %d\n", output->bb3dChunk->nodeChunk->nodeChunkArray[0]->meshChunk->trisChunkArray[0]->indexArray[0]);
    printf("second vertex of first triangle = %d\n", output->bb3dChunk->nodeChunk->nodeChunkArray[0]->meshChunk->trisChunkArray[0]->indexArray[1]);
    printf("third vertex of first triangle = %d\n", output->bb3dChunk->nodeChunk->nodeChunkArray[0]->meshChunk->trisChunkArray[0]->indexArray[2]);

    printf("first vertex of second triangle = %d\n", output->bb3dChunk->nodeChunk->nodeChunkArray[0]->meshChunk->trisChunkArray[0]->indexArray[3]);
    printf("second vertex of second triangle = %d\n", output->bb3dChunk->nodeChunk->nodeChunkArray[0]->meshChunk->trisChunkArray[0]->indexArray[4]);
    printf("third vertex of second triangle = %d\n", output->bb3dChunk->nodeChunk->nodeChunkArray[0]->meshChunk->trisChunkArray[0]->indexArray[5]);

    printf("vertex count of first vrts chunk = %d\n", output->bb3dChunk->nodeChunk->nodeChunkArray[0]->meshChunk->vrtsChunk->vertexCount);

    printf("x of first vertex = %g\n", output->bb3dChunk->nodeChunk->nodeChunkArray[0]->meshChunk->vrtsChunk->vertexArray[0]);
    printf("y of first vertex = %g\n", output->bb3dChunk->nodeChunk->nodeChunkArray[0]->meshChunk->vrtsChunk->vertexArray[1]);
    printf("z of first vertex = %g\n", output->bb3dChunk->nodeChunk->nodeChunkArray[0]->meshChunk->vrtsChunk->vertexArray[2]);

    fclose(fp);

    return output;
}

Blitz3DBB3DChunk* getBB3DChunkFromFile(B3DFile* blitz3dFile) {
    return blitz3dFile->bb3dChunk;
}

Blitz3DNODEChunk* getNODEChunkFromBB3DChunk(Blitz3DBB3DChunk* bb3dChunk) {
    return bb3dChunk->nodeChunk;
}

Blitz3DMESHChunk* getMESHChunkFromNODEChunk(Blitz3DNODEChunk* nodeChunk) {
    return nodeChunk->meshChunk;
}

unsigned int getNODEChunkArrayCountFromNodeChunk(Blitz3DNODEChunk* nodeChunk) {
    return nodeChunk->nodeChunkCount;
}

Blitz3DNODEChunk* getNODEChunkArrayEntryFromNODEChunk(Blitz3DNODEChunk* nodeChunk, unsigned int index) {
    return nodeChunk->nodeChunkArray[index];
}

Blitz3DVRTSChunk* getVRTSChunkFromMESHChunk(Blitz3DMESHChunk* meshChunk) {
    return meshChunk->vrtsChunk;
}

unsigned int getTRISChunkArrayCountFromMESHChunk(Blitz3DMESHChunk* meshChunk) {
    return meshChunk->trisChunkCount;
}

Blitz3DTRISChunk* getTRISChunkArrayEntryFromMESHChunk(Blitz3DMESHChunk* meshChunk, unsigned int index) {
    return meshChunk->trisChunkArray[index];
}

float* getVertexArrayFromVRTSChunk(Blitz3DVRTSChunk* vrtsChunk) {
    return vrtsChunk->vertexArray;
}

float* getNormalArrayFromVRTSChunk(Blitz3DVRTSChunk* vrtsChunk) {
    return vrtsChunk->normalArray;
}

float* getColorArrayFromVRTSChunk(Blitz3DVRTSChunk* vrtsChunk) {
    return vrtsChunk->colorArray;
}

unsigned int getTexCoordArrayCountFromVRTSChunk(Blitz3DVRTSChunk* vrtsChunk) {
    return vrtsChunk->tex_coord_sets;
}

float* getTexCoordArrayEntryFromVRTSChunk(Blitz3DVRTSChunk* vrtsChunk, unsigned int index) {
    return vrtsChunk->texCoordArrays[index];
}

unsigned int getTexCoordArrayComponentCountFromVRTSChunk(Blitz3DVRTSChunk* vrtsChunk) {
    return vrtsChunk->tex_coord_set_size;
}

int normalArrayPresentInVRTSChunk(Blitz3DVRTSChunk* vrtsChunk) {
    return ( (vrtsChunk->flags & BLITZ3D_VERTEX_FLAG_NORMAL) != 0 );
}

int colorArrayPresentInVRTSChunk(Blitz3DVRTSChunk* vrtsChunk) {
    return ( (vrtsChunk->flags & BLITZ3D_VERTEX_FLAG_COLOR) != 0 );
}

unsigned getTriangleCountFromTRISChunk(Blitz3DTRISChunk* trisChunk) {
    return trisChunk->triangleCount;
}

int* getTriangleIndexArrayFromTRISChunk(Blitz3DTRISChunk* trisChunk) {
    return trisChunk->indexArray;
}
