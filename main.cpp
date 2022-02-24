/*Header Inclusions*/
#include <iostream>
#include <GL/glew.h>
#include <GLUT/freeglut.h>

//GLM Math Header Inclusions
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//SOIL image loader Inclusion
#include "SOIL2/SOIL2.h"
using namespace std; //Standard namespace
#define WINDOW_TITLE "Shelf - Mihir Patel" //Window title Macro

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif

/*Variable declarations for shader, window size initialization, buffer and array
objects */
GLint shelfShaderProgram, lampShaderProgram, WindowWidth = 800, WindowHeight = 600;
GLuint VBO, ShelfVAO, LightVAO, texture;

//Subject position and scale
glm::vec3 shelfPosition(0.0f, 0.0f, 0.0f);
glm::vec3 shelfScale(1.0f);

//shelf and light color
glm::vec3 objectColor(1.0f, 1.0f, 1.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f); //key light color
glm::vec3 fillLightColor(0.0f, 0.0f, 0.0f); //  fill light color (shadows)

//Key Light position and scale
glm::vec3 lightPosition(-15.0f, 8.0f, 15.0f);
glm::vec3 lightScale(0.3f);

//Fill Light position and scale
glm::vec3 fillLightPosition(5.0f, 5.0f, -5.0f);
glm::vec3 fillLightScale(0.3f);

//Camera position
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, -6.0f); // initial camera position -3.0 units in x

//Camera rotation
float cameraRotation = glm::radians(-25.0f);
GLfloat cameraSpeed = 0.05f; // movement speed per frame
GLchar currentKey; //will store key pressed

GLfloat lastMouseX = 400, lastMouseY = 300; // Locks mouse cursor at center of screen
GLfloat mouseXOffset, mouseYOffset, yaw = 0.0f, pitch = 0.0f; // mouse offset, yaw, and pitch variables
GLfloat sensitivity = 0.5f; // used for mouse/camera rotation sensitivity
bool mouseDetected = true; // initially true when mouse movement is detected
bool mouseLeft = true;
bool mouseRight = true;
bool altKey = true;

//Global vector declarations
glm::vec3 CameraUpY = glm::vec3(0.0f, 1.0f, 0.0f); // temporary y unit vector
glm::vec3 CameraForwardZ = glm::vec3(0.0f, 0.0f, -1.0f); // temporary z unit vector

/*Function prototypes*/
void UResizeWindow(int, int);
void URenderGraphics(void);
void UCreateShader(void);
void UCreateBuffers(void);
void UGenerateTexture(void);
void UMouseClick(int button, int state, int x, int y);
void UMousePressedMove(int x, int y);
void UKeyboard(unsigned char key, int x, int y);

/*shelf Vertex Shader Source Code*/
const GLchar* shelfVertexShaderSource = GLSL(330,
    layout(location = 0) in vec3 position; //Vertex data from Vertex Attrib Pointer 0
layout(location = 1) in vec3 normal; //VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;
out vec3 FragmentPos; //For outgoing color / pixels to fragment shader
out vec3 Normal; //For outgoing normals to fragment shader
out vec2 mobileTextureCoordinate;
//Global variables for the transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main() {
    gl_Position = projection * view * model * vec4(position, 1.0f);

    //transforms vertices to clip coordinates
    FragmentPos = vec3(model * vec4(position, 1.0f)); //Gets fragment / pixel position in world space only (exclude view and projection)
    Normal = mat3(transpose(inverse(model))) * normal; //get normal vectors in world space only and exclude normal translation properties
    mobileTextureCoordinate = vec2(textureCoordinate.x, 1 - textureCoordinate.y); //flips the texture horizontal
}
);

/*shelf Fragment Shader Source Code*/
const GLchar* shelfFragmentShaderSource = GLSL(330,
    in vec3 FragmentPos; //For incoming fragment position
in vec3 Normal; //For incoming normals
in vec2 mobileTextureCoordinate;
out vec4 shelfColor; //For outgoing chair color to the GPU

//Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec3 lightColor; //key light color
uniform vec3 lightPos; //key light position
uniform vec3 fillLightColor; //fill light color
uniform vec3 fillLightPos; //fill light position
uniform vec3 viewPosition; //Key light view
uniform sampler2D uTexture; //Useful when working with multiple textures

void main() {
    /*Phong lighting model calculations to generate ambient, diffuse, and specular components*/
    vec3 norm = normalize(Normal); //Normalize vectors to 1 unit
    vec3 viewDir = normalize(viewPosition - FragmentPos); //Calculate view direction

    /*Key Lighting*/
    //Calculate Ambient Lighting
    float ambientStrength = 0.5f; //Set ambient or global lighting
    vec3 lightDirection = normalize(lightPos - FragmentPos); //Calculate distance (light direction) between light source and fragments/pixels on
    float impact = max(dot(norm, lightDirection), 0.0); //Calculate diffuse impact by generating dot product of normal and light
    vec3 diffuse = impact * lightColor; //Generate diffuse light color

    //Calculate Specular lighting
    float specularIntensity = 1.0f; //Set specular light strength
    float highlightSize = 10.0f; //Set specular highlight size
    vec3 reflectDir = reflect(-lightDirection, norm); //Calculate reflection vector

    //Calculate specular component
    float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 specular = specularIntensity * specularComponent * lightColor;

    /*Fill Lighting*/
    //Calculate Ambient Lighting
    float fillAmbientStrength = 0.5f; //Set ambient or global lighting strength

    //Generate ambient light color
    vec3 fillAmbient = fillAmbientStrength * fillLightColor;

    //Calculate Diffuse Lighting
    vec3 fillLightDirection = normalize(fillLightPos - FragmentPos); //Calculate distance (light direction) between light source and fragments/pixels on
    float fillImpact = max(dot(norm, fillLightDirection), 0.0); //Calculate diffuse impact by generating dot product of normal and light
    vec3 fillDiffuse = fillImpact * fillLightColor; //Generate diffuse light color

    //Calculate Specular lighting
    float fillSpecularIntensity = 0.1f; //Set specular light
    float fillHighlightSize = 5.0f; //Set specular highlight size
    vec3 fillReflectDir = reflect(-fillLightDirection, norm);

    //Calculate reflection vector
    //Calculate specular component
    float fillSpecularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
    vec3 fillSpecular = fillSpecularIntensity * fillSpecularComponent * fillLightColor;

    //Calculate phong result
    vec3 objectColor = texture(uTexture, mobileTextureCoordinate).xyz;
    vec3 keyLightResult = (ambient + diffuse + specular);
    vec3 fillLightResult = (fillAmbient + fillDiffuse + fillSpecular);
    vec3 phong = (keyLightResult + fillLightResult) * objectColor;
    shelfColor = vec4(phong, 1.0f); //Send lighting results to GPU
}
);

/*Lamp Shader Source Code*/
const GLchar* lampVertexShaderSource = GLSL(330,
    layout(location = 0) in vec3 position; //VAP position 0 for vertex position data
    //Uniform / Global variables for the transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0f);
    //Transforms vertices into clip coordinates
}
);

/*Fragment Shader Source Code*/
const GLchar* lampFragmentShaderSource = GLSL(330,
    out vec4 color; //For outgoing lamp color
void main()
{
    color = vec4(1.0f); //Set color to white (1.0f, 1.0f, 1.0f) with alpha 1.0
}
);

/*Main Program*/
int main(int argc, char* argv[]) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(WindowWidth, WindowHeight);
    glutCreateWindow(WINDOW_TITLE);
    glutReshapeFunc(UResizeWindow);
    glewExperimental = GL_TRUE;

    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    UCreateShader();
    UCreateBuffers();
    UGenerateTexture();
    glutMouseFunc(UMouseClick); // Detects mouse click
    glutMotionFunc(UMousePressedMove); // Detects mouse press and movement
    glClearColor(0.6f, 0.5f, 0.3f, 1.0f); //Set background color
    glutDisplayFunc(URenderGraphics);
    glutKeyboardFunc(UKeyboard); // Detects key press
    glutMainLoop();

    //Destroys Buffer objects once used
    glDeleteVertexArrays(1, &ShelfVAO);
    glDeleteVertexArrays(1, &LightVAO);
    glDeleteBuffers(1, &VBO);
    return 0;
}

/*Resizes the window*/
void UResizeWindow(int w, int h)
{
    WindowWidth = w;
    WindowHeight = h;
    glViewport(0, 0, WindowWidth, WindowHeight);
}
/*Renders graphics*/
void URenderGraphics(void)
{
    glEnable(GL_DEPTH_TEST); //Enable z-depth
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //Clears the screen
    GLint modelLoc, viewLoc, projLoc, uTextureLoc, lightColorLoc, lightPositionLoc, viewPositionLoc, fillLightColorLoc, fillLightPositionLoc;
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;

    /*********Use the shelf Shader to activate the shelf Vertex Array Object for
    rendering and transforming*********/
    glUseProgram(shelfShaderProgram);
    glBindVertexArray(ShelfVAO);

    //Camera movement logic
    CameraForwardZ = cameraPosition; //replaces camera forward vector with Radians normalized as a unit vector

    //Transform the shelf
    model = glm::translate(model, shelfPosition);
    model = glm::scale(model, shelfScale);

    //Transform the camera
    view = glm::translate(view, cameraPosition);
    view = glm::rotate(view, cameraRotation, glm::vec3(0.0f, 1.0f, 0.0f));
    view = glm::lookAt(cameraPosition, glm::vec3(0.0f, 0.0f, 0.0f), CameraUpY);

    //Set the camera projection to orthographic (2D)
    if (currentKey != '3') {
        projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
    }

    //Set the camera projection to perspective (3D)
    if (currentKey != '2') {
        projection = glm::perspective(45.0f, (GLfloat)WindowWidth / (GLfloat)WindowHeight, 0.1f, 100.0f);
    }

    //Reference matrix uniforms from the shelf Shader program
    modelLoc = glGetUniformLocation(shelfShaderProgram, "model");
    viewLoc = glGetUniformLocation(shelfShaderProgram, "view");
    projLoc = glGetUniformLocation(shelfShaderProgram, "projection");

    //Pass matrix data to the shelf Shader program's matrix uniforms
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    //Reference matrix uniforms from the shelf Shader program for the shelf color, light color, light position, and camera position
    uTextureLoc = glGetUniformLocation(shelfShaderProgram, "uTexture");
    lightColorLoc = glGetUniformLocation(shelfShaderProgram, "lightColor");

    //key light color
    lightPositionLoc = glGetUniformLocation(shelfShaderProgram, "lightPos");

    //key light position
    fillLightColorLoc = glGetUniformLocation(shelfShaderProgram, "fillLightColor"); //fill light color
    fillLightPositionLoc = glGetUniformLocation(shelfShaderProgram, "fillLightPos"); //fill light position
    viewPositionLoc = glGetUniformLocation(shelfShaderProgram, "viewPosition");

    //Pass color, light, and camera data to the shelf Shader programs corresponding uniforms
    glUniform1i(uTextureLoc, 0); // texture unit 0
    glUniform3f(lightColorLoc, lightColor.r, lightColor.g, lightColor.b); //key light color
    glUniform3f(lightPositionLoc, lightPosition.x, lightPosition.y, lightPosition.z); //key light position
    glUniform3f(fillLightColorLoc, fillLightColor.r, fillLightColor.g, fillLightColor.b); //fill light color
    glUniform3f(fillLightPositionLoc, fillLightPosition.x, fillLightPosition.y, fillLightPosition.z); //fill light position
    glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);
    glDrawArrays(GL_TRIANGLES, 0, 72); //Draw the primitives / shelf
    glBindVertexArray(0); //Deactivate the shelf Vertex Array Object
/***************Use the Lamp Shader and activate the Lamp Vertex Array
Object for rendering and transforming ************/
    glUseProgram(lampShaderProgram);
    glBindVertexArray(LightVAO);
    //Transform the smaller shelf used as a visual cue for the light source
    model = glm::translate(model, lightPosition);
    model = glm::scale(model, lightScale);
    //Reference matrix uniforms from the Lamp Shader program
    modelLoc = glGetUniformLocation(lampShaderProgram, "model");
    viewLoc = glGetUniformLocation(lampShaderProgram, "view");
    projLoc = glGetUniformLocation(lampShaderProgram, "projection");
    //Pass matrix uniforms from the Lamp Shader Program
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glDrawArrays(GL_TRIANGLES, 0, 72);//Draws the triangles
    glBindVertexArray(0); //Deactivate the Lamp Vertex Array Object
    glutPostRedisplay();
    glutSwapBuffers(); //Flips the back buffer with the front buffer every frame. Similar to GL Flush
}

/*Create the Shader program*/
void UCreateShader()
{
    //shelf Vertex shader
    GLint shelfVertexShader = glCreateShader(GL_VERTEX_SHADER); //Creates the Vertex shader
    glShaderSource(shelfVertexShader, 1, &shelfVertexShaderSource, NULL);

    //Attaches the Vertex shader to the source code
    glCompileShader(shelfVertexShader); //Compiles the Vertex shader

    //shelf Fragment Shader
    GLint shelfFragmentShader = glCreateShader(GL_FRAGMENT_SHADER); //Creates the Fragment Shader
    glShaderSource(shelfFragmentShader, 1, &shelfFragmentShaderSource, NULL); //Attaches the Fragment shader to the source code
    glCompileShader(shelfFragmentShader); //Compiles the Fragment Shader

    //shelf Shader program
    shelfShaderProgram = glCreateProgram(); //Creates the Shader program and returns an id
    glAttachShader(shelfShaderProgram, shelfVertexShader); //Attaches Vertex shader to the Shader program
    glAttachShader(shelfShaderProgram, shelfFragmentShader); //Attaches Fragment shader to the Shader program
    glLinkProgram(shelfShaderProgram); //Link Vertex and Fragment shaders to the Shader program

    //Delete the Vertex and Fragment shaders once linked
    glDeleteShader(shelfVertexShader);
    glDeleteShader(shelfFragmentShader);

    //Lamp Vertex shader
    GLint lampVertexShader = glCreateShader(GL_VERTEX_SHADER); //Creates the Vertex shader
    glShaderSource(lampVertexShader, 1, &lampVertexShaderSource, NULL);

    //Attaches the Vertex shader to the source code
    glCompileShader(lampVertexShader); //Compiles the Vertex shader

    //Lamp Fragment shader
    GLint lampFragmentShader = glCreateShader(GL_FRAGMENT_SHADER); //Creates the Fragment shader
    glShaderSource(lampFragmentShader, 1, &lampFragmentShaderSource, NULL);

    //Attaches the Fragment shader to the source code
    glCompileShader(lampFragmentShader); //Compiles the Fragment shader

    //Lamp Shader Program
    lampShaderProgram = glCreateProgram(); //Creates the Shader program and returns an id
    glAttachShader(lampShaderProgram, lampVertexShader); //Attach Vertex shader to the Shader program
    glAttachShader(lampShaderProgram, lampFragmentShader); //Attach Fragment shader to the Shader program
    glLinkProgram(lampShaderProgram); //Link Vertex and Fragment shaders to the Shader program

    //Delete the lamp shaders once linked
    glDeleteShader(lampVertexShader);
    glDeleteShader(lampFragmentShader);
}

/*Creates the Buffer and Array Objects*/
void UCreateBuffers()
{
    //Position and Texture coordinate data for 18 triangles
    GLfloat vertices[] = {

        //X   //Y   //Z           //Normals                  //Texture Coordinates  
        //Bottom BookShelf
        0.0f, 0.5f, -0.25f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f, // BookShelf Back Top Left Corner Vertex 0
        0.0f, 0.5f,  0.25f,   -1.0f,  0.0f,  0.0f,    0.0f, 1.0f, // BookShelf Front Top Left Corner Vertex 1
        0.0f, 0.0f, -0.25f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f, // BookShelf Back Bottom Left Corner Vertex 2
        0.0f, 0.0f, -0.25f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f, // BookShelf Back Bottom Left Corner Vertex 3
        0.0f, 0.0f,  0.25f,   -1.0f,  0.0f,  0.0f,    1.0f, 0.0f, // BookShelf Front Bottom Left Corner Vertex 4
        0.0f, 0.5f,  0.25f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f, // BookShelf Front Top Left Corner Vertex 5
        1.0f, 0.5f, -0.25f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f, // BookShelf Back Top Right Corner Vertex 6
        1.0f, 0.5f,  0.25f,    1.0f,  0.0f,  0.0f,    0.0f, 1.0f, // BookShelf Front Top Right Corner Vertex 7
        1.0f, 0.0f, -0.25f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f, // BookShelf Back Bottom Right Corner Vertex 8
        1.0f, 0.0f, -0.25f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f, // BookShelf Back Bottom Right Corner Vertex 9
        1.0f, 0.0f,  0.25f,    1.0f,  0.0f,  0.0f,    1.0f, 0.0f, // BookShelf Front Bottom Right Corner Vertex 10
        1.0f, 0.5f,  0.25f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f, // BookShelf Front Top Right Corner Vertex 11
        0.0f, 0.5f, -0.25f,    0.0f,  1.0f,  0.0f,    0.0f, 1.0f, // BookShelf Top Back Left Corner Vertex 12
        0.0f, 0.5f,  0.25f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f, // BookShelf Top Front Left Corner Vertex 13
        1.0f, 0.5f, -0.25f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f, // BookShelf Top Back Right Corner Vertex 14
        1.0f, 0.5f, -0.25f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f, // BookShelf Top Back Right Corner Vertex 15
        0.0f, 0.5f,  0.25f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f, // BookShelf Top Front Left Corner Vertex 16
        1.0f, 0.5f,  0.25f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f, // BookShelf Top Front Right Corner Vertex 17
        0.0f, 0.0f, -0.25f,    0.0f, -1.0f,  0.0f,    0.0f, 1.0f, // BookShelf Bottom Back Left Corner Vertex 18
        0.0f, 0.0f,  0.25f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f, // BookShelf Bottom Front Left Corner Vertex 19
        1.0f, 0.0f, -0.25f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f, // BookShelf Bottom Back Right Corner Vertex 20
        1.0f, 0.0f, -0.25f,    0.0f, -1.0f,  0.0f,    1.0f, 1.0f, // BookShelf Bottom Back Right Corner Vertex 21
        0.0f, 0.0f,  0.25f,    0.0f, -1.0f,  0.0f,    0.0f, 0.0f, // BookShelf Bottom Front Left Corner Vertex 22
        1.0f, 0.0f,  0.25f,    0.0f, -1.0f,  0.0f,    1.0f, 0.0f, // BookShelf Bottom Front Right Corner Vertex 23

        //Top BookShelf
        0.0f, 1.0f, -0.25f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f, // BookShelf Back Top Left Corner Vertex 24
        0.0f, 1.0f,  0.25f,   -1.0f,  0.0f,  0.0f,    0.0f, 1.0f, // BookShelf Front Top Left Corner Vertex 25
        0.0f, 0.5f, -0.25f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f, // BookShelf Back Bottom Left Corner Vertex 26
        0.0f, 0.5f, -0.25f,   -1.0f,  0.0f,  0.0f,    0.0f, 0.0f, // BookShelf Back Bottom Left Corner Vertex 27
        0.0f, 0.5f,  0.25f,   -1.0f,  0.0f,  0.0f,    1.0f, 0.0f, // BookShelf Front Bottom Left Corner Vertex 28
        0.0f, 1.0f,  0.25f,   -1.0f,  0.0f,  0.0f,    1.0f, 1.0f, // BookShelf Front Top Left Corner Vertex 29
        1.0f, 1.0f, -0.25f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f, // BookShelf Back Top Right Corner Vertex 30
        1.0f, 1.0f,  0.25f,    1.0f,  0.0f,  0.0f,    0.0f, 1.0f, // BookShelf Front Top Right Corner Vertex 31
        1.0f, 0.5f, -0.25f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f, // BookShelf Back Bottom Right Corner Vertex 32
        1.0f, 0.5f, -0.25f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f, // BookShelf Back Bottom Right Corner Vertex 33
        1.0f, 0.5f,  0.25f,    1.0f,  0.0f,  0.0f,    1.0f, 0.0f, // BookShelf Front Bottom Right Corner Vertex 34
        1.0f, 1.0f,  0.25f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f, // BookShelf Front Top Right Corner Vertex 35
        0.0f, 1.0f, -0.25f,    0.0f,  1.0f,  0.0f,    0.0f, 1.0f, // BookShelf Top Back Left Corner Vertex 36
        0.0f, 1.0f,  0.25f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f, // BookShelf Top Front Left Corner Vertex 37
        1.0f, 1.0f, -0.25f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f, // BookShelf Top Back Right Corner Vertex 38
        1.0f, 1.0f, -0.25f,    0.0f,  1.0f,  0.0f,    1.0f, 1.0f, // BookShelf Top Back Right Corner Vertex 39
        0.0f, 1.0f,  0.25f,    0.0f,  1.0f,  0.0f,    0.0f, 0.0f, // BookShelf Top Front Left Corner Vertex 40
        1.0f, 1.0f,  0.25f,    0.0f,  1.0f,  0.0f,    1.0f, 0.0f, // BookShelf Top Front Right Corner Vertex 41

        //Center Divider
        0.5f, 1.0f, -0.25f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f, // BookShelf Middle Back Top Corner Vertex 42
        0.5f, 1.0f,  0.25f,    1.0f,  0.0f,  0.0f,    0.0f, 1.0f, // BookShelf Middle Front Top Left Corner Vertex 43
        0.5f, 0.0f, -0.25f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f, // BookShelf Middle Back Bottom Left Corner Vertex 44
        0.5f, 0.0f, -0.25f,    1.0f,  0.0f,  0.0f,    0.0f, 0.0f, // BookShelf Middle Back Bottom Left Corner Vertex 45
        0.5f, 0.0f,  0.25f,    1.0f,  0.0f,  0.0f,    1.0f, 0.0f, // BookShelf Middle Front Bottom Left Corner Vertex 46
        0.5f, 1.0f,  0.25f,    1.0f,  0.0f,  0.0f,    1.0f, 1.0f // BookShelf Middle Front Top Left Corner Vertex 47

    };

    //Generate buffer ids glGenVertexArrays(1, &shelfVAO); glGenBuffers(1, &VBO);
    //Activate the shelfVAO before binding and setting VBOs and VAPs
    glBindVertexArray(ShelfVAO);

    //Activate the VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); //Copy vertices to VBO

    //Set attribute pointer 0 to hold position data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0); //Enables vertex attribute

    //Set attribute pointer 1 to hold Normal data
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    //Set attribute pointer 2 to hold Texture coordinate data
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0); //Unbind the shelf VAO

    //Generate buffer ids for lamp (smaller shelf)
    glGenVertexArrays(1, &LightVAO); //Vertex Array for shelf vertex copies to serve as light source

    //Activate the Vertex Array Object before binding and setting any VBOs and Vertex Attribute Pointers
    glBindVertexArray(LightVAO);

    //Referencing the same VBO for its vertices
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    //Set attribute pointer to 0 to hold Position data (used for the lamp)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

//Implement UMouseClick Function
void UMouseClick(int button, int state, int x, int y) {
    if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_DOWN)) {
        mouseLeft = true;
    }

    if ((button == GLUT_LEFT_BUTTON) && (state == GLUT_UP)) {
        mouseLeft = false;
    }

    if ((button == GLUT_RIGHT_BUTTON) && (state == GLUT_DOWN)) {
        mouseRight = true;
    }

    if ((button == GLUT_RIGHT_BUTTON) && (state == GLUT_UP)) {
        mouseRight = false;
    }
}

//Implements UMouseMove function
void UMousePressedMove(int x, int y) {
    if (glutGetModifiers() == GLUT_ACTIVE_ALT) {
        altKey = true;
    }
    else {
        altKey = false;
    }

    //immediately replaces center locked coordinates with new mouse coordinates
    if (mouseDetected) {
        lastMouseX = x;
        lastMouseY = y;
        mouseDetected = false;
    }

    //Get direction mouse was moved in x and y
    mouseXOffset = x - lastMouseX;
    mouseYOffset = lastMouseY - y; // Inverted y

    //Updates with new mouse coordinates
    lastMouseX = x;
    lastMouseY = y;

    //Applies sensitivity to mouse direction
    mouseXOffset *= sensitivity;
    mouseYOffset *= sensitivity;

    //accumulates yaw and pitch variables
    yaw += mouseXOffset;
    pitch += mouseYOffset;

    //maintains a 90 degree pitch for gimbal lock
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    //Orbit
    if (mouseLeft == true && altKey == true) {
        //Orbits around center
        cameraPosition.x = 3.0f * cos(yaw);
        cameraPosition.y = 3.0f * sin(pitch);
        cameraPosition.z = sin(yaw) * cos(pitch) * 10.0f;
        if (mouseXOffset < 0)
            cameraPosition -= glm::normalize(glm::cross(CameraForwardZ, CameraUpY)) * cameraSpeed; //Orbit Right
        if (mouseXOffset > 0)
            cameraPosition += glm::normalize(glm::cross(CameraForwardZ, CameraUpY)) * cameraSpeed; // Orbit Left
        if (mouseYOffset < 0)
            cameraPosition -= CameraUpY * cameraSpeed; // Orbit Up
        if (mouseYOffset > 0)
            cameraPosition += CameraUpY * cameraSpeed; // Orbit down
    }
}

/*Generate and load texture*/
void UGenerateTexture() {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    int width, height;
    unsigned char* image = SOIL_load_image("black-wood.jpg", &width, &height, 0, SOIL_LOAD_RGB); //Load texture file
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    SOIL_free_image_data(image);
    glBindTexture(GL_TEXTURE_2D, 0); //unbind texture
}

//Implement UKeyboard Function
void UKeyboard(unsigned char key, GLint x, GLint y) {
    switch (key) {
    case '2':
        currentKey = key;
        cout << "Switching to 2D!" << endl;
        break;
    case '3':
        currentKey = key;
        cout << "Switching to 3D!" << endl;
        break;
    default:
        cout << "Press a key!" << endl;
    }
}

