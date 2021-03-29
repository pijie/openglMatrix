#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window_jni.h>
#include <GLES3/gl3.h>

#include <EGL/egl.h>

#include <ctime>
// 矩阵变换相关类
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"

#define JAVA_CLASS "com/cci/openglmatrix/GlMatrixRenderer"

jlong drawMatrix(JNIEnv *env, [[maybe_unused]] jclass clazz, jobject surface, jstring path,
                 jstring path1);

const static JNINativeMethod Methods[] = {
        {"nativeMatrix", "(Landroid/view/Surface;Ljava/lang/String;Ljava/lang/String;)V", reinterpret_cast<void *>(drawMatrix)}
};

extern "C" {
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, [[maybe_unused]] void *reserved) {
    JNIEnv *env;
    if (jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    env->RegisterNatives(env->FindClass(JAVA_CLASS), Methods,
                         sizeof(Methods) / sizeof(Methods[0]));
    return JNI_VERSION_1_6;
}
}


namespace {
    // 上下颠倒 调整y轴坐标就行
    const char *VERTEX_SHADER_SRC = R"SRC(
        #version 300 es
        // 顶点位置坐标
        layout (location = 0) in vec3 aPos;
        // 纹理坐标
        layout (location = 1) in vec2 aTexCoord;
        // 纹理坐标传递给片元着色器，
        out vec2 TexCoord;
        // 顶点位置矩阵变化
        uniform mat4 transform;

        void main(){
            gl_Position =transform * vec4(aPos,1.0);
            TexCoord = aTexCoord;
        }

    )SRC";

    const char *FRAGMENT_SHADER_SRC = R"SRC(
        #version 300 es
        // 片元着色器输出
        out vec4 FragColor;
        // 纹理坐标输入
        in vec2 TexCoord;
        // 2d纹理采样器1
        uniform sampler2D texture1;
        // 2d纹理采样器2
        uniform sampler2D texture2;

        void main(){
            // 纹理混合    texture1 :80%, texture2 : 20%
            FragColor = mix(texture(texture1,TexCoord), texture(texture2,TexCoord),0.2);
        }
    )SRC";

}

jlong drawMatrix(JNIEnv *env, [[maybe_unused]] jclass clazz, jobject surface, jstring path,
                 jstring path1) {
    // 获取当前android平台的窗口
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    // 创建显示设备
    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    // 创建eglsurface
    EGLConfig config;
    EGLint configNum;
    EGLint configAttrs[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_NONE
    };
    eglChooseConfig(eglDisplay, configAttrs, &config, 1, &configNum);
    assert(configNum > 0);
    // 创建surface
    EGLSurface eglSurface = eglCreateWindowSurface(eglDisplay, config, nativeWindow, nullptr);
    // 创建上下文
    const EGLint ctxAttr[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    EGLContext eglContext = eglCreateContext(eglDisplay, config, EGL_NO_CONTEXT, ctxAttr);
    eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    // 颜色缓存区，深度缓存区，模板缓存区
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glViewport(0, 0, ANativeWindow_getWidth(nativeWindow),
               ANativeWindow_getHeight(nativeWindow) );

    const char *imageTexturePath = env->GetStringUTFChars(path, nullptr);
    const char *imageTexturePath1 = env->GetStringUTFChars(path1, nullptr);

    float vertices[] = {
            // positions          // texture coords
            0.5f, 0.5f, 0.0f, 1.0f, 1.0f, // top right
            0.5f, -0.5f, 0.0f, 1.0f, 0.0f, // bottom right
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, // bottom left
            -0.5f, 0.5f, 0.0f, 0.0f, 1.0f  // top left
    };
    unsigned int indices[] = {
            0, 1, 3, // first triangle
            1, 2, 3  // second triangle
    };

    GLuint VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // 向显存中批量传递数据
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 向显存中批量传递数据
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 设置顶点位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), nullptr);
    // 使能 顶点坐标属性数组
    glEnableVertexAttribArray(0);
    // 设置纹理坐标属性
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void *) (3 * sizeof(float)));
    // 使能 纹理坐标属性数组
    glEnableVertexAttribArray(1);


    GLuint texture1, texture2;
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, nrChannel;
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(imageTexturePath, &width, &height, &nrChannel, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    data = stbi_load(imageTexturePath1, &width, &height, &nrChannel, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &VERTEX_SHADER_SRC, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &FRAGMENT_SHADER_SRC, nullptr);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);
    // 使用纹理0;
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);
    // 使用纹理1；
    glUniform1i(glGetUniformLocation(shaderProgram, "texture2"), 1);

//    while (true){
    // 激活纹理0
    glActiveTexture(GL_TEXTURE0);
    // 纹理0 绑定 texture1;
    glBindTexture(GL_TEXTURE_2D, texture1);
    // 激活纹理1
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);

    // 初始化一个单位矩阵
    glm::mat4 transform = glm::mat4(1.0f);
    // 将单位举证 沿着x轴正方向移动0.5， 沿着y轴负方向移动0.5; 得到平移后的变换矩阵
//        transform = glm::translate(transform,glm::vec3(0.5f,-0.5f,0.0f));
//        // 沿着z轴旋转
//        transform = glm::rotate(transform,glm::radians(-90.0f),glm::vec3(0.0f,0.0f,1.0f));

    // 沿着z轴旋转

    transform = glm::rotate(transform, glm::radians(-0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::translate(transform, glm::vec3(0.5f, -0.5f, 0.0f));

    glUseProgram(shaderProgram);
    GLuint transformLoc = glGetUniformLocation(shaderProgram, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transform));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


    transform = glm::mat4(1.0f); // reset it to identity matrix
    transform = glm::translate(transform, glm::vec3(-0.5f, 0.5f, 0.0f));

    float pi = 3.1415926;
    float scaleAmount = sin(pi / 4);
    __android_log_print(ANDROID_LOG_INFO,"matrix","scaleAmount: %f",scaleAmount);

    transform = glm::scale(transform, glm::vec3(scaleAmount, scaleAmount, scaleAmount));
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, &transform[0][0]); // this time take the matrix value array's first element as its memory pointer value

    // now with the uniform matrix being replaced with new transformations, draw it again.
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    eglSwapBuffers(eglDisplay, eglSurface);
//    }





    return 0;
}


