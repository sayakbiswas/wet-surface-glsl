#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <fstream>
#include <vector>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>

using namespace glm;
using namespace std;

const int window_width = 800, window_height = 600;
GLFWwindow *window;
typedef struct Vertex {
    float Position[4];
    float Color[4];
    float Normal[3];

    void SetPosition(float *coords) {
        Position[0] = coords[0];
        Position[1] = coords[1];
        Position[2] = coords[2];
        Position[3] = 1.0;
    }

    void SetColor(float *color) {
        Color[0] = color[0];
        Color[1] = color[1];
        Color[2] = color[4];
        Color[3] = color[3];
    }

    void SetNormal(float *normal) {
        Normal[0] = normal[0];
        Normal[1] = normal[1];
        Normal[2] = normal[2];
    }
} Vertex;
glm::mat4 gProjectionMatrix;
glm::mat4 gViewMatrix;
GLuint programID;
const GLuint numObjects = 256;
GLuint vertexBufferID[numObjects] = {0};
GLuint vertexArrayID[numObjects] = {0};
GLuint indexBufferID[numObjects] = {0};
size_t numIndices[numObjects] = {0};
size_t vertexBufferSize[numObjects] = {0};
size_t indexBufferSize[numObjects] = {0};
Vertex* suzanne_verts;
GLushort* suzanne_idcs;
GLuint matrixID;
GLuint modelMatrixID;
GLuint viewMatrixID;
GLuint projectionMatrixID;
GLuint lightID;
GLfloat cameraAngleTheta = 3.142 / 4;
GLfloat cameraAnglePhi = asin(1 / sqrt(3));
GLfloat cameraSphereRadius = sqrt(675);
bool moveCameraLeft = false;
bool moveCameraRight = false;
bool moveCameraUp = false;
bool moveCameraDown = false;
bool shouldDisplayWireframeMode = false;

int initWindow(void);
void initOpenGL(void);
void createObjects(void);
static void keyCallback(GLFWwindow* , int, int, int, int);
static void mouseCallback(GLFWwindow*, int, int, int);
GLuint loadStandardShaders(const char*, const char*);
void loadObject(char*, glm::vec4, Vertex* &, GLushort* &, int);
void createVAOs(Vertex[], GLushort[], int);
void renderScene(void);
void cleanup(void);

int main(void) {
    int errorCode = initWindow();
    if(errorCode != 0) {
        return errorCode;
    }

    initOpenGL();

    do {
        renderScene();
    } while(glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS && glfwWindowShouldClose(window) == 0);

    cleanup();

    return 0;
}

void initOpenGL() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);

    gProjectionMatrix = glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f);

    gViewMatrix = glm::lookAt(glm::vec3(15.0f, 15.0f, 15.0f),
                              glm::vec3(0.0f, 10.0f, 0.0f),
                              glm::vec3(0.0f, 1.0f, 0.0f));

    programID = loadStandardShaders("shaders/Standard.vs.glsl", "shaders/Standard.fs.glsl");

    matrixID = glGetUniformLocation(programID, "MVP");
    modelMatrixID = glGetUniformLocation(programID, "M");
    viewMatrixID = glGetUniformLocation(programID, "V");
    projectionMatrixID = glGetUniformLocation(programID, "P");
    lightID = glGetUniformLocation(programID, "lightPosition_worldspace");

    createObjects();
}

void createObjects() {
    //-- COORDINATE AXES --//
    Vertex coordVerts[] =
    {
        { { 0.0, 0.0, 0.0, 1.0 },{ 1.0, 0.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 5.0, 0.0, 0.0, 1.0 },{ 1.0, 0.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 0.0, 0.0, 0.0, 1.0 },{ 0.0, 1.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 0.0, 5.0, 0.0, 1.0 },{ 0.0, 1.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 0.0, 0.0, 0.0, 1.0 },{ 0.0, 0.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
        { { 0.0, 0.0, 5.0, 1.0 },{ 0.0, 0.0, 1.0, 1.0 },{ 0.0, 0.0, 1.0 } },
    };

    vertexBufferSize[0] = sizeof(coordVerts);	// ATTN: this needs to be done for each hand-made object with the ObjectID (subscript)
    createVAOs(coordVerts, NULL, 0);

    loadObject("Model/Suzanne.obj", glm::vec4(0.4, 0.5, 0.3, 1.0), suzanne_verts, suzanne_idcs, 1);
    createVAOs(suzanne_verts, suzanne_idcs, 1);
}

void loadObject(char* file, glm::vec4 color, Vertex * &out_vertices, GLushort * &out_indices, int objectID) {
    vector<glm::vec3> vertices;
    vector<glm::vec3> normals;

    bool res = loadOBJ(file, vertices, normals);

    vector<GLushort> indices;
    vector<glm::vec3> indexed_vertices;
    vector<glm::vec3> indexed_normals;
    indexVBO(vertices, normals, indices, indexed_vertices, indexed_normals);

    const size_t vert_count = indexed_vertices.size();
    const size_t idx_count = indices.size();

    out_vertices = new Vertex[vert_count];
    for(int i = 0; i < vert_count; i++) {
        out_vertices[i].SetPosition(&indexed_vertices[i].x);
        out_vertices[i].SetNormal(&indexed_normals[i].x);
        out_vertices[i].SetColor(&color[0]);
    }

    out_indices = new GLushort[idx_count];
    for(int i = 0; i < idx_count; i++) {
        out_indices[i] = indices[i];
    }

    numIndices[objectID] = idx_count;
    vertexBufferSize[objectID] = sizeof(out_vertices[0]) * vert_count;
    indexBufferSize[objectID] = sizeof(GLushort) * idx_count;
}

void createVAOs(Vertex vertices[], GLushort indices[], int objectID) {
    GLenum errorCheckValue = glGetError();
    const size_t vertexSize = sizeof(vertices[0]);
    const size_t colorOffset = sizeof(vertices[0].Position);
    const size_t normalOffset = colorOffset + sizeof(vertices[0].Color);

    glGenVertexArrays(1, &vertexArrayID[objectID]);
    glBindVertexArray(vertexArrayID[objectID]);

    glGenBuffers(1, &vertexBufferID[objectID]);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID[objectID]);
    glBufferData(GL_ARRAY_BUFFER, vertexBufferSize[objectID], vertices, GL_STATIC_DRAW);

    if(indices != NULL) {
        glGenBuffers(1, &indexBufferID[objectID]);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID[objectID]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize[objectID], indices, GL_STATIC_DRAW);
    }

    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, vertexSize, 0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, vertexSize, (GLvoid*)colorOffset);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, vertexSize, (GLvoid*)normalOffset);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    errorCheckValue = glGetError();
    if(errorCheckValue != GL_NO_ERROR) {
        fprintf(stderr, "Error: Could not create a VBO: %s\n", gluErrorString(errorCheckValue));
    }
}

void renderScene() {
    glClearColor(0.0f, 0.0f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_CULL_FACE);

    if (moveCameraLeft) {
        cameraAngleTheta -= 0.01f;
    }

    if (moveCameraRight) {
        cameraAngleTheta += 0.01f;
    }

    if (moveCameraUp) {
        cameraAnglePhi -= 0.01f;
    }

    if (moveCameraDown) {
        cameraAnglePhi += 0.01f;
    }

    if (moveCameraLeft || moveCameraRight || moveCameraDown || moveCameraUp) {
        float camX = cameraSphereRadius * cos(cameraAnglePhi) * sin(cameraAngleTheta);
        float camY = cameraSphereRadius * sin(cameraAnglePhi);
        float camZ = cameraSphereRadius * cos(cameraAnglePhi) * cos(cameraAngleTheta);
        gViewMatrix = glm::lookAt(glm::vec3(camX, camY, camZ),	// eye
            glm::vec3(0.0, 10.0, 0.0),	// center
            glm::vec3(0.0, 1.0, 0.0));	// up
    }

    if(shouldDisplayWireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    glm::vec3 lightPos = glm::vec3(5.0f, 15.0f, 5.0f);
    glm::mat4x4 modelMatrix = glm::mat4(1.0);
    glUseProgram(programID);
    {
        glUniform3f(lightID, lightPos.x, lightPos.y, lightPos.z);
        glUniformMatrix4fv(viewMatrixID, 1, GL_FALSE, &gViewMatrix[0][0]);
        glUniformMatrix4fv(projectionMatrixID, 1, GL_FALSE, &gProjectionMatrix[0][0]);
        glUniformMatrix4fv(modelMatrixID, 1, GL_FALSE, &modelMatrix[0][0]);

        glBindVertexArray(vertexArrayID[0]);	// draw CoordAxes
        glDrawArrays(GL_LINES, 0, 6);

        glBindVertexArray(vertexArrayID[1]);
        glDrawElements(GL_TRIANGLES, numIndices[1], GL_UNSIGNED_SHORT, (void*)0);

        glBindVertexArray(0);
    }

    glUseProgram(0);
    glfwSwapBuffers(window);
    glfwPollEvents();
}

GLuint loadStandardShaders(const char *vert_file_path, const char *frag_file_path) {
    GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    string vertexShaderCode;
    ifstream vertexShaderStream(vert_file_path, ios::in);
    if(vertexShaderStream.is_open()) {
        string line = "";
        while(std::getline(vertexShaderStream, line)) {
            vertexShaderCode += "\n" + line;
        }
        vertexShaderStream.close();
    } else {
        cout << "Couldn't open " << vert_file_path << endl;
        getchar();
        return 0;
    }

    string fragmentShaderCode;
    ifstream fragmentShaderStream(frag_file_path, ios::in);
    if(fragmentShaderStream.is_open()) {
        string line = "";
        while(std::getline(fragmentShaderStream, line)) {
            fragmentShaderCode += "\n" + line;
        }
        fragmentShaderStream.close();
    } else {
        cout << "Couldn't open " << frag_file_path << endl;
        getchar();
        return 0;
    }

    GLint result = GL_FALSE;
    int infoLogLength;

    cout << "Compiling shader " <<  vert_file_path << endl;
    char const *vertexSource = vertexShaderCode.c_str();
    glShaderSource(vertexShaderID, 1, &vertexSource, NULL);
    glCompileShader(vertexShaderID);

    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 0) {
        vector<char> vertexShaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(vertexShaderID, infoLogLength, NULL, &vertexShaderErrorMessage[0]);
        cout << &vertexShaderErrorMessage[0] << endl;
    }

    cout << "Compiling shader " <<  frag_file_path << endl;
    char const *fragmentSource = fragmentShaderCode.c_str();
    glShaderSource(fragmentShaderID, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShaderID);

    glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 0) {
        vector<char> fragmentShaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(fragmentShaderID, infoLogLength, NULL, &fragmentShaderErrorMessage[0]);
        cout << &fragmentShaderErrorMessage[0] << endl;
    }

    cout << "Linking shader" << endl;
    GLuint programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);

    glGetShaderiv(programID, GL_LINK_STATUS, &result);
    glGetShaderiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
    if(infoLogLength > 0) {
        vector<char> programErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(programID, infoLogLength, NULL, &programErrorMessage[0]);
        cout << programErrorMessage[0] << endl;
    }

    glDetachShader(programID, vertexShaderID);
    glDetachShader(programID, fragmentShaderID);

    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    return programID;
}

int initWindow() {
    if(!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW!\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(window_width, window_height, "PN Triangles", NULL, NULL);
    if(window == NULL) {
        fprintf(stderr, "Failed to open GLFW window.\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = true;
    if(glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW!\n");
        return -1;
    }

    glfwSetCursorPos(window, window_width / 2, window_height / 2);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseCallback);

    return 0;
}

void cleanup() {
    for(int i = 0; i < numObjects; i++) {
        glDeleteBuffers(1, &vertexBufferID[i]);
        glDeleteBuffers(1, &indexBufferID[i]);
        glDeleteVertexArrays(1, &vertexArrayID[i]);
    }
    glDeleteProgram(programID);
    glfwTerminate();
}

static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if(action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_LEFT:
            moveCameraLeft = true;
            break;
        case GLFW_KEY_RIGHT:
            moveCameraRight = true;
            break;
        case GLFW_KEY_UP:
            moveCameraUp = true;
            break;
        case GLFW_KEY_DOWN:
            moveCameraDown = true;
            break;
        default:
            break;
        }
    } else if(action == GLFW_RELEASE) {
        switch (key) {
        case GLFW_KEY_LEFT:
            moveCameraLeft = false;
            break;
        case GLFW_KEY_RIGHT:
            moveCameraRight = false;
            break;
        case GLFW_KEY_UP:
            moveCameraUp = false;
            break;
        case GLFW_KEY_DOWN:
            moveCameraDown = false;
            break;
        case GLFW_KEY_T:
            break;
        case GLFW_KEY_W:
            shouldDisplayWireframeMode = ! shouldDisplayWireframeMode;
            break;
        case GLFW_KEY_LEFT_BRACKET:
            break;
        case GLFW_KEY_RIGHT_BRACKET:
            break;
        default:
            break;
        }
    }
}

static void mouseCallback(GLFWwindow *window, int button, int action, int mods) {

}
