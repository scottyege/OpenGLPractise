
/* Using the standard output for fprintf */
#include <stdio.h>
#include <stdlib.h>

/* Use glew.h instead of gl.h to get all the GL prototypes declared */
#include <GL/glew.h>
/* Using the GLUT library for the base windowing setup */
#include <GL/freeglut.h>

#include "shader_utils.h"

#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <SOIL.h>

#include<iostream>
using std::cerr;
using std::cout;

#include "MyClasses.h"

/* ADD GLOBAL VARIABLES HERE LATER */
GLuint texture_id;
GLuint program;

xDModel *xdModel = NULL;

int screen_width = 800, screen_height = 600;

struct FPSCount
{
    int frame;
    int timebase;
    int time;
} fpsCount = {0, 0, 0};

struct CameraProperty
{
    glm::vec3 eye;
    glm::vec3 at;
    glm::vec3 up;

    GLfloat fov;
} cameraProp;

struct ShaderAttriLoc
{
    GLuint vObjPos;
    GLuint vObjNormal;

    GLuint vTex;
} attriLoc;

struct ShaderUniformLoc
{
    GLuint modelMatrix;
    GLuint viewMatrix;
    GLuint projMatrix;
    GLuint normalMatrix;

    GLuint lightModelMatrix;
    GLuint lightPosition;
    GLuint lightDiffuse;
    GLuint lightAmbient;

    GLuint mat_diffuse;
    GLuint mat_ambient;

    GLuint sampler;
} uniformLoc;

struct Buffers
{
    GLuint vertexBuffer;
    GLuint normalBuffer;
    GLuint textureBuffer;

    GLuint elementBuffer;
} buffers;

struct PointLight
{
    glm::vec4 position;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
} plight =
{
    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
    glm::vec4(0.3f, 0.3f, 0.3f, 1.0f),
    glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
    glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
};

xDMaterial modelMat =
{
    glm::vec4(0.5f,0.5f,0.5f,1.0f),
    glm::vec4(0.5f,0.5f,0.5f,1.0f),
    glm::vec4(0.5f,0.5f,0.5f,1.0f)
};

void loadTexture()
{

    int width, height, channels;
    unsigned char *ht_map = SOIL_load_image
                            (
                                "fiber.jpg",
                                &width, &height, &channels,
                                SOIL_LOAD_AUTO
                            );
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, ht_map);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

}

void calCameraSet()
{
    cameraProp.up = glm::vec3(0.0f, 1.0f, 0.0f);
    cameraProp.at = xdModel->center;

    cameraProp.fov = 45.0f;

    float distanceToCamera = xdModel->boundingShpereRadius / tanf(M_PI * cameraProp.fov / 360.0f);
    cameraProp.eye = xdModel->center;
    cameraProp.eye.z = distanceToCamera;
}

/*
Function: init_resources
Receives: void
Returns: int
This function creates all GLSL related stuff
explained in this example.
Returns 1 when all is ok, 0 with a displayed error
*/
int init_resources(void)
{
    //loading model
    xdModel = new xDModel("teapot.obj");
    //
    //setting shader
    GLint link_ok = GL_FALSE;

    GLuint vs, fs;
    if ((vs = create_shader("triangle.vert", GL_VERTEX_SHADER))   == 0) return 0;
    if ((fs = create_shader("triangle.frag", GL_FRAGMENT_SHADER)) == 0) return 0;

    program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
    if (!link_ok)
    {
        fprintf(stderr, "glLinkProgram:");
        print_log(program);
        return 0;
    }


    ////attributes
    if(!getAttributeLoc(program, "vObjPos", attriLoc.vObjPos)
            || !getAttributeLoc(program, "vObjNormal", attriLoc.vObjNormal)
            || !getAttributeLoc(program, "vTex", attriLoc.vTex)
      )

        return 0;

    //uniforms
    if(
        !getUniformLoc(program, "modelMatrix", uniformLoc.modelMatrix)
        || !getUniformLoc(program, "projMatrix", uniformLoc.projMatrix)
        || !getUniformLoc(program, "viewMatrix", uniformLoc.viewMatrix)
        || !getUniformLoc(program, "normalMatrix", uniformLoc.normalMatrix)
        || !getUniformLoc(program, "light_diffuse", uniformLoc.lightDiffuse)
        || !getUniformLoc(program, "lightModelMatrix", uniformLoc.lightModelMatrix)
        || !getUniformLoc(program, "lightPosition", uniformLoc.lightPosition)
        || !getUniformLoc(program, "mat_diffuse", uniformLoc.mat_diffuse)
        || !getUniformLoc(program, "mat_ambient", uniformLoc.mat_ambient)
        || !getUniformLoc(program, "light_ambient", uniformLoc.lightAmbient)
        || !getUniformLoc(program, "sampler", uniformLoc.sampler)
    )
        return 0;

    loadTexture();
    calCameraSet();

    return 1;
}

void onDisplay()
{

    /* Clear the background as white */
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);


    glUseProgram(program);

    glEnableVertexAttribArray(attriLoc.vObjPos);
    glEnableVertexAttribArray(attriLoc.vObjNormal);
    glEnableVertexAttribArray(attriLoc.vTex);


    glUniform4fv(uniformLoc.lightDiffuse, 1, glm::value_ptr(plight.diffuse));
    glUniform4fv(uniformLoc.lightAmbient, 1, glm::value_ptr(plight.ambient));
    glUniform4fv(uniformLoc.lightPosition, 1, glm::value_ptr(plight.position));
    glUniform4fv(uniformLoc.mat_diffuse, 1, glm::value_ptr(modelMat.diffuse));
    glUniform4fv(uniformLoc.mat_ambient, 1, glm::value_ptr(modelMat.ambient));

    GLMmodel *m = xdModel->glmModel;

    glBegin(GL_TRIANGLES);
    for(int i = 0; i < m->numtriangles; i++)
    {
        for(int j = 0; j < 3; j++)
        {
            GLuint vIdx = m->triangles[i].vindices[j] * 3;
            GLuint nIdx = m->triangles[i].nindices[j] * 3;
            GLuint tIdx = m->triangles[i].tindices[j] * 2;

            glVertexAttrib3fv(attriLoc.vObjNormal, &m->normals[nIdx]);
            glVertexAttrib3fv(attriLoc.vTex, &m->texcoords[tIdx]);

            glVertexAttrib3fv(attriLoc.vObjPos, &m->vertices[vIdx]);
        }
    }
    glEnd();


    glDisableVertexAttribArray(attriLoc.vObjPos);
    glDisableVertexAttribArray(attriLoc.vObjNormal);
    glDisableVertexAttribArray(attriLoc.vTex);


    /* Display the result */
    glutSwapBuffers();
}

void free_resources()
{
    glDeleteProgram(program);

    if(xdModel)
        delete xdModel;
}

void onReshape(int width, int height)
{
    screen_width = width;
    screen_height = height;
    glViewport(0, 0, screen_width, screen_height);
}

void onIdle()
{

    fpsCount.frame++;
    fpsCount.time = glutGet(GLUT_ELAPSED_TIME);

    if (fpsCount.time - fpsCount.timebase > 1000)
    {
        double fps = fpsCount.frame * 1000.0 / (fpsCount.time - fpsCount.timebase);
        fpsCount.timebase = fpsCount.time;
        fpsCount.frame = 0;
        printf("fps: %f\n", fps);
    }

    glUseProgram(program);
    //
    float angle = glutGet(GLUT_ELAPSED_TIME) / 1000.0f * 45.0f;
    glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 model = rotate * glm::translate(glm::mat4(1.0f), -xdModel->center);
    glm::mat4 view = glm::lookAt(cameraProp.eye, cameraProp.at, cameraProp.up);
    glm::mat4 projection = glm::perspective(cameraProp.fov, 1.0f*screen_width/screen_height, 0.1f, 1000.0f);

    glUniformMatrix4fv(uniformLoc.modelMatrix, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(uniformLoc.viewMatrix, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(uniformLoc.projMatrix, 1, GL_FALSE, glm::value_ptr(projection));

    glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(view * model)));
    glUniformMatrix3fv(uniformLoc.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalMatrix));

    //light transformation
    glm::mat4 lightModelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(sinf(glutGet(GLUT_ELAPSED_TIME) / 1000.0 * (2*3.14) / 5) * 50.0f, 15.0f, 0.0f));
    glUniformMatrix4fv(uniformLoc.lightModelMatrix, 1, GL_FALSE, glm::value_ptr(lightModelMatrix));

    glutPostRedisplay();
}

int main(int argc, char* argv[])
{
    /* Glut-related initialising functions */
    glutInit(&argc, argv);
    glutInitContextVersion(2,0);
    glutInitDisplayMode(GLUT_RGBA|GLUT_DOUBLE|GLUT_DEPTH|GLUT_ALPHA);
    glutInitWindowSize(screen_width, screen_height);
    glutCreateWindow("ZZZZZZZZ");

    /* Extension wrangler initialising */
    GLenum glew_status = glewInit();
    if (glew_status != GLEW_OK)
    {
        fprintf(stderr, "Error: %s\n", glewGetErrorString(glew_status));
        return EXIT_FAILURE;
    }

    /* When all init functions run without errors,
    the program can initialise the resources */
    if (1 == init_resources())
    {
        /* We can display it if everything goes OK */
        glutDisplayFunc(onDisplay);
        glutIdleFunc(onIdle);
        glutReshapeFunc(onReshape);
        glutMainLoop();
    }

    /* If the program exits in the usual way,
    free resources and exit with a success */
    free_resources();
    return EXIT_SUCCESS;
}