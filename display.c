#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>
#include <SDL_opengl.h>
#include <GL/glu.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <png.h>

#include "Blitz3DFile.h"

/* program global variables */

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

#define MOUSE_HORIZONTAL_SENSITIVITY 0.5
#define MOUSE_VERTICAL_SENSITIVITY 0.5
#define CAMERA_SPEED 20.0

SDL_Window* glWindow = NULL;
SDL_GLContext glContext;
SDL_Event event;
const Uint8* keyPress;
int quit = 0;

B3DFile* b3dTest;
int* textures;

void (APIENTRY * glActiveTextureARB)(unsigned int) = NULL;
void (APIENTRY * glClientActiveTextureARB)(unsigned int) = NULL;

/* image structure */

/* commentary: consider moving this to its own file (along with png reading code) */

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

/* commentary: old comments below, mostly here for reference */

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

        /* commentary: not sure why the brush textures seem reversed here! */

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

    /* commentary: still need to apply matrix transforms per node */

    if (getMESHChunkFromNODEChunk(node) != NULL) {
        drawMesh( getMESHChunkFromNODEChunk(node) );
    }

    for (iter = 0; iter < getNODEChunkArrayCountFromNodeChunk(node); iter++) {
        drawNode( getNODEChunkArrayEntryFromNODEChunk(node, iter) );
    }
}

void drawB3D(B3DFile* b3d) {
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

        /*printf("full path: %s\n", filePath);*/

        if (filePath[strlen(filePath) - 1] == 'g') {
            /*printf(" is a PNG image\n");*/
            output[iter] = loadPNGTexture(filePath);
            /*printf(" texture id: %d\n", output[iter]);*/
        }

        if (filePath[strlen(filePath) - 1] == 'p') {
            /*printf(" is a BMP image\n");*/
            output[iter] = loadBMPTexture(filePath);
            /*printf(" texture id: %d\n", output[iter]);*/
        }

        free(filePath);
    }

    return output;
}

/* actual program */

int main(int argc, char* argv[]) {
    char* b3dFilePath;

    /* camera variables */
    float angleX = 0.f, angleY = 0.f;
    float positionX = 0.f, positionY = 0.f, positionZ = 0.f;

    /* camera movement variables */
    float forwardX = 0.f, forwardY = 0.f, forwardZ = 1.f;
    float rightX = 1.f, rightY = 0.f, rightZ = 0.f;

    /* memory to store only the rotation transform of the view matrix */
    float viewRotation[16];

    if (argc < 2) b3dFilePath = "test1/test1.b3d";
    else b3dFilePath = argv[1];

    b3dTest = loadB3DFile(b3dFilePath);
/*
    printf("textures:\n");
    printf("directory: %s\n", getDirectoryFromFile(b3dTest));
    printf("texture count: %d\n", getNumberOfTexturesFromBRUSChunk(getBRUSChunkFromBB3DChunk(getBB3DChunkFromFile(b3dTest))));
*/
    SDL_Init(SDL_INIT_VIDEO);

    glWindow = SDL_CreateWindow("B3D Lightmap Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);

    glContext = SDL_GL_CreateContext(glWindow);

    glActiveTextureARB = SDL_GL_GetProcAddress("glActiveTextureARB");
    glClientActiveTextureARB = SDL_GL_GetProcAddress("glClientActiveTextureARB");

    keyPress = SDL_GetKeyboardState(NULL);

    /* multitexture setup */

    /* commentary: possible support for more than 2 texture layers? if necessary. */

    textures = loadTextures(b3dTest);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glActiveTextureARB(GL_TEXTURE0_ARB);
    glEnable(GL_TEXTURE_2D);

    glActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_2D);

    /* OpenGL initial setup */

    glClearColor(0.f, 0.f, 0.5f, 1.f);

    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    gluPerspective(70.0, 1.333, 10, 10000);

    glMatrixMode(GL_MODELVIEW);

    /* main loop */

    while (!quit) {
        int differentialX = 0, differentialY = 0;
        int mouseStates = 0;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) quit = 1;
        }

        if (keyPress[SDL_SCANCODE_ESCAPE]) quit = 1;

        mouseStates = SDL_GetRelativeMouseState(&differentialX, &differentialY);

        /* change camera orientation when mouse dragged */
        if (mouseStates & SDL_BUTTON(SDL_BUTTON_LEFT)) {
            angleX -= MOUSE_VERTICAL_SENSITIVITY * differentialY;
            angleY -= MOUSE_HORIZONTAL_SENSITIVITY * differentialX;
        }

        /* use existing base vectors in view rotation matrix for movement */
        forwardX = viewRotation[2];
        forwardY = viewRotation[6];
        forwardZ = viewRotation[10];

        rightX = viewRotation[0];
        rightY = viewRotation[4];
        rightZ = viewRotation[8];

        /* move camera forward when W pressed */
        if (keyPress[SDL_SCANCODE_W]) {
            positionX -= CAMERA_SPEED * forwardX;
            positionY -= CAMERA_SPEED * forwardY;
            positionZ -= CAMERA_SPEED * forwardZ;
        }

        /* move camera left when A pressed */
        if (keyPress[SDL_SCANCODE_A]) {
            positionX -= CAMERA_SPEED * rightX;
            positionY -= CAMERA_SPEED * rightY;
            positionZ -= CAMERA_SPEED * rightZ;
        }

        /* move camera backward when S pressed */
        if (keyPress[SDL_SCANCODE_S]) {
            positionX += CAMERA_SPEED * forwardX;
            positionY += CAMERA_SPEED * forwardY;
            positionZ += CAMERA_SPEED * forwardZ;
        }

        /* move camera right when D pressed */
        if (keyPress[SDL_SCANCODE_D]) {
            positionX += CAMERA_SPEED * rightX;
            positionY += CAMERA_SPEED * rightY;
            positionZ += CAMERA_SPEED * rightZ;
        }

        /* reset camera position and orientation when R pressed */
        if (keyPress[SDL_SCANCODE_R]) {
            positionX = 0.f;
            positionY = 0.f;
            positionZ = 0.f;
            angleX = 0.f;
            angleY = 0.f;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* construct the view matrix */
        glLoadIdentity();
        glRotatef(-angleX, 1.f, 0.f, 0.f);
        glRotatef(-angleY, 0.f, 1.f, 0.f);
        glGetFloatv(GL_MODELVIEW_MATRIX, viewRotation);
        glTranslatef(-positionX, -positionY, -positionZ);
        glScalef(1.f, 1.f, -1.f);

        drawB3D(b3dTest);

        SDL_GL_SwapWindow(glWindow);
        SDL_Delay(16);
    }

    /* commentary: still need to clear the loaded data from memory */

    SDL_DestroyWindow(glWindow);
    SDL_Quit();

    return 0;
}
