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
    static constexpr float TAU = 1.0f;
    
    // Debug mode
    bool debugMode = true;
    int frameCount = 0;
    
    // D2Q9 weights
    const float weights[9] = {
        1.0f/36.0f, 1.0f/9.0f, 1.0f/36.0f,
        1.0f/9.0f, 4.0f/9.0f, 1.0f/9.0f,
        1.0f/36.0f, 1.0f/9.0f, 1.0f/36.0f
    };
    
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
            std::cerr << "OpenGL error at " << where << ": 0x" << std::hex << err << std::dec;
            switch(err) {
                case GL_INVALID_ENUM: std::cerr << " (INVALID_ENUM)"; break;
                case GL_INVALID_VALUE: std::cerr << " (INVALID_VALUE)"; break;
                case GL_INVALID_OPERATION: std::cerr << " (INVALID_OPERATION)"; break;
                case GL_INVALID_FRAMEBUFFER_OPERATION: std::cerr << " (INVALID_FRAMEBUFFER_OPERATION)"; break;
                case GL_OUT_OF_MEMORY: std::cerr << " (OUT_OF_MEMORY)"; break;
            }
            std::cerr << std::endl;
            hasError = true;
        }
        if (!hasError && debugMode && frameCount < 3) {
            std::cout << "[OK] No GL errors at: " << where << std::endl;
        }
    }

public:
    void initialize() {
        std::cout << "\n=== Step 3: LBM Distribution Functions ===" << std::endl;
        std::cout << "Grid size: " << NX << "x" << NY << std::endl;
        std::cout << "Debug mode: " << (debugMode ? "ON" : "OFF") << std::endl;
        
        // Create screen quad
        screenQuad = Mesh<Vt_2Dclassic>::from_vectors(quadVertices, quadIndices);
        std::cout << "1. Screen quad created" << std::endl;
        checkGLError("after screen quad");
        
        // Create textures
        createDistributionTextures();
        std::cout << "2. Distribution textures created" << std::endl;
        checkGLError("after distribution textures");
        
        createMacroscopicTextures();
        std::cout << "3. Macroscopic textures created" << std::endl;
        checkGLError("after macroscopic textures");
        
        // Create framebuffers
        createFramebuffers();
        std::cout << "4. Framebuffers created" << std::endl;
        checkGLError("after framebuffers");
        
        // Load shaders
        initShader.create("lbm_init3", "lbm_init3_frag");
        macroscopicShader.create("lbm_macro3", "lbm_macro3_frag");
        displayShader.create("display3", "display3_frag");
        std::cout << "5. Shaders loaded" << std::endl;
        checkGLError("after shaders");
        
        // Initialize distributions
        initializeLBM();
        std::cout << "6. LBM initialized" << std::endl;
        checkGLError("after LBM init");
        
        // Compute initial macroscopic quantities
        computeMacroscopic();
        std::cout << "7. Initial macroscopic quantities computed" << std::endl;
        checkGLError("after initial macro");
        
        // Debug: Check initial values
        debugCheckValues();
        
        std::cout << "=== Initialization Complete! ===\n" << std::endl;
    }
    
    void createDistributionTextures() {
        for (int p = 0; p < 2; p++) {
            for (int i = 0; i < 3; i++) {
                glGenTextures(1, &distTextures[p][i]);
                glBindTexture(GL_TEXTURE_2D, distTextures[p][i]);
                
                if (i < 2) {
                    // RGBA textures for distributions 0-3 and 4-7
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, NX, NY, 0, 
                               GL_RGBA, GL_FLOAT, nullptr);
                } else {
                    // R texture for distribution 8
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, NX, NY, 0, 
                               GL_RED, GL_FLOAT, nullptr);
                }
                
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                
                if (debugMode) {
                    std::cout << "  Created dist texture[" << p << "][" << i 
                             << "] ID=" << distTextures[p][i] << std::endl;
                }
            }
        }
    }
    
    void createMacroscopicTextures() {
        // Density texture
        glGenTextures(1, &densityTexture);
        glBindTexture(GL_TEXTURE_2D, densityTexture);
        
        // Initialize with test pattern
        float* testDensity = new float[NX * NY];
        for (int y = 0; y < NY; y++) {
            for (int x = 0; x < NX; x++) {
                // Gradient pattern for testing
                testDensity[y * NX + x] = 1.0f + 0.1f * float(x) / float(NX);
            }
        }
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, NX, NY, 0, GL_RED, GL_FLOAT, testDensity);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        delete[] testDensity;
        
        std::cout << "  Created density texture ID=" << densityTexture << std::endl;
        
        // Velocity texture
        glGenTextures(1, &velocityTexture);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, NX, NY, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        std::cout << "  Created velocity texture ID=" << velocityTexture << std::endl;
    }
    
    void createFramebuffers() {
        // Distribution framebuffers
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
            
            GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "ERROR: Distribution framebuffer " << i 
                         << " incomplete! Status: 0x" << std::hex << status << std::dec << std::endl;
            } else {
                std::cout << "  Distribution FBO " << i << " complete" << std::endl;
            }
        }
        
        // Macroscopic framebuffer
        glGenFramebuffers(1, &macroFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, macroFBO);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D, densityTexture, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                              GL_TEXTURE_2D, velocityTexture, 0);
        
        GLenum macroBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, macroBuffers);
        
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR: Macroscopic framebuffer incomplete! Status: 0x" 
                     << std::hex << status << std::dec << std::endl;
        } else {
            std::cout << "  Macroscopic FBO complete" << std::endl;
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void initializeLBM() {
        std::cout << "  Initializing LBM distributions..." << std::endl;
        
        // Clear distributions first
        for (int i = 0; i < 2; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, distFBO[i]);
            float clearColor[] = {0.1f, 0.1f, 0.1f, 0.1f};
            glClearBufferfv(GL_COLOR, 0, clearColor);
            glClearBufferfv(GL_COLOR, 1, clearColor);
            float clearRed[] = {0.1f, 0.0f, 0.0f, 0.0f};
            glClearBufferfv(GL_COLOR, 2, clearRed);
        }
        
        // Run initialization shader
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
        
        std::cout << "  LBM initialization complete" << std::endl;
    }
    
    void computeMacroscopic() {
        int current = pingPong ? 1 : 0;
        
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
        if (!debugMode || frameCount > 5) return;
        
        std::cout << "\n--- Debug Check (Frame " << frameCount << ") ---" << std::endl;
        
        // Read back some density values
        glBindFramebuffer(GL_FRAMEBUFFER, macroFBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        
        float densityValues[9];
        glReadPixels(NX/2-1, NY/2-1, 3, 3, GL_RED, GL_FLOAT, densityValues);
        
        std::cout << "Density values (3x3 center):" << std::endl;
        for (int y = 2; y >= 0; y--) {
            for (int x = 0; x < 3; x++) {
                std::cout << std::fixed << std::setprecision(3) 
                         << densityValues[y*3 + x] << " ";
            }
            std::cout << std::endl;
        }
        
        // Check for issues
        float sum = 0.0f;
        bool hasNaN = false;
        bool hasNegative = false;
        bool hasHuge = false;
        
        for (int i = 0; i < 9; i++) {
            sum += densityValues[i];
            if (std::isnan(densityValues[i])) hasNaN = true;
            if (densityValues[i] < 0.0f) hasNegative = true;
            if (densityValues[i] > 100.0f) hasHuge = true;
        }
        
        std::cout << "Average density: " << sum/9.0f << std::endl;
        if (hasNaN) std::cout << "WARNING: NaN detected!" << std::endl;
        if (hasNegative) std::cout << "WARNING: Negative density!" << std::endl;
        if (hasHuge) std::cout << "WARNING: Huge values!" << std::endl;
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void render() {
        // CRITICAL: Reset viewport for window rendering
        glViewport(0, 0, window.width, window.height);
        
        displayShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, densityTexture);
        ShaderHelper::setUniform1i("densityTex", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        ShaderHelper::setUniform1i("velocityTex", 1);
        
        gl.draw_mesh(screenQuad);
        
        checkGLError("after render");
    }
    
    void update() {
        frameCount++;
        computeMacroscopic();
        
        if (debugMode && frameCount <= 5) {
            debugCheckValues();
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
    window.create("Step 3: LBM Debug", 800, 800);
    
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
