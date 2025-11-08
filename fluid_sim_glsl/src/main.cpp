// main.cpp - LBM Fluid Simulation with GLSL
#include <flgl.h>
#include <flgl/tools.h>
#include <flgl/geometry.h>
#include <flgl/allocators.h>
#include <vector>
#include <array>
#include <cmath>
#include <iostream>

// Simulation parameters
constexpr int NX = 380;
constexpr int NY = 300;
constexpr float TAU = 1.0f;
constexpr int STEPS_PER_FRAME = 5;

// D2Q9 lattice constants
constexpr float WEIGHTS[9] = {
    1.0f/36.0f, 1.0f/9.0f, 1.0f/36.0f,
    1.0f/9.0f, 4.0f/9.0f, 1.0f/9.0f,
    1.0f/36.0f, 1.0f/9.0f, 1.0f/36.0f
};

class LBMFluidSimulation {
private:
    // Screen quad for rendering
    Mesh<Vt_2Dclassic> screenQuad;
    
    // Shaders for each step
    Shader initShader;
    Shader collisionShader;
    Shader streamingShader;
    Shader boundaryShader;
    Shader mouseShader;
    Shader macroscopicShader;
    Shader visualizationShader;
    
    // Framebuffers and textures
    GLuint fbo[2];
    GLuint distributionTextures[2][2];
    GLuint densityTexture;
    GLuint velocityTexture;
    GLuint macroscopicFBO;
    
    // Mouse interaction
    float mouseX = -1.0f;
    float mouseY = -1.0f;
    float prevMouseX = -1.0f;
    float prevMouseY = -1.0f;
    bool mousePressed = false;
    
    // Simulation state
    bool pingPong = true;
    
    // Window handle
    GLFWwindow* windowHandle = nullptr;
    
    // Screen quad vertices
    std::vector<Vt_2Dclassic> quadVertices = {
        {{-1.0f,  1.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 1.0f}}
    };
    
    std::vector<uint32_t> quadIndices = {
        0, 1, 2,
        0, 2, 3
    };

    // Helper to set uniforms using raw OpenGL
    void setUniform1i(Shader& shader, const char* name, int value) {
        GLint loc = glGetUniformLocation(shader.program, name);
        if (loc >= 0) glUniform1i(loc, value);
    }
    
    void setUniform1f(Shader& shader, const char* name, float value) {
        GLint loc = glGetUniformLocation(shader.program, name);
        if (loc >= 0) glUniform1f(loc, value);
    }
    
    void setUniform2f(Shader& shader, const char* name, float v1, float v2) {
        GLint loc = glGetUniformLocation(shader.program, name);
        if (loc >= 0) glUniform2f(loc, v1, v2);
    }
    
    void setUniform1fv(Shader& shader, const char* name, int count, const float* values) {
        GLint loc = glGetUniformLocation(shader.program, name);
        if (loc >= 0) glUniform1fv(loc, count, values);
    }

public:
    void initialize() {
        // Store window handle
        windowHandle = glfwGetCurrentContext();
        
        // Create screen quad mesh
        screenQuad = Mesh<Vt_2Dclassic>::from_vectors(quadVertices, quadIndices);
        
        // Set shader path
        glconfig.set_shader_path("./shaders/");
        
        // Load all shaders
        initShader.create("lbm_init", "lbm_init");
        collisionShader.create("lbm_collision", "lbm_collision");
        streamingShader.create("lbm_streaming", "lbm_streaming");
        boundaryShader.create("lbm_boundary", "lbm_boundary");
        mouseShader.create("lbm_mouse", "lbm_mouse");
        macroscopicShader.create("lbm_macroscopic", "lbm_macroscopic");
        visualizationShader.create("visualization", "visualization");
        
        // Create textures
        createTextures();
        
        // Create framebuffers
        createFramebuffers();
        
        // Initialize distribution functions to equilibrium
        runInitialization();
        
        std::cout << "LBM Simulation initialized: " << NX << "x" << NY << " grid" << std::endl;
    }
    
    void createTextures() {
        for (int i = 0; i < 2; i++) {
            for (int j = 0; j < 2; j++) {
                glGenTextures(1, &distributionTextures[i][j]);
                glBindTexture(GL_TEXTURE_2D, distributionTextures[i][j]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, NX, NY, 0, GL_RGBA, GL_FLOAT, nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            }
        }
        
        glGenTextures(1, &densityTexture);
        glBindTexture(GL_TEXTURE_2D, densityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, NX, NY, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        
        glGenTextures(1, &velocityTexture);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, NX, NY, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    
    void createFramebuffers() {
        for (int i = 0; i < 2; i++) {
            glGenFramebuffers(1, &fbo[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo[i]);
            
            GLenum drawBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, distributionTextures[i][0], 0);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, distributionTextures[i][1], 0);
            glDrawBuffers(2, drawBuffers);
            
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "Framebuffer " << i << " is not complete!" << std::endl;
            }
        }
        
        glGenFramebuffers(1, &macroscopicFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, macroscopicFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, densityTexture, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, velocityTexture, 0);
        GLenum macroBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, macroBuffers);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void runInitialization() {
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[0]);
        
        initShader.bind();
        setUniform1f(initShader, "tau", TAU);
        
        float weights[9];
        for (int i = 0; i < 9; i++) weights[i] = WEIGHTS[i];
        setUniform1fv(initShader, "weights", 9, weights);
        
        gl.draw_mesh(screenQuad);
        
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo[0]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo[1]);
        glBlitFramebuffer(0, 0, NX, NY, 0, 0, NX, NY, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    
    void runCollisionStep() {
        int src = pingPong ? 0 : 1;
        int dst = pingPong ? 1 : 0;
        
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[dst]);
        
        collisionShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, distributionTextures[src][0]);
        setUniform1i(collisionShader, "distTex0", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, distributionTextures[src][1]);
        setUniform1i(collisionShader, "distTex1", 1);
        
        setUniform1f(collisionShader, "tau", TAU);
        setUniform2f(collisionShader, "gridSize", float(NX), float(NY));
        
        gl.draw_mesh(screenQuad);
    }
    
    void runStreamingStep() {
        int src = pingPong ? 1 : 0;
        int dst = pingPong ? 0 : 1;
        
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[dst]);
        
        streamingShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, distributionTextures[src][0]);
        setUniform1i(streamingShader, "distTex0", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, distributionTextures[src][1]);
        setUniform1i(streamingShader, "distTex1", 1);
        
        setUniform2f(streamingShader, "gridSize", float(NX), float(NY));
        
        gl.draw_mesh(screenQuad);
        
        pingPong = !pingPong;
    }
    
    void runBoundaryConditions() {
        int current = pingPong ? 0 : 1;
        
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[current]);
        
        boundaryShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, distributionTextures[current][0]);
        setUniform1i(boundaryShader, "distTex0", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, distributionTextures[current][1]);
        setUniform1i(boundaryShader, "distTex1", 1);
        
        setUniform2f(boundaryShader, "gridSize", float(NX), float(NY));
        setUniform1f(boundaryShader, "wallDamping", 0.7f);
        
        gl.draw_mesh(screenQuad);
    }
    
    void applyMouseForce() {
        if (!mousePressed) return;
        
        int current = pingPong ? 0 : 1;
        
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[current]);
        
        mouseShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, distributionTextures[current][0]);
        setUniform1i(mouseShader, "distTex0", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, distributionTextures[current][1]);
        setUniform1i(mouseShader, "distTex1", 1);
        
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, densityTexture);
        setUniform1i(mouseShader, "densityTex", 2);
        
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        setUniform1i(mouseShader, "velocityTex", 3);
        
        setUniform2f(mouseShader, "mousePos", mouseX, mouseY);
        setUniform2f(mouseShader, "prevMousePos", prevMouseX, prevMouseY);
        setUniform1f(mouseShader, "forceStrength", 0.1f);
        setUniform1f(mouseShader, "forceRadius", 30.0f);
        setUniform2f(mouseShader, "gridSize", float(NX), float(NY));
        
        gl.draw_mesh(screenQuad);
    }
    
    void computeMacroscopic() {
        int current = pingPong ? 0 : 1;
        
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, macroscopicFBO);
        
        macroscopicShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, distributionTextures[current][0]);
        setUniform1i(macroscopicShader, "distTex0", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, distributionTextures[current][1]);
        setUniform1i(macroscopicShader, "distTex1", 1);
        
        gl.draw_mesh(screenQuad);
    }
    
    void visualize() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, window.width, window.height);
        
        visualizationShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, densityTexture);
        setUniform1i(visualizationShader, "densityTex", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        setUniform1i(visualizationShader, "velocityTex", 1);
        
        setUniform2f(visualizationShader, "screenSize", float(window.width), float(window.height));
        setUniform2f(visualizationShader, "gridSize", float(NX), float(NY));
        
        gl.draw_mesh(screenQuad);
    }
    
    void update() {
        for (int step = 0; step < STEPS_PER_FRAME; step++) {
            runCollisionStep();
            runStreamingStep();
            runBoundaryConditions();
            applyMouseForce();
            computeMacroscopic();
        }
        
        visualize();
    }
    
    void handleMouseInput() {
        double mx, my;
        glfwGetCursorPos(windowHandle, &mx, &my);
        
        prevMouseX = mouseX;
        prevMouseY = mouseY;
        mouseX = (float(mx) / window.width) * NX;
        mouseY = (1.0f - float(my) / window.height) * NY;
        
        mousePressed = (glfwGetMouseButton(windowHandle, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
        
        if (!mousePressed) {
            prevMouseX = -1.0f;
            prevMouseY = -1.0f;
        }
    }
    
    void cleanup() {
        glDeleteFramebuffers(2, fbo);
        glDeleteFramebuffers(1, &macroscopicFBO);
        
        for (int i = 0; i < 2; i++) {
            glDeleteTextures(2, distributionTextures[i]);
        }
        glDeleteTextures(1, &densityTexture);
        glDeleteTextures(1, &velocityTexture);
    }
};

int main() {
    gl.init();
    window.create("LBM Fluid Simulation", 1280, 720);
    
    LBMFluidSimulation simulation;
    simulation.initialize();
    
    gl.set_clear_color(0.1f, 0.1f, 0.1f, 1.0f);
    
    while (!window.should_close()) {
        gl.clear(GL_COLOR_BUFFER_BIT);
        
        simulation.handleMouseInput();
        simulation.update();
        
        window.update();
    }
    
    simulation.cleanup();
    gl.destroy();
    
    return 0;
}
