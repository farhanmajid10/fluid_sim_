#include <flgl.h>
#include <flgl/tools.h>
#include <flgl/geometry.h>
#include <iostream>
#include <shader_helper.h>
#include <cmath>
#include <vector>

class IncrementalLBM{
private:
    Mesh<Vt_2Dclassic> screenQuad;
    Shader initShader;
    Shader macroscopicShader;
    Shader displayShader;

    GLuint distTextures[2][3];

    GLuint densityTexture;
    GLuint velocityTexture;

    GLuint distFBO[2];
    GLuint macroFBO;

    bool pingPong = false; //which texture is current.

    static constexpr int NX = 256;
    static constexpr int NY = 256;

    static constexpr float TAU = 1.0f;

    const float weights[9] = {
        1.0f/36.0f, 1.0f/9.0f, 1.0f/36.0f,
        1.0f/9.0f, 4.0f/9.0f, 1.0f/9.0f,
        1.0f/36.0f, 1.0f/9.0f, 1.0f/36.0f
    };

    // D2Q9 lattice velocities
    const int ex[9] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};
    const int ey[9] = { 1, 1, 1,  0, 0, 0, -1,-1,-1};
    
    // Quad vertices
    std::vector<Vt_2Dclassic> quadVertices = {
        {{-1.0f,  1.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 1.0f}}
    };

    std::vector<uint32_t> quadIndices = {0, 1, 2, 0, 2, 3};

public:
    void initialize(){
        std::cout << "=== Step 3: LBM Distribution Functions ===" << std::endl;
        
        // Create screen quad
        screenQuad = Mesh<Vt_2Dclassic>::from_vectors(quadVertices, quadIndices);
        std::cout << "Screen quad created" << std::endl;
        
        // Create textures
        createDistributionTextures();
        createMacroscopicTextures();
        std::cout << "Textures created" << std::endl;
        
        // Create framebuffers
        createFramebuffers();
        std::cout << "Framebuffers created" << std::endl;
        
        // Load shaders
        initShader.create("lbm_init3", "lbm_init3_frag");
        macroscopicShader.create("lbm_macro3", "lbm_macro3_frag");
        displayShader.create("display3", "display3_frag");
        std::cout << "Shaders loaded" << std::endl;
        
        // Initialize distributions to equilibrium
        initializeLBM();
        std::cout << "LBM initialized" << std::endl;
        
        // Compute initial macroscopic quantities
        computeMacroscopic();
        std::cout << "Initial macroscopic quantities computed" << std::endl;
        
        std::cout << "Initialization complete!" << std::endl;
    }
    void createDistributionTextures() {
        for (int p = 0; p < 2; p++) {  // ping-pong buffers
            for (int i = 0; i < 3; i++) {  // 3 textures for 9 distributions
                glGenTextures(1, &distTextures[p][i]);
                glBindTexture(GL_TEXTURE_2D, distTextures[p][i]);
                
                if (i < 2) {
                    // First two textures store 4 distributions each (RGBA)
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, NX, NY, 0, 
                               GL_RGBA, GL_FLOAT, nullptr);
                } else {
                    // Third texture stores just f8 (R channel)
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
        // Density texture
        glGenTextures(1, &densityTexture);
        glBindTexture(GL_TEXTURE_2D, densityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, NX, NY, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        // Velocity texture (2 components)
        glGenTextures(1, &velocityTexture);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, NX, NY, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    void createFramebuffers() {
        // Framebuffers for distributions
        for (int i = 0; i < 2; i++) {
            glGenFramebuffers(1, &distFBO[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, distFBO[i]);
            
            // Attach all 3 distribution textures
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
        
        // Framebuffer for macroscopic quantities
        glGenFramebuffers(1, &macroFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, macroFBO);
        
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                              GL_TEXTURE_2D, densityTexture, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                              GL_TEXTURE_2D, velocityTexture, 0);
        
        GLenum macroBuffers[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        glDrawBuffers(2, macroBuffers);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Macroscopic framebuffer incomplete!" << std::endl;
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void initializeLBM() {
        // Set viewport for texture operations
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, distFBO[0]);
        
        initShader.bind();
        ShaderHelper::setUniform1f("tau", TAU);
        ShaderHelper::setUniform2f("gridSize", float(NX), float(NY));
        
        gl.draw_mesh(screenQuad);
        
        // Copy to second buffer
        glBindFramebuffer(GL_READ_FRAMEBUFFER, distFBO[0]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, distFBO[1]);
        glBlitFramebuffer(0, 0, NX, NY, 0, 0, NX, NY, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void computeMacroscopic() {
        int current = pingPong ? 1 : 0;
        
        // Set viewport for texture operations
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
        
        glFinish();  // Make sure GPU is done
    
    // Read a density value from the center
    float centerDensity;
    glBindTexture(GL_TEXTURE_2D, densityTexture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, &centerDensity);
    
    // Only print first few frames
    static int debugCount = 0;
    if (debugCount++ < 5) {
        std::cout << "Frame " << debugCount << " - Center density: " << centerDensity << std::endl;
        
        // Also check what the sum of weights is
        float weightSum = 1.0f/36.0f * 4 + 1.0f/9.0f * 4 + 4.0f/9.0f;
        std::cout << "  Weight sum should be 1.0, is: " << weightSum << std::endl;
    }   

    }
    
    void render() {
        // CRITICAL FIX: Reset viewport to window size for screen rendering!
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
        // For now, just recompute macroscopic quantities each frame
        computeMacroscopic();
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
    window.create("Step 3: LBM Initialization", 800, 800);
    
    IncrementalLBM sim;
    sim.initialize();
    
    gl.set_clear_color(0.0f, 0.0f, 0.0f, 1.0f);
    
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
