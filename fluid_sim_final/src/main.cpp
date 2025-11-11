#include <flgl.h>
#include <flgl/tools.h>
#include <flgl/geometry.h>
#include <shader_helper.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <iomanip>

class LBMStreamingTest {
private:
    Mesh<Vt_2Dclassic> screenQuad;
    
    // Shaders
    Shader initShader;
    Shader collisionShader;
    Shader streamingShader;  // NEW!
    Shader macroscopicShader;
    Shader displayShader;
    
    // LBM textures
    GLuint distTextures[2][3];
    
    // Macroscopic quantities
    GLuint densityTexture;
    GLuint velocityTexture;
    
    // Framebuffers
    GLuint distFBO[2];
    GLuint macroFBO;
    
    // State
    bool pingPong = false;
    int frameCount = 0;
    
    // Grid size
    static constexpr int NX = 256;
    static constexpr int NY = 256;
    
    // LBM parameters
    static constexpr float TAU = 0.6f;
    
    std::vector<Vt_2Dclassic> quadVertices = {
        {{-1.0f,  1.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 1.0f}}
    };
    
    std::vector<uint32_t> quadIndices = {0, 1, 2, 0, 2, 3};

public:
    void initialize() {
        std::cout << "=== Step 3.1: LBM Streaming Test ===" << std::endl;
        std::cout << "Grid: " << NX << "x" << NY << std::endl;
        std::cout << "Tau: " << TAU << std::endl;
        
        // Create mesh
        screenQuad = Mesh<Vt_2Dclassic>::from_vectors(quadVertices, quadIndices);
        std::cout << "✓ Quad created" << std::endl;
        
        // Create textures
        createDistributionTextures();
        std::cout << "✓ Distribution textures created" << std::endl;
        
        createMacroscopicTextures();
        std::cout << "✓ Macroscopic textures created" << std::endl;
        
        // Create framebuffers
        createFramebuffers();
        std::cout << "✓ Framebuffers created" << std::endl;
        
        // Load shaders
        initShader.create("lbm_init", "lbm_init_frag");
        collisionShader.create("lbm_collision", "lbm_collision_frag");
        streamingShader.create("lbm_streaming", "lbm_streaming_frag");  // NEW!
        macroscopicShader.create("lbm_macro", "lbm_macro_frag");
        displayShader.create("lbm_display", "lbm_display_frag");
        std::cout << "✓ Shaders loaded" << std::endl;
        
        // Initialize distributions
        initializeLBM();
        std::cout << "✓ LBM initialized" << std::endl;
        
        // Compute initial macroscopic
        computeMacroscopic();
        checkValues();
        
        std::cout << "\nYou should see a blob that naturally spreads outward!" << std::endl;
        std::cout << "✓ Initialization complete!\n" << std::endl;
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
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glGenTextures(1, &velocityTexture);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, NX, NY, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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
                std::cerr << "ERROR: Distribution FBO " << i << " incomplete!" << std::endl;
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
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "ERROR: Macro FBO incomplete!" << std::endl;
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void initializeLBM() {
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, distFBO[0]);
        
        initShader.bind();
        gl.draw_mesh(screenQuad);
        
        glBindFramebuffer(GL_READ_FRAMEBUFFER, distFBO[0]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, distFBO[1]);
        glBlitFramebuffer(0, 0, NX, NY, 0, 0, NX, NY, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void runCollision() {
        int src = pingPong ? 1 : 0;
        int dst = pingPong ? 0 : 1;
        
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
        
        gl.draw_mesh(screenQuad);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        pingPong = !pingPong;
    }
    
    void runStreaming() {
        int src = pingPong ? 1 : 0;
        int dst = pingPong ? 0 : 1;
        
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
    
    void checkValues() {
        glBindFramebuffer(GL_FRAMEBUFFER, macroFBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        
        float density[9];
        glReadPixels(NX/2-1, NY/2-1, 3, 3, GL_RED, GL_FLOAT, density);
        
        float sum = 0, min = density[0], max = density[0];
        for (int i = 0; i < 9; i++) {
            sum += density[i];
            min = std::min(min, density[i]);
            max = std::max(max, density[i]);
        }
        
        std::cout << "Density check - Center: " << density[4] 
                  << ", Range: [" << min << ", " << max 
                  << "], Avg: " << sum/9.0f << std::endl;
        
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
        
        // LBM steps
        runCollision();
        runStreaming();  // NEW!
        computeMacroscopic();
        
        if (frameCount % 100 == 0) {
            std::cout << "Frame " << frameCount << " - ";
            checkValues();
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
    window.create("LBM Step 3.1 - Streaming", 800, 800);
    
    LBMStreamingTest sim;
    sim.initialize();
    
    gl.set_clear_color(0.1f, 0.1f, 0.1f, 1.0f);
    
    while (!window.should_close()) {
        sim.update();
        gl.clear(GL_COLOR_BUFFER_BIT);
        sim.render();
        window.update();
    }
    
    sim.cleanup();
    gl.destroy();
    return 0;
}
