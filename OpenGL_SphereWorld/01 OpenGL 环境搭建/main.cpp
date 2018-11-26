#include "GLTools.h"
#include "GLShaderManager.h"
#include "GLMatrixStack.h"
#include "GLFrustum.h"
#include "GLBatch.h"
#include "GLGeometryTransform.h"
#include "StopWatch.h"

#include <math.h>
#include <stdio.h>

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

#define NUM_SPHERES 50
GLFrame spheres[NUM_SPHERES];

GLShaderManager shaderManager;
GLGeometryTransform transformPipeline;
GLMatrixStack modelViewMatrix;
GLMatrixStack projectionMatrix;
GLFrame cameraFrame;
GLFrustum viewFrustum;
GLTriangleBatch torusBatch;
GLTriangleBatch sphereBatch;
GLBatch floorBatch;

void ChangeSize(int w, int h)
{
    //防止h为0
    if (h == 0) {
        h = 1;
    }
    //绘制窗口
    glViewport(0, 0, w, h);
    
    //设置透视投影矩阵：观察角度、宽高比、近距离、远距离
    viewFrustum.SetPerspective(35.0f, float(w)/float(h), 1.0f, 500.0f);
    //获取透视投影矩阵并载入透视投影矩阵堆栈中
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
}

void SpecialKeys(int key, int x, int y)
{
    //Z轴方向移动步长
    float linar = 0.1f;
    //围绕y轴旋转角度
    float angular = float(m3dDegToRad(5.0f));
    
    //上下键：z轴移动；左右键：围绕y轴旋转
    if (key == GLUT_KEY_UP) {
        cameraFrame.MoveForward(linar);
    }
    
    if (key == GLUT_KEY_DOWN) {
        cameraFrame.MoveForward(-linar);
    }
    
    if (key == GLUT_KEY_LEFT) {
        cameraFrame.RotateWorld(angular, 0.0f, 1.0f, 0.0f);
    }
    
    if (key == GLUT_KEY_RIGHT) {
        cameraFrame.RotateWorld(-angular, 0.0f, 1.0f, 0.0f);
    }
    
    //RenderScene()方法中已经做了窗口实时刷新处理，此处无须再做
}

void RenderScene()
{
    //清楚缓存区：颜色缓存区、深度缓存区、模版缓存区
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
    
    //设置颜色：地板、甜甜圈、球体
    GLfloat vFloorColor[] = {0.0f, 1.0f, 0.0f, 1.0f};
    GLfloat vTorusColor[] = {1.0f, 0.0f, 0.0f, 1.0f};
    GLfloat vSphereColor[] = {0.0f, 0.0f, 1.0f, 1.0f};
    
    //基于当前时间动画：当前时间*60s
    static CStopWatch rotTime;
    float yRot = rotTime.GetElapsedSeconds()*60.0f;
    
    //获取观察者矩阵并入栈
    M3DMatrix44f mCamera;
    cameraFrame.GetCameraMatrix(mCamera);
    modelViewMatrix.PushMatrix(mCamera);
    
    //设置光源矩阵
    M3DVector4f vLightPos = {0.0f, 10.0f, 5.0f, 1.0f};
    M3DVector4f vLightEyePos;
    //将光源矩阵和观察者矩阵相乘的结果放在vLightEyePos中
    m3dTransformVector4(vLightEyePos, vLightPos, mCamera);
    
    //使用管线控制器，平面着色器进行渲染
    shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(), vFloorColor);
    floorBatch.Draw();
    
    //在模型视图矩阵堆栈中绘制以下图形：先压栈，绘制完毕后再出栈——始终对栈顶矩阵图形渲染
    
    //绘制随机球体
    for (int i = 0; i < NUM_SPHERES; i++) {
        modelViewMatrix.PushMatrix();
        //模型视图矩阵堆栈栈顶矩阵与随机球体矩阵相乘的结果放入栈顶
        modelViewMatrix.MultMatrix(spheres[i]);
        shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transformPipeline.GetModelViewMatrix(), transformPipeline.GetProjectionMatrix(), vLightEyePos, vSphereColor);
        //可与公转球体使用同一个容器类对象
        sphereBatch.Draw();
        
        modelViewMatrix.PopMatrix();
    }
    
    //设置甜甜圈平移距离（z轴负方向2.5）和旋转角度；
    modelViewMatrix.Translate(0.0f, 0.0f, -2.5f);
    modelViewMatrix.PushMatrix();
    modelViewMatrix.Rotate(yRot, 0.0f, 0.1f, 0.0f);
    //使用点光源着色器渲染
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF, transformPipeline.GetModelViewMatrix(), transformPipeline.GetProjectionMatrix(), vLightEyePos, vTorusColor);
    torusBatch.Draw();
    modelViewMatrix.PopMatrix();
    
    //设置公转球体：反方向旋转，在x轴上平移0.8
    modelViewMatrix.Rotate(yRot *-2.0f, 0.0f, 1.0f, 0.0f);
    modelViewMatrix.Translate(0.8f, 0.0f, 0.0f);
    shaderManager.UseStockShader(GLT_SHADER_FLAT, transformPipeline.GetModelViewProjectionMatrix(), vSphereColor);
    sphereBatch.Draw();
    
    modelViewMatrix.PopMatrix();
    
    //后台渲染完成后交给前台
    glutSwapBuffers();
    //基于时间动画：实时刷新窗口
    glutPostRedisplay();
}

void SetupRC()
{
    //设置窗口背景颜色
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //初始化管线控制器
    shaderManager.InitializeStockShaders();
    //开启深度测试：图形间重叠部分无须重复绘制
    glEnable(GL_DEPTH_TEST);

    //提交甜甜圈数据：三角形批次类对象、外圈半径、内圈半径、主半径三角形对数x、小半径三角形对数y(尽量：x=2*y,更圆滑)
    gltMakeTorus(torusBatch, 0.4f, 0.15f, 30, 15);
    //提交公转球体数据：三角形批次类对象、半径、底部到顶部三角形带的数量、一条三角形带中的三角形对数（一般指球体中间那条，为最大数）
    gltMakeSphere(sphereBatch, 0.1f, 26, 20);

    //线段模式，325个顶点
    floorBatch.Begin(GL_LINES, 325);
    for (GLfloat x = -20.0f; x <= 20.0f; x+=0.5f) {
        /*
         1.一个格子四个顶点，格子方向朝屏幕里面；
         2.只绘制x和z轴方向上的顶点；
         3.y轴坐标保持不变，负值，展示随机球体悬浮效果；
         */
        floorBatch.Vertex3f(x, -0.55f, 20.0f);
        floorBatch.Vertex3f(x, -0.55f, -20.0f);
        floorBatch.Vertex3f(20.0f, -0.55f, x);
        floorBatch.Vertex3f(-20.0f, -0.55f, x);
    }
    floorBatch.End();

    //随机悬浮球体：y值不变，x、z值随机
    for (int i = 0; i < NUM_SPHERES; i++) {
        GLfloat x = (GLfloat)(((rand()%400)-200)*0.1f);
        GLfloat z = (GLfloat)(((rand()%400)-200)*0.1f);
        spheres[i].SetOrigin(x, 0.0f, z);
    }
}

int main(int argc, char* argv[])
{
    //创建工作目录
    gltSetWorkingDirectory(argv[0]);
    //初始化GLUT库
    glutInit(&argc, argv);
    //设置显示模式：双缓存、颜色缓存、深度缓存
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    //配置显示窗口
    glutInitWindowSize(800, 600);
    glutCreateWindow("Sphere World");

    //自适应窗口大小
    glutReshapeFunc(ChangeSize);
    //方位键控制
    glutSpecialFunc(SpecialKeys);
    //图形渲染
    glutDisplayFunc(RenderScene);

    //容错判断
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW Error:%s\n", glewGetErrorString(err));
        return 1;
    }

    //图形定位
    SetupRC();
    //事件循环
    glutMainLoop();

    return 0;
}
