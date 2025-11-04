#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>

int main(){

    std::cout << "S1: Blue Window\n";
    //init glfw
    if(!glfwInit()){
        std::cerr << "Couldn't init glfw.\n";
        return -1;
    }

    std::cout << "GLFW initialized.\n";


    //opengl core profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);      //for updated features

    //create window.
    GLFWwindow* window = glfwCreateWindow(1200, 720, "S1: Blue window.\n", NULL, NULL);
    if(!window){
        std::cout << "window creation failed.\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "window created successfully.\n";

    glfwMakeContextCurrent(window);

    //init glew
    glewExperimantal = GL_TRUE;
    if(glewInit() != GLEW_OK){
        std::cerr << "Could'nt init GLEW.\n";
        return -1;
    }
    std::cout << "GLEW init Successful.\n";
    
    //print opengl info
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << "\n";
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << "\n\n";

    //main loop
        
        //handle input

        //clear screen to blue

        //swap buffers

        //poll events

    //cleanup


}
