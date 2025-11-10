#include <flgl.h>
#include <flgl/tools.h>
#include <flgl/geometry.h>
#include <shader_helper.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <iomanip>

class IncrementalLBM {
private:
    // Mesh for rendering
    Mesh<Vt_2Dclassic> screenQuad;
    
    // Shaders
    Shader initShader;
    Shader collisionShader;
    Shader streamingShader;  // ADD THIS
    Shader macroscopicShader;
    Shader displayShader;
    
    // LBM Distribution textures (9 distributions in 3 textures)
    GLuint distTextures[2][3];  // [ping-pong][texture index]
    
    // Macroscopic quantities
    GLuint densityTexture;
    GLuint velocityTexture;
    
    // Framebuffers
    GLuint distFBO[2];  // For distributions
    GLuint macroFBO;     // For macroscopic quantities
    
    // State
    bool pingPong = false;
    
    // Grid size
    static constexpr int NX = 256;
    static constexpr int NY = 256;
    
    // LBM parameters
    static constexpr float TAU = 1.0f;  // Changed back to 1.0 for stability
    
    // Debug mode
    bool debugMode = true;
    int frameCount = 0;
    
    // Quad vertices
    std::vector<Vt_2Dclassic> quadVertices = {
        {{-1.0f,  1.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 1.0f}}
    };
    
    std::vector<uint32_t> quadIndices = {0, 1, 2, 0, 2, 3};

    void checkGLError(const char* where) {
        GLenum err;
        bool hasError = false;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cerr << "OpenGL error at " << where << ": 0x" << std::hex << err << std::dec << std::endl;
            hasError = true;
        }
        if (!hasError && debugMode && frameCount < 3) {
            std::cout << "[OK] No GL errors at: " << where << std::endl;
        }
    }

public:
    void initialize() {
        std::cout << "\n=== Step 4: LBM with Collision & Streaming ===" << std::endl;
        std::cout << "Grid size: " << NX << "x" << NY << std::endl;
        std::cout << "Tau: " << TAU << std::endl;
        
        // Create screen quad
        screenQuad = Mesh<Vt_2Dclassic>::from_vectors(quadVertices, quadIndices);
        std::cout << "1. Screen quad created" << std::endl;
        
        // Create textures
        createDistributionTextures();
        createMacroscopicTextures();
        std::cout << "2. Textures created" << std::endl;
        
        // Create framebuffers
        createFramebuffers();
        std::cout << "3. Framebuffers created" << std::endl;
        
        // Load shaders
        initShader.create("lbm_init4", "lbm_init4_frag");
        collisionShader.create("lbm_collision4", "lbm_collision4_frag");
        streamingShader.create("lbm_streaming4", "lbm_streaming4_frag");  // ADD THIS
        macroscopicShader.create("lbm_macro4", "lbm_macro4_frag");
        displayShader.create("display4", "display4_frag");
        std::cout << "4. Shaders loaded" << std::endl;
        
        // Initialize distributions
        initializeLBM();
        std::cout << "5. LBM initialized" << std::endl;
        
        // Compute initial macroscopic quantities
        computeMacroscopic();
        debugCheckValues();
        
        std::cout << "=== Initialization Complete! ===\n" << std::endl;
    }
    
    void createDistributionTextures() {
        for (int p = 0; p < 2; p++) {
            for (int i = 0; i < 3; i++) {
                glGenTextures(1, &distTextures[p][i]);
                glBindTexture(GL_TEXTURE_2D, distTextures[p][i]);
                
                if (i < 2) {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, NX, NY, 0, 
                               GL_RGBA, GL_FLOAT, nullptr);
                } else {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, NX, NY, 0, 
                               GL_RED, GL_FLOAT, nullptr);
                }
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
        }
    }
    
    void createMacroscopicTextures() {
        glGenTextures(1, &densityTexture);
        glBindTexture(GL_TEXTURE_2D, densityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, NX, NY, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        glGenTextures(1, &velocityTexture);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, NX, NY, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    
    void createFramebuffers() {
        for (int i = 0; i < 2; i++) {
            glGenFramebuffers(1, &distFBO[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, distFBO[i]);
            
            GLenum drawBuffers[3] = {
                GL_COLOR_ATTACHMENT0,
                GL_COLOR_ATTACHMENT1,
                GL_COLOR_ATTACHMENT2
            };
            
            for (int j = 0; j < 3; j++) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + j,
                                      GL_TEXTURE_2D, distTextures[i][j], 0);
            }
            
            glDrawBuffers(3, drawBuffers);
            
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "Distribution framebuffer " << i << " incomplete!" << std::endl;
            }
        }
        
        glGenFramebuffers(1, &macroFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, macroFBO);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D, densityTexture, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                              GL_TEXTURE_2D, velocityTexture, 0);
        
        GLenum macroBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, macroBuffers);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void initializeLBM() {
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, distFBO[0]);
        
        initShader.bind();
        ShaderHelper::setUniform1f("tau", TAU);
        
        gl.draw_mesh(screenQuad);
        
        // Copy to second buffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, distFBO[0]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, distFBO[1]);
        glBlitFramebuffer(0, 0, NX, NY, 0, 0, NX, NY, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void runCollisionStep() {
        int src = pingPong ? 0 : 1;
        int dst = pingPong ? 1 : 0;
        
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, distFBO[dst]);
        
        collisionShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, distTextures[src][0]);
        ShaderHelper::setUniform1i("distTex0", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, distTextures[src][1]);
        ShaderHelper::setUniform1i("distTex1", 1);
        
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, distTextures[src][2]);
        ShaderHelper::setUniform1i("distTex2", 2);
        
        ShaderHelper::setUniform1f("tau", TAU);
        ShaderHelper::setUniform1f("time", frameCount * 0.01f);
        
        gl.draw_mesh(screenQuad);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        pingPong = !pingPong;
    }
    
    void runStreamingStep() {  // ADD THIS FUNCTION
        int src = pingPong ? 0 : 1;
        int dst = pingPong ? 1 : 0;
        
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, distFBO[dst]);
        
        streamingShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, distTextures[src][0]);
        ShaderHelper::setUniform1i("distTex0", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, distTextures[src][1]);
        ShaderHelper::setUniform1i("distTex1", 1);
        
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, distTextures[src][2]);
        ShaderHelper::setUniform1i("distTex2", 2);
        
        ShaderHelper::setUniform2f("gridSize", float(NX), float(NY));
        
        gl.draw_mesh(screenQuad);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        pingPong = !pingPong;
    }
    
    void computeMacroscopic() {
        int current = pingPong ? 0 : 1;
        
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, macroFBO);
        
        macroscopicShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, distTextures[current][0]);
        ShaderHelper::setUniform1i("distTex0", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, distTextures[current][1]);
        ShaderHelper::setUniform1i("distTex1", 1);
        
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, distTextures[current][2]);
        ShaderHelper::setUniform1i("distTex2", 2);
        
        gl.draw_mesh(screenQuad);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void debugCheckValues() {
        if (!debugMode || frameCount > 10) return;
        
        std::cout << "\n--- Debug Check (Frame " << frameCount << ") ---" << std::endl;
        
        glBindFramebuffer(GL_FRAMEBUFFER, macroFBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        
        float densityValues[9];
        glReadPixels(NX/2-1, NY/2-1, 3, 3, GL_RED, GL_FLOAT, densityValues);
        
        std::cout << "Density at center: " << densityValues[4] << std::endl;
        
        float min = densityValues[0], max = densityValues[0], sum = 0;
        for (int i = 0; i < 9; i++) {
            sum += densityValues[i];
            if (densityValues[i] < min) min = densityValues[i];
            if (densityValues[i] > max) max = densityValues[i];
        }
        std::cout << "Density range: [" << min << ", " << max << "], avg: " << sum/9.0f << std::endl;
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void render() {
        glViewport(0, 0, window.width, window.height);
        
        displayShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, densityTexture);
        ShaderHelper::setUniform1i("densityTex", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        ShaderHelper::setUniform1i("velocityTex", 1);
        
        gl.draw_mesh(screenQuad);
    }
    
    void update() {
        frameCount++;
        
        // Run LBM steps (correct order!)
        runCollisionStep();
        runStreamingStep();  // ADD THIS
        computeMacroscopic();
        
        if (debugMode && frameCount <= 10) {
            debugCheckValues();
        }
        
        if (frameCount % 100 == 0) {
            std::cout << "Frame " << frameCount << std::endl;
        }
    }
    
    void cleanup() {
        for (int i = 0; i < 2; i++) {
            glDeleteTextures(3, distTextures[i]);
        }
        glDeleteTextures(1, &densityTexture);
        glDeleteTextures(1, &velocityTexture);
        glDeleteFramebuffers(2, distFBO);
        glDeleteFramebuffers(1, &macroFBO);
    }
};

int main() {
    gl.init();
    window.create("Step 4: Fixed LBM", 800, 800);
    
    IncrementalLBM sim;
    sim.initialize();
    
    gl.set_clear_color(0.1f, 0.1f, 0.1f, 1.0f);
    
    while (!window.should_close()) {
        gl.clear(GL_COLOR_BUFFER_BIT);
        sim.update();
        sim.render();
        window.update();
    }
    
    sim.cleanup();
    gl.destroy();
    return 0;
}
