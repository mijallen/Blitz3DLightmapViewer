#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/glu.h>

#include <png.h>

#include "Blitz3DFile.h"

/* program global variables */

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

SDL_Window* glWindow = NULL;
SDL_GLContext glContext;
SDL_Event event;
const Uint8* keyPress;
int quit = 0;

float angle = 0.f;

B3DFile* b3dTest;
int* textures;

void (APIENTRY * glActiveTextureARB)(unsigned int) = NULL;
void (APIENTRY * glClientActiveTextureARB)(unsigned int) = NULL;

/* image structure */

typedef struct Image Image;
struct Image {
    int width, height;
    png_byte* data;
    png_byte colorType;
};

void error(const char* message) {
    fprintf(stderr, "ERROR: %s\n", message);
    exit(1);
}

/*
    idea:
      struct BB3D contains arrays of structs TEXS, BRUS, and NODE
      struct NODE contains optional array of struct NODE
      struct NODE also contains optional struct MESH
      struct MESH contains arrays of structs VRTS and TRIS
      loadB3DFile checks for BB3D tag then calls readBB3DChunk
      readBB3DChunk checks for TEXS, BRUS, NODE, calls readTEXSChunk, ...
      readTEXSChunk reads textures in a loop and places them on a stack

    getting arrays:
      make general purpose (void*) stack structure (push, pop, count)
      push new structs onto stack as found from file
      once finished pushing, allocate based on count
      pop elements off stack into back of array (reverse the stack)

    drawing layout:
      VTRS chunk contains vertexArray, normalArray, colorArray, etc.
      optional arrays can be set to NULL if not present
      TRIS chunk contains indexArray for vertex_id of all triangles
      drawB3D will call drawNode and drawMesh
      drawMesh will refer to VRTS chunk and TRIS chunks for drawing
      drawNode will create transform matrix and push onto stack

    remaining tasks:
      reorganize code, put on github
      get working directory of b3d file
      load textures mentioned in TEXS chunk
      use brushes to guide mesh drawing
      fix texture blending (maybe)
      flashlight?
*/

/* PNG loading function */
/* thank you to http://zarb.org/~gc/html/libpng.html */

Image* loadPNGImage(const char* filePath) {
    Image* output = NULL;

    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;
    png_byte** row_pointers;

    png_byte bitDepth;
    int iter;

    char header[8];

    FILE* fp = fopen(filePath, "rb");
    if (!fp) return NULL;

    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8) != 0) return NULL;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) return NULL;

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) return NULL;

    if (setjmp(png_jmpbuf(png_ptr))) return NULL;

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    bitDepth = png_get_bit_depth(png_ptr, info_ptr);
    if (bitDepth == 16) return NULL;

    output = (Image*)malloc(sizeof(Image));

    output->width = png_get_image_width(png_ptr, info_ptr);
    output->height = png_get_image_height(png_ptr, info_ptr);
    output->colorType = png_get_color_type(png_ptr, info_ptr);

    if (output->colorType == PNG_COLOR_TYPE_RGB)
        output->data = (png_byte*)malloc(output->width * output->height * 3);
    else if (output->colorType == PNG_COLOR_TYPE_RGBA)
        output->data = (png_byte*)malloc(output->width * output->height * 4);
    else
        return NULL;

    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    if (setjmp(png_jmpbuf(png_ptr))) return NULL;

    row_pointers = (png_byte**)malloc(sizeof(png_byte*) * output->height);

    if (output->colorType == PNG_COLOR_TYPE_RGB)
        for (iter = 0; iter < output->height; iter++)
            row_pointers[iter] = output->data + iter * output->width * 3;
    else if (output->colorType == PNG_COLOR_TYPE_RGBA)
        for (iter = 0; iter < output->height; iter++)
            row_pointers[iter] = output->data + iter * output->width * 4;
    else
        return NULL;

    png_read_image(png_ptr, row_pointers);

    fclose(fp);

    return output;
}

/* OpenGL draw code for Blitz3D level */

void drawMesh(Blitz3DMESHChunk* mesh) {
    unsigned int iter;
    unsigned int texCoordCount;
    unsigned int texCoordComponentCount;
    int meshBrushId;
    Blitz3DVRTSChunk* vrtsChunk;
    Blitz3DBRUSChunk* brusChunk;

    meshBrushId = getBrushIdFromMESHChunk(mesh);

    brusChunk = getBRUSChunkFromBB3DChunk( getBB3DChunkFromFile(b3dTest) );
    vrtsChunk = getVRTSChunkFromMESHChunk(mesh);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, getVertexArrayFromVRTSChunk(vrtsChunk));

    if (normalArrayPresentInVRTSChunk(vrtsChunk)) {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, getNormalArrayFromVRTSChunk(vrtsChunk));
    }

    if (colorArrayPresentInVRTSChunk(vrtsChunk)) {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_FLOAT, 0, getColorArrayFromVRTSChunk(vrtsChunk));
    }

    texCoordCount = getTexCoordArrayCountFromVRTSChunk(vrtsChunk);
    texCoordComponentCount = getTexCoordArrayComponentCountFromVRTSChunk(vrtsChunk);

    glClientActiveTextureARB(GL_TEXTURE0_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(texCoordComponentCount, GL_FLOAT, 0, getTexCoordArrayEntryFromVRTSChunk(vrtsChunk, 0));

    glClientActiveTextureARB(GL_TEXTURE1_ARB);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer(texCoordComponentCount, GL_FLOAT, 0, getTexCoordArrayEntryFromVRTSChunk(vrtsChunk, 1));

    for (iter = 0; iter < getTRISChunkArrayCountFromMESHChunk(mesh); iter++) {
        Blitz3DTRISChunk* trisChunk;
        Blitz3DBrush* brush;
        int trisBrushId;

        trisChunk = getTRISChunkArrayEntryFromMESHChunk(mesh, iter);

        trisBrushId = getBrushIdFromTRISChunk(trisChunk);
        if (trisBrushId == -1) trisBrushId = meshBrushId;

        brush = getBrushArrayEntryFromBRUSChunk(brusChunk, trisBrushId);

        glActiveTextureARB(GL_TEXTURE0_ARB);
        glBindTexture(GL_TEXTURE_2D, textures[getTextureIdArrayEntryFromBrush(brush, 1)]);

        glActiveTextureARB(GL_TEXTURE1_ARB);
        glBindTexture(GL_TEXTURE_2D, textures[getTextureIdArrayEntryFromBrush(brush, 0)]);

        glDrawElements(GL_TRIANGLES, 3 * getTriangleCountFromTRISChunk(trisChunk),
            GL_UNSIGNED_INT, getTriangleIndexArrayFromTRISChunk(trisChunk));
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glClientActiveTextureARB(GL_TEXTURE0_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);

    glClientActiveTextureARB(GL_TEXTURE1_ARB);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}

void drawNode(Blitz3DNODEChunk* node) {
    unsigned int iter;

    if (getMESHChunkFromNODEChunk(node) != NULL) {
        drawMesh( getMESHChunkFromNODEChunk(node) );
    }

    for (iter = 0; iter < getNODEChunkArrayCountFromNodeChunk(node); iter++) {
        drawNode( getNODEChunkArrayEntryFromNODEChunk(node, iter) );
    }
}

void drawB3D(B3DFile* b3d) {
    glMatrixMode(GL_PROJECTION);
    gluPerspective(70.0, 1.333, 1, 1000);

    glMatrixMode(GL_MODELVIEW);
    glTranslatef(0.f, -50.f, -100.f);
    glRotatef(angle, 0.f, 1.f, 0.f); angle += 1.f;
    glScalef(1.f, 1.f, -1.f);

    drawNode( getNODEChunkFromBB3DChunk( getBB3DChunkFromFile(b3d) ) );

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

int loadPNGTexture(char* filePath) {
    int texture;
    Image* pngImage = loadPNGImage(filePath);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    if (pngImage->colorType == PNG_COLOR_TYPE_RGB)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pngImage->width, pngImage->height, 0, GL_RGB, GL_UNSIGNED_BYTE, pngImage->data);
    else if (pngImage->colorType == PNG_COLOR_TYPE_RGBA)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pngImage->width, pngImage->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pngImage->data);

    free(pngImage->data);
    free(pngImage);

    return texture;
}

int loadBMPTexture(char* filePath) {
    int texture;
    SDL_Surface* bmpImage = SDL_LoadBMP(filePath);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, bmpImage->w, bmpImage->h, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, bmpImage->pixels);

    SDL_FreeSurface(bmpImage);

    return texture;
}

int* loadTextures(B3DFile* b3d) {
    int* output;
    Blitz3DTEXSChunk* texsChunk;
    unsigned int textureCount;
    unsigned int iter;

    texsChunk = getTEXSChunkFromBB3DChunk(getBB3DChunkFromFile(b3d));
    textureCount = getTextureArrayCountFromTEXSChunk(texsChunk);

    output = (int*)calloc(textureCount, sizeof(int));
    glEnable(GL_TEXTURE_2D);

    for (iter = 0; iter < textureCount; iter++) {
        char* directoryPath;
        char* fileName;
        char* filePath;

        directoryPath = getDirectoryFromFile(b3d);
        fileName = getFileFromTexture(getTextureArrayEntryFromTEXSChunk(texsChunk, iter));

        filePath = (char*)calloc(strlen(directoryPath) + strlen(fileName) + 1, sizeof(char));
        sprintf(filePath, "%s%s", directoryPath, fileName);

        printf("full path: %s\n", filePath);

        if (filePath[strlen(filePath) - 1] == 'g') {
            printf(" is a PNG image\n");
            output[iter] = loadPNGTexture(filePath);
            printf(" texture id: %d\n", output[iter]);
        }

        if (filePath[strlen(filePath) - 1] == 'p') {
            printf(" is a BMP image\n");
            output[iter] = loadBMPTexture(filePath);
            printf(" texture id: %d\n", output[iter]);
        }

        free(filePath);
    }

    return output;
}

/* actual program */

int main(int argc, char* argv[]) {
    b3dTest = loadB3DFile("test1/test1.b3d");

    printf("textures:\n");
    printf("directory: %s\n", getDirectoryFromFile(b3dTest));
    printf("texture count: %d\n", getNumberOfTexturesFromBRUSChunk(getBRUSChunkFromBB3DChunk(getBB3DChunkFromFile(b3dTest))));

    SDL_Init(SDL_INIT_VIDEO);

    glWindow = SDL_CreateWindow("B3D Lightmap Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);

    glContext = SDL_GL_CreateContext(glWindow);

    glActiveTextureARB = SDL_GL_GetProcAddress("glActiveTextureARB");
    glClientActiveTextureARB = SDL_GL_GetProcAddress("glClientActiveTextureARB");

    keyPress = SDL_GetKeyboardState(NULL);

    glClearColor(0.f, 0.f, 0.5f, 1.f);

    /* multitexture setup */

    textures = loadTextures(b3dTest);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glActiveTextureARB(GL_TEXTURE0_ARB);
    glEnable(GL_TEXTURE_2D);

    glActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_2D);

    /* main loop */

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) quit = 1;
        }

        if (keyPress[SDL_SCANCODE_ESCAPE]) quit = 1;

        glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        drawB3D(b3dTest);

        SDL_GL_SwapWindow(glWindow);
        SDL_Delay(16);
    }

    SDL_DestroyWindow(glWindow);
    SDL_Quit();

    return 0;
}
