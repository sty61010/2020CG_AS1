#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "textfile.h"
#include "Vectors.h"
#include "Matrices.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifndef max
# define max(a,b) (((a)>(b))?(a):(b))
# define min(a,b) (((a)<(b))?(a):(b))
#endif

#define OFFSET 0.05
#define PI 3.1415926

using namespace std;

// Default window size
int WINDOW_WIDTH = 800;
int WINDOW_HEIGHT = 600;

bool mouse_pressed = false;
int starting_press_x = -1;
int starting_press_y = -1;

Matrix4 view_matrix;
Matrix4 project_matrix;
Matrix4 T, R, S;
//void PrintMatrix(Matrix4 m){
//    cout << m[0] << " " << m[4] << " " << m[8] << " " << m[12] << endl;
//    cout << m[1] << " " << m[5] << " " << m[9] << " " << m[13] << endl;
//    cout << m[2] << " " << m[6] << " " << m[10] << " " << m[14] << endl;
//    cout << m[3] << " " << m[7] << " " << m[11] << " " << m[15] << endl;
//}

void PrintAllMatrix(){
    cout << "------------------------------------" << endl;
    cout << "Viewing Matrrix" <<endl;
    cout << view_matrix;
    cout << "------------------------------------" << endl;
    cout << "Projection Matrix" <<endl;
    cout << project_matrix;
    cout << "------------------------------------" << endl;
    cout << "Translate Matrix" << endl;
    cout << T;
    cout << "------------------------------------" << endl;
    cout << "Rotation Matrix" <<endl;
    cout << R;
    cout << "------------------------------------" << endl;
    cout << "Scaling Matrix" <<endl;
    cout << S;

}
enum TransMode
{
    GeoTranslation = 0,
    GeoRotation = 1,
    GeoScaling = 2,
    ViewCenter = 3,
    ViewEye = 4,
    ViewUp = 5,
};

GLint iLocMVP;
vector<string> filenames; // .obj filename list

struct model
{
    Vector3 position = Vector3(0, 0, 0);
    Vector3 scale = Vector3(1, 1, 1);
    Vector3 rotation = Vector3(0, 0, 0);    // Euler form
};
vector<model> models;

struct camera
{
    Vector3 position;
    Vector3 center;
    Vector3 up_vector;
};
camera main_camera;

struct project_setting
{
    GLfloat nearClip, farClip;
    GLfloat fovy;
    GLfloat aspect;
    GLfloat left, right, top, bottom;
};
project_setting proj;

enum ProjMode
{
    Orthogonal = 0,
    Perspective = 1,
};
ProjMode cur_proj_mode = Orthogonal;
TransMode cur_trans_mode = GeoTranslation;
typedef struct
{
    GLuint vao;
    GLuint vbo;
    GLuint vboTex;
    GLuint ebo;
    GLuint p_color;
    int vertex_count;
    GLuint p_normal;
    int materialId;
    int indexCount;
    GLuint m_texture;
} Shape;
Shape quad;
Shape m_shpae;
vector<Shape> m_shape_list;
int cur_idx = 0; // represent which model should be rendered now


static GLvoid Normalize(GLfloat v[3])
{
    GLfloat l;
    l = (GLfloat)sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    v[0] /= l;
    v[1] /= l;
    v[2] /= l;
}

static GLvoid Cross(GLfloat u[3], GLfloat v[3], GLfloat n[3])
{
    n[0] = u[1] * v[2] - u[2] * v[1];
    n[1] = u[2] * v[0] - u[0] * v[2];
    n[2] = u[0] * v[1] - u[1] * v[0];
}


// [TODO] given a translation vector then output a Matrix4 (Translation Matrix)
Matrix4 translate(Vector3 vec)
{
    Matrix4 mat;
    /*
    mat = Matrix4(
        ...
    );
    */
    mat = Matrix4(
		1, 0, 0, vec.x,
        0, 1, 0, vec.y,
        0, 0, 1, vec.z,
        0, 0, 0, 1);

    return mat;
}

// [TODO] given a scaling vector then output a Matrix4 (Scaling Matrix)
Matrix4 scaling(Vector3 vec)
{
    Matrix4 mat;

    /*
    mat = Matrix4(
        ...
    );
    */
    mat = Matrix4(
		vec.x, 0, 0, 0,
        0, vec.y, 0, 0,
        0, 0, vec.z, 0,
        0, 0, 0, 1);

    return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-X (rotate alone axis-X)
Matrix4 rotateX(GLfloat val)
{
    Matrix4 mat;

    /*
    mat = Matrix4(
        ...
    );
    */
    mat = Matrix4(
		1, 0, 0, 0,
        0, cos(val), -sin(val), 0,
        0, sin(val), cos(val), 0,
        0, 0, 0, 1);

    return mat;
}
// [TODO] given a float value then ouput a rotation matrix alone axis-Y (rotate alone axis-Y)
Matrix4 rotateY(GLfloat val)
{
    Matrix4 mat;

    /*
    mat = Matrix4(
        ...
    );
    */
    mat = Matrix4(
		cos(val), 0, sin(val), 0,
        0, 1, 0, 0,
        -sin(val), 0, cos(val), 0,
        0, 0, 0, 1);

    return mat;
}

// [TODO] given a float value then ouput a rotation matrix alone axis-Z (rotate alone axis-Z)
Matrix4 rotateZ(GLfloat val)
{
    Matrix4 mat;

    /*
    mat = Matrix4(
        ...
    );
    */
    mat = Matrix4(
		cos(val), -sin(val), 0, 0,
        sin(val), cos(val), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1);
    return mat;
}

Matrix4 rotate(Vector3 vec)
{
    return rotateX(vec.x)*rotateY(vec.y)*rotateZ(vec.z);
}

// [TODO] compute viewing matrix accroding to the setting of main_camera
void setViewingMatrix()
{
    // view_matrix[...] = ...
	/*
    Vector3 p1, p2, p3;
    p1 = main_camera.position;
    p2 = main_camera.center;
    p3 = main_camera.up_vector;

    Vector3 p1p2 = p2 - p1;
    Vector3 p1p3 = p3;
    GLfloat p1p2_norm[3] = { p1p2.x, p1p2.y, p1p2.z };
    Normalize(p1p2_norm);
    GLfloat Rz[3];
    Rz[0] = -p1p2_norm[0];
    
    Rz[1] = -p1p2_norm[1];
    Rz[2] = -p1p2_norm[2];

    GLfloat u[3] = { p1p2.x, p1p2.y, p1p2.z };
    GLfloat v[3] = { p1p3.x, p1p3.y, p1p3.z };
    GLfloat p1p2Xp1p3[3] = { 0.0, 0.0, 0.0 };
    Cross(u, v, p1p2Xp1p3);
    GLfloat p1p2Xp1p3_norm[3] = { p1p2Xp1p3[0], p1p2Xp1p3[1], p1p2Xp1p3[2] };
    Normalize(p1p2Xp1p3_norm);
    GLfloat Rx[3];
    Rx[0] = p1p2Xp1p3_norm[0];
    Rx[1] = p1p2Xp1p3_norm[1];
    Rx[2] = p1p2Xp1p3_norm[2];
    GLfloat Ry[3];
    Cross(Rz, Rx, Ry);

    Matrix4 R;
    R = Matrix4(Rx[0], Rx[1], Rx[2], 0,
        Ry[0], Ry[1], Ry[2], 0,
        Rz[0], Rz[1], Rz[2], 0,
        0, 0, 0, 1);

    Matrix4 T = translate(-main_camera.position);

    view_matrix = R * T;
	*/
	GLfloat r1x, r2x, r3x;
	GLfloat r1y, r2y, r3y;
	GLfloat r1z, r2z, r3z;
	//vectorp1p2
	GLfloat p1_p2_x = main_camera.center.x - main_camera.position.x;
	GLfloat p1_p2_y = main_camera.center.y - main_camera.position.y;
	GLfloat p1_p2_z = main_camera.center.z - main_camera.position.z;
	Vector3 p1_p2 = Vector3(p1_p2_x, p1_p2_y, p1_p2_z);

	//vectorp1p3
	GLfloat p1_p3_x = main_camera.up_vector.x - main_camera.position.x;
	GLfloat p1_p3_y = main_camera.up_vector.y - main_camera.position.y;
	GLfloat p1_p3_z = main_camera.up_vector.z - main_camera.position.z;
	Vector3 p1_p3 = Vector3(p1_p3_x, p1_p3_y, p1_p3_z);

	Vector3 rz = -p1_p2 / p1_p2.length();
	Vector3 rx = p1_p2.cross(p1_p3) / p1_p2.cross(p1_p3).length();
	Vector3 ry = rz.cross(rx);


	Matrix4 R_base = Matrix4(rx.x, rx.y, rx.z, 0,
		ry.x, ry.y, ry.z, 0,
		rz.x, rz.y, rz.z, 0,
		0, 0, 0, 1);
	Matrix4 T = Matrix4(1, 0, 0, -main_camera.position.x,
		0, 1, 0, -main_camera.position.y,
		0, 0, 1, -main_camera.position.z,
		0, 0, 0, 1);
	view_matrix = R_base * T;
}

// [TODO] compute orthogonal projection matrix
void setOrthogonal()
{
    cur_proj_mode = Orthogonal;
    // project_matrix [...] = ...
    project_matrix = Matrix4(
		2 / (proj.right - proj.left), 0, 0, -(proj.right + proj.left) / (proj.right - proj.left),
        0, 2 / (proj.top - proj.bottom), 0, -(proj.top + proj.bottom) / (proj.top - proj.bottom),
        0, 0, -2 / (proj.farClip - proj.nearClip), -(proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip),
        0, 0, 0, 1
    );

}

// [TODO] compute persepective projection matrix
void setPerspective()
{
    cur_proj_mode = Perspective;
    // project_matrix [...] = ...
    project_matrix = Matrix4(
        2 * proj.nearClip / (proj.right - proj.left), 0, (proj.right + proj.left) / (proj.right - proj.left), 0,
        0, 2 * proj.nearClip / (proj.top - proj.bottom), (proj.top + proj.bottom) / (proj.top - proj.bottom), 0,
        0, 0, -((proj.farClip + proj.nearClip) / (proj.farClip - proj.nearClip)), -2 * proj.nearClip * proj.farClip / (proj.farClip - proj.nearClip),
        0, 0, -1, 0
    );
}


// Vertex buffers
GLuint VAO, VBO, Color_BO;
// Call back function for window reshape
void ChangeSize(GLFWwindow* window, int width, int height)
{
//	GLsizei p;
//	p = width < height ? height : width;
//	glViewport(0, 0, p, p);
    if (width > height) {
        glViewport((width - height) / 2, 0, min(width, height), min(width, height));
    }
    else {
        glViewport(0, (height - width) / 2, min(width, height), min(width, height));
    }
    //glViewport(0, 0, width, height);
    // [TODO] change your aspect ratio
    proj.aspect = (GLfloat)width / (GLfloat)height;
    WINDOW_HEIGHT = height;
    WINDOW_WIDTH = width;
	glMatrixMode(GL_PROJECTION);
	/*
	glLoadIdentity();

	if (width <= height)
	{
		glOrtho(-100.0f, 100.0f, -100.0f / proj.aspect, 100.0f /proj.aspect, 1.0f, -1.0f);
	}
	else
	{
		glOrtho(-100.0f * proj.aspect, 100.0f * proj.aspect, -100.0f, 100.0f, 1.0f, -1.0f);
	}
	glMatrixMode(GL_MODELVIEW);
	*/
	glLoadIdentity();
	glOrtho(-100.0, 100.0, -100.0, 100.0, 1, -1);
}

void drawPlane()
{
    GLfloat vertices[18]{ 1.0, -0.9, -1.0,
        1.0, -0.9,  1.0,
        -1.0, -0.9, -1.0,
        1.0, -0.9,  1.0,
        -1.0, -0.9,  1.0,
        -1.0, -0.9, -1.0 };

    GLfloat colors[18]{ 0.0,1.0,0.0,
        0.0,0.5,0.8,
        0.0,1.0,0.0,
        0.0,0.5,0.8,
        0.0,0.5,0.8,
        0.0,1.0,0.0 };

    // [TODO] draw the plane with above vertices and color*
    Matrix4 MVP = project_matrix * view_matrix;
    GLfloat mvp[16];
    mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
    mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
    mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
    mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];
    glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
	
    glGenVertexArrays(1, &quad.vao);
    glBindVertexArray(quad.vao);
    glGenBuffers(1, &quad.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, quad.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    quad.vertex_count = 18 ;
    glGenBuffers(1, &quad.p_color);
    glBindBuffer(GL_ARRAY_BUFFER, quad.p_color);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
   
    glDrawArrays(GL_TRIANGLES, 0, 18);
}
// Render function for display rendering
void RenderScene(void) {
    // clear canvas
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    // [TODO] update translation, rotation and scaling
    T = translate(models[cur_idx].position);
    R = rotate(models[cur_idx].rotation);
    S = scaling(models[cur_idx].scale);

    Matrix4 MVP;
    MVP = project_matrix * view_matrix * T * R * S;
    GLfloat mvp[16];
    // [TODO] multiply all the matrix
    // [TODO] row-major ---> column-major
    mvp[0] = MVP[0];  mvp[4] = MVP[1];   mvp[8] = MVP[2];    mvp[12] = MVP[3];
    mvp[1] = MVP[4];  mvp[5] = MVP[5];   mvp[9] = MVP[6];    mvp[13] = MVP[7];
    mvp[2] = MVP[8];  mvp[6] = MVP[9];   mvp[10] = MVP[10];   mvp[14] = MVP[11];
    mvp[3] = MVP[12]; mvp[7] = MVP[13];  mvp[11] = MVP[14];   mvp[15] = MVP[15];

    // use uniform to send mvp to vertex shader
    glUniformMatrix4fv(iLocMVP, 1, GL_FALSE, mvp);
    glBindVertexArray(m_shape_list[cur_idx].vao);
    glDrawArrays(GL_TRIANGLES, 0, m_shape_list[cur_idx].vertex_count);
    drawPlane();
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods){
    // [TODO] Call back function for keyboard
    if (action == GLFW_PRESS) {
        switch (key){
        case GLFW_KEY_SPACE:
            exit(0);
            break;
        case GLFW_KEY_Z:
//            if (done) {
                cur_idx = (cur_idx - 1 + 5)% 5;
//                offset = -2;
//                dir = -1;
//            }
            break;
        case GLFW_KEY_X:
//            if (done) {
                cur_idx = (cur_idx + 1) % 5;
//                offset = -2;
//                dir = 1;
//            }
            break;
        case GLFW_KEY_O:
            cur_proj_mode = Orthogonal;
            setOrthogonal();
            break;
        case GLFW_KEY_P:
            cur_proj_mode = Perspective;
            setPerspective();
            break;
        case GLFW_KEY_T:
            cur_trans_mode = GeoTranslation;
            break;
        case GLFW_KEY_S:
            cur_trans_mode = GeoScaling;
            break;
        case GLFW_KEY_R:
            cur_trans_mode = GeoRotation;
            break;
        case GLFW_KEY_E:
            cur_trans_mode = ViewEye;
            break;
        case GLFW_KEY_C:
            cur_trans_mode = ViewCenter;
            break;
        case GLFW_KEY_U:
            cur_trans_mode = ViewUp;
            break;
        case GLFW_KEY_I:
//            cout << "---------------------------------------------------" << endl;
//            cout << "Using manual : " << endl;
//            cout << "    z : move to previous model" << endl;
//            cout << "    x : move to next model" << endl;
//            cout << "    o : switch to Orthogonal" << endl;
//            cout << "    p : switch to Perspective" << endl;
//            cout << "    s : GeoScaling" << endl;
//            cout << "    t : GeoTranslation" << endl;
//            cout << "    r : GeoRotation" << endl;
//            cout << "    e : ViewEye" << endl;
//            cout << "    c : ViewCenter" << endl;
//            cout << "    u : ViewUp" << endl;
//            cout << "----------------------------------------------------" << endl;
//			cout << "Viewing Transformation: " << endl;
//			printf("Camera Eye position :(x, y, z): (%f, %f, %f) \n", main_camera.position.x, main_camera.position.y, main_camera.position.z);
//			printf("Camera Center position :(x, y, z): (%f, %f, %f) \n", main_camera.center.x, main_camera.center.y, main_camera.center.z);
//			printf("Camera UP position :(x, y, z): (%f, %f, %f) \n", main_camera.up_vector.x, main_camera.up_vector.y, main_camera.up_vector.z);
//			cout << "----------------------------------------------------" << endl;
//			cout << "Geometrical Transformation: " << endl;
//			printf("Model Translatiion :(x, y, z): (%f, %f, %f) \n", models[cur_idx].position.x, models[cur_idx].position.y, models[cur_idx].position.z);
//			printf("Model Scale :(x, y, z): (%f, %f, %f) \n", models[cur_idx].scale.x, models[cur_idx].scale.y, models[cur_idx].scale.z);
//			printf("Model Rotation :(x, y, z): (%f, %f, %f) \n", models[cur_idx].rotation.x, models[cur_idx].rotation.y, models[cur_idx].rotation.z);
//            cout << "----------------------------------------------------" << endl;

            PrintAllMatrix();
            break;
        }
        printf("\n");
    }
}
GLfloat arc = PI / 2;
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// [TODO] mouse press callback function
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		mouse_pressed = true;
	}
	else {
		mouse_pressed = false;
	}
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	// [TODO] cursor position callback function
	if (!mouse_pressed) {
		starting_press_x = xpos;
		starting_press_y = ypos;
	}
	else {
		int diff_x = xpos - starting_press_x;
		int diff_y = ypos - starting_press_y;
		starting_press_x = xpos;
		starting_press_y = ypos;
		switch (cur_trans_mode) {
		case ViewEye:
			main_camera.position.x += (0.01 * -diff_x );
			main_camera.position.y += (0.01 * diff_y );
			setViewingMatrix();
			printf("Camera Eye position :(x, y, z): (%f, %f, %f) \n", main_camera.position.x, main_camera.position.y, main_camera.position.z);
			break;
		case ViewCenter:
			main_camera.center.x += (0.001 * -diff_x);
			main_camera.center.y += (0.001 * diff_y);
			setViewingMatrix();
			printf("Camera Center position :(x, y, z): (%f, %f, %f) \n", main_camera.center.x, main_camera.center.y, main_camera.center.z);
			break;
		case ViewUp:
			main_camera.up_vector.x += (0.001 * -diff_x);
			main_camera.up_vector.y += (0.001 * diff_y);
			setViewingMatrix();
			printf("Camera UP position :(x, y, z): (%f, %f, %f) \n", main_camera.up_vector.x, main_camera.up_vector.y, main_camera.up_vector.z);
			break;
		case GeoTranslation:
			models[cur_idx].position.x += (0.01 * diff_x);
			models[cur_idx].position.y += (0.01 * -diff_y);
			printf("Model Translatiion :(x, y, z): (%f, %f, %f) \n", models[cur_idx].position.x, models[cur_idx].position.y, models[cur_idx].position.z);
			break;
		case GeoScaling:
			models[cur_idx].scale.x += (0.01 * diff_x);
			models[cur_idx].scale.y += (0.01 * -diff_y);
			printf("Model Scale :(x, y, z): (%f, %f, %f) \n", models[cur_idx].scale.x, models[cur_idx].scale.y, models[cur_idx].scale.z);
			break;
		case GeoRotation:
			models[cur_idx].rotation.y += (0.01 * diff_x);//y-axis!!
			models[cur_idx].rotation.x += (0.01 * diff_y);//x-axis!!
			printf("Model Rotation :(x, y, z): (%f, %f, %f) \n", models[cur_idx].rotation.x, models[cur_idx].rotation.y, models[cur_idx].rotation.z);
			break;
		}
	}
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    // [TODO] scroll up positive, otherwise it would be negtive
    if(yoffset > 0){
        switch (cur_trans_mode){
        case ViewEye:
			main_camera.position.z += 0.1;
            setViewingMatrix();
			printf("Camera Eye position :(x, y, z): (%f, %f, %f) \n", main_camera.position.x, main_camera.position.y, main_camera.position.z);
            break;
        case ViewCenter:
            main_camera.center.z += 0.1;
            setViewingMatrix();
			printf("Camera Center position :(x, y, z): (%f, %f, %f) \n", main_camera.center.x, main_camera.center.y, main_camera.center.z);
            break;
        case ViewUp:
			main_camera.up_vector.z += 0.1;
			setViewingMatrix();
			printf("Camera UP position :(x, y, z): (%f, %f, %f) \n", main_camera.up_vector.x, main_camera.up_vector.y, main_camera.up_vector.z);
            break;
        case GeoTranslation:
//            models[cur_idx].position.z -= (models[cur_idx].position.z >= -10) ? 0.01 : 0;
			models[cur_idx].position.z +=0.1;
			printf("Model Translatiion :(x, y, z): (%f, %f, %f) \n", models[cur_idx].position.x, models[cur_idx].position.y, models[cur_idx].position.z);
            break;
        case GeoScaling:
            models[cur_idx].scale.z += 0.1;
			printf("Model Scale :(x, y, z): (%f, %f, %f) \n", models[cur_idx].scale.x, models[cur_idx].scale.y, models[cur_idx].scale.z);
            break;
        case GeoRotation:
            models[cur_idx].rotation.z += 0.1;
			printf("Model Rotation :(x, y, z): (%f, %f, %f) \n", models[cur_idx].rotation.x, models[cur_idx].rotation.y, models[cur_idx].rotation.z);
            break;
        }
    }
    else if(yoffset<0){
        switch (cur_trans_mode){
        case ViewEye:
            main_camera.position.z -= 0.1;
            setViewingMatrix();
			printf("Camera Eye position :(x, y, z): (%f, %f, %f) \n", main_camera.position.x, main_camera.position.y, main_camera.position.z);
            break;
        case ViewCenter:
            main_camera.center.z -= 0.1;
            setViewingMatrix();
			printf("Camera Center position :(x, y, z): (%f, %f, %f) \n", main_camera.center.x, main_camera.center.y, main_camera.center.z);
            break;
        case ViewUp:
			main_camera.up_vector.z -= 0.1;
			setViewingMatrix();
			printf("Camera UP position :(x, y, z): (%f, %f, %f) \n", main_camera.up_vector.x, main_camera.up_vector.y, main_camera.up_vector.z);
            break;
        case GeoTranslation:
//            models[cur_idx].position.z += (models[cur_idx].position.z <= 1) ? 0.01 : 0;
            models[cur_idx].position.z -= 0.1;
			printf("Model Translatiion :(x, y, z): (%f, %f, %f) \n", models[cur_idx].position.x, models[cur_idx].position.y, models[cur_idx].position.z);
            break;
        case GeoScaling:
            models[cur_idx].scale.z -= 0.1;
			printf("Model Scale :(x, y, z): (%f, %f, %f) \n", models[cur_idx].scale.x, models[cur_idx].scale.y, models[cur_idx].scale.z);
            break;
        case GeoRotation:
            models[cur_idx].rotation.z -= 0.1;
			printf("Model Rotation :(x, y, z): (%f, %f, %f) \n", models[cur_idx].rotation.x, models[cur_idx].rotation.y, models[cur_idx].rotation.z);
            break;
                
        }
    }
     
}
     
void setShaders()
{
    GLuint v, f, p;
    char *vs = NULL;
    char *fs = NULL;

    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);

    vs = textFileRead("shader.vs");
    fs = textFileRead("shader.fs");

    glShaderSource(v, 1, (const GLchar**)&vs, NULL);
    glShaderSource(f, 1, (const GLchar**)&fs, NULL);

    free(vs);
    free(fs);

    GLint success;
    char infoLog[1000];
    // compile vertex shader
    glCompileShader(v);
    // check for shader compile errors
    glGetShaderiv(v, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(v, 1000, NULL, infoLog);
        std::cout << "ERROR: VERTEX SHADER COMPILATION FAILED\n" << infoLog << std::endl;
    }

    // compile fragment shader 
    glCompileShader(f);
    // check for shader compile errors
    glGetShaderiv(f, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(f, 1000, NULL, infoLog);
        std::cout << "ERROR: FRAGMENT SHADER COMPILATION FAILED\n" << infoLog << std::endl;
    }

    // create program object
    p = glCreateProgram();

    // attach shaders to program object
    glAttachShader(p,f);
    glAttachShader(p,v);

    // link program
    glLinkProgram(p);
    // check for linking errors
    glGetProgramiv(p, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(p, 1000, NULL, infoLog);
        std::cout << "ERROR: SHADER PROGRAM LINKING FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(v);
    glDeleteShader(f);

    iLocMVP = glGetUniformLocation(p, "mvp");

    if (success)
        glUseProgram(p);
    else
    {
        system("pause");
        exit(123);
    }
}

void normalization(tinyobj::attrib_t* attrib, vector<GLfloat>& vertices, vector<GLfloat>& colors, tinyobj::shape_t* shape)
{
    vector<float> xVector, yVector, zVector;
    float minX = 10000, maxX = -10000, minY = 10000, maxY = -10000, minZ = 10000, maxZ = -10000;

    // find out min and max value of X, Y and Z axis
    for (int i = 0; i < attrib->vertices.size(); i++)
    {
        //maxs = max(maxs, attrib->vertices.at(i));
        if (i % 3 == 0)
        {

            xVector.push_back(attrib->vertices.at(i));

            if (attrib->vertices.at(i) < minX)
            {
                minX = attrib->vertices.at(i);
            }

            if (attrib->vertices.at(i) > maxX)
            {
                maxX = attrib->vertices.at(i);
            }
            
        }
        else if (i % 3 == 1)
        {
            yVector.push_back(attrib->vertices.at(i));

            if (attrib->vertices.at(i) < minY)
            {
                minY = attrib->vertices.at(i);
            }

            if (attrib->vertices.at(i) > maxY)
            {
                maxY = attrib->vertices.at(i);
            }
        }
        else if (i % 3 == 2)
        {
            zVector.push_back(attrib->vertices.at(i));

            if (attrib->vertices.at(i) < minZ)
            {
                minZ = attrib->vertices.at(i);
            }

            if (attrib->vertices.at(i) > maxZ)
            {
                maxZ = attrib->vertices.at(i);
            }
        }
    }

    float offsetX = (maxX + minX) / 2;
    float offsetY = (maxY + minY) / 2;
    float offsetZ = (maxZ + minZ) / 2;

    for (int i = 0; i < attrib->vertices.size(); i++)
    {
        if (offsetX != 0 && i % 3 == 0)
        {
            attrib->vertices.at(i) = attrib->vertices.at(i) - offsetX;
        }
        else if (offsetY != 0 && i % 3 == 1)
        {
            attrib->vertices.at(i) = attrib->vertices.at(i) - offsetY;
        }
        else if (offsetZ != 0 && i % 3 == 2)
        {
            attrib->vertices.at(i) = attrib->vertices.at(i) - offsetZ;
        }
    }

    float greatestAxis = maxX - minX;
    float distanceOfYAxis = maxY - minY;
    float distanceOfZAxis = maxZ - minZ;

    if (distanceOfYAxis > greatestAxis){
        greatestAxis = distanceOfYAxis;
    }

    if (distanceOfZAxis > greatestAxis){
        greatestAxis = distanceOfZAxis;
    }

    float scale = greatestAxis / 2;

    for (int i = 0; i < attrib->vertices.size(); i++)
    {
        //std::cout << i << " = " << (double)(attrib.vertices.at(i) / greatestAxis) << std::endl;
        attrib->vertices.at(i) = attrib->vertices.at(i)/ scale;
    }
    size_t index_offset = 0;
    vertices.reserve(shape->mesh.num_face_vertices.size() * 3);
    colors.reserve(shape->mesh.num_face_vertices.size() * 3);
    for (size_t f = 0; f < shape->mesh.num_face_vertices.size(); f++) {
        int fv = shape->mesh.num_face_vertices[f];

        // Loop over vertices in the face.
        for (size_t v = 0; v < fv; v++) {
            // access to vertex
            tinyobj::index_t idx = shape->mesh.indices[index_offset + v];
            vertices.push_back(attrib->vertices[3 * idx.vertex_index + 0]);
            vertices.push_back(attrib->vertices[3 * idx.vertex_index + 1]);
            vertices.push_back(attrib->vertices[3 * idx.vertex_index + 2]);
            // Optional: vertex colors
            colors.push_back(attrib->colors[3 * idx.vertex_index + 0]);
            colors.push_back(attrib->colors[3 * idx.vertex_index + 1]);
            colors.push_back(attrib->colors[3 * idx.vertex_index + 2]);
        }
        index_offset += fv;
    }
}

void LoadModels(string model_path)
{
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;
    tinyobj::attrib_t attrib;
    vector<GLfloat> vertices;
    vector<GLfloat> colors;

    string err;
    string warn;

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_path.c_str());

    if (!warn.empty()) {cout << warn << std::endl;}
    if (!err.empty()) {cerr << err << std::endl;}
    if (!ret) {exit(1);}

    printf("Load Models Success ! Shapes size %d Maerial size %d\n", shapes.size(), materials.size());
    
    normalization(&attrib, vertices, colors, &shapes[0]);

    Shape tmp_shape;
    glGenVertexArrays(1, &tmp_shape.vao);
    glBindVertexArray(tmp_shape.vao);

    glGenBuffers(1, &tmp_shape.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices.at(0), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    tmp_shape.vertex_count = vertices.size() / 3;

    glGenBuffers(1, &tmp_shape.p_color);
    glBindBuffer(GL_ARRAY_BUFFER, tmp_shape.p_color);
    glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(GL_FLOAT), &colors.at(0), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

    m_shape_list.push_back(tmp_shape);
    model tmp_model;
    models.push_back(tmp_model);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    shapes.clear();
    materials.clear();
    
}

void initParameter()
{
    proj.left = -1.5;
    proj.right = 1.5;
    proj.top = 1.5;
    proj.bottom = -1.5;
    proj.nearClip = 1;
    proj.farClip = 10.0;
    proj.fovy = 80;
    proj.aspect = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

    main_camera.position = Vector3(0.0f, 0.0f, 2.0f);
    main_camera.center = Vector3(0.0f, 0.0f, 0.0f);
    main_camera.up_vector = Vector3(0.0f, 1.0f, 0.0f);

    setViewingMatrix();
    setPerspective();    //set default projection matrix as perspective matrix
}

void setupRC()
{
    // setup shaders
    setShaders();
    initParameter();

    // OpenGL States and Values
    glClearColor(0.2, 0.2, 0.2, 1.0);
    vector<string> model_list{ "../ColorModels/bunny5KC.obj", "../ColorModels/dragon10KC.obj", "../ColorModels/lucy25KC.obj", "../ColorModels/teapot4KC.obj", "../ColorModels/dolphinC.obj"};
    // [TODO] Load five model at here
    for(int i=cur_idx;i<5;i++){
        LoadModels(model_list[i]);
    }
}

void glPrintContextInfo(bool printExtension)
{
    cout << "GL_VENDOR = " << (const char*)glGetString(GL_VENDOR) << endl;
    cout << "GL_RENDERER = " << (const char*)glGetString(GL_RENDERER) << endl;
    cout << "GL_VERSION = " << (const char*)glGetString(GL_VERSION) << endl;
    cout << "GL_SHADING_LANGUAGE_VERSION = " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
    if (printExtension)
    {
        GLint numExt;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExt);
        cout << "GL_EXTENSIONS =" << endl;
        for (GLint i = 0; i < numExt; i++)
        {
            cout << "\t" << (const char*)glGetStringi(GL_EXTENSIONS, i) << endl;
        }
    }
}


int main(int argc, char **argv)
{
    // initial glfw
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // fix compilation on OS X
#endif

    
    // create window
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "106034061_HW1", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    
    // load OpenGL function pointer
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // register glfw callback functions
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetFramebufferSizeCallback(window, ChangeSize);
    glEnable(GL_DEPTH_TEST);
    // Setup render context
    setupRC();
    // main loop
    while (!glfwWindowShouldClose(window))
    {
        // render
        RenderScene();
        // swap buffer from back to front
        glfwSwapBuffers(window);
        // Poll input event
        glfwPollEvents();
    }
    // just for compatibiliy purposes
    return 0;
}

