#include <flgl.h>
#include <flgl/tools.h>
#include <flgl/geometry.h>
#include <shader_helper.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <iomanip>

class SimpleLBM {
private:
    Mesh<Vt_2Dclassic> screenQuad;
    
    // Shaders
    Shader initShader;
    Shader collisionShader;
    Shader macroscopicShader;
    Shader displayShader;
    
    // TWO sets of textures for ping-pong
    GLuint distTextures[3];   // Set 1
    GLuint distTextures2[3];  // Set 2
    GLuint densityTexture;
    GLuint velocityTexture;
    
    // Framebuffers
    GLuint distFBO;   // FBO 1
    GLuint distFBO2;  // FBO 2
    GLuint macroFBO;
    
    // State
    bool pingPong = false;
    
    // Grid
    static constexpr int NX = 256;
    static constexpr int NY = 256;
    
    int frameCount = 0;
    
    // Quad
    std::vector<Vt_2Dclassic> quadVertices = {
        {{-1.0f,  1.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 1.0f}}
    };
    
    std::vector<uint32_t> quadIndices = {0, 1, 2, 0, 2, 3};

public:
    void initialize() {
        std::cout << "\n=== MINIMAL LBM WITH COLLISION TEST ===" << std::endl;
        
        // Create mesh
        screenQuad = Mesh<Vt_2Dclassic>::from_vectors(quadVertices, quadIndices);
        std::cout << "✓ Mesh created" << std::endl;
        
        // Create textures
        createTextures();
        std::cout << "✓ Textures created" << std::endl;
        
        // Create framebuffers
        createFramebuffers();
        std::cout << "✓ Framebuffers created" << std::endl;
        
        // Load shaders
        initShader.create("simple_init", "simple_init_frag");
        collisionShader.create("simple_collision", "simple_collision_frag");
        macroscopicShader.create("simple_macro", "simple_macro_frag");
        displayShader.create("simple_display", "simple_display_frag");
        std::cout << "✓ Shaders loaded" << std::endl;
        
        // Initialize distributions
        initializeLBM();
        std::cout << "✓ LBM initialized" << std::endl;
        
        // Copy to second buffer for ping-pong
        glBindFramebuffer(GL_READ_FRAMEBUFFER, distFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, distFBO2);
        glBlitFramebuffer(0, 0, NX, NY, 0, 0, NX, NY, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        std::cout << "✓ Second buffer initialized" << std::endl;
        
        // Compute initial macroscopic
        computeMacroscopic();
        checkDensity();
        
        std::cout << "=== Setup Complete ===\n" << std::endl;
    }
    
    void createTextures() {
        // Create TWO sets of distribution textures for ping-pong
        for (int set = 0; set < 2; set++) {
            GLuint* texSet = (set == 0) ? distTextures : distTextures2;
            
            for (int i = 0; i < 3; i++) {
                glGenTextures(1, &texSet[i]);
                glBindTexture(GL_TEXTURE_2D, texSet[i]);
                
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
        
        // Density texture
        glGenTextures(1, &densityTexture);
        glBindTexture(GL_TEXTURE_2D, densityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, NX, NY, 0, GL_RED, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        // Velocity texture
        glGenTextures(1, &velocityTexture);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, NX, NY, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    void createFramebuffers() {
        // Create TWO distribution FBOs for ping-pong
        for (int set = 0; set < 2; set++) {
            GLuint* fbo = (set == 0) ? &distFBO : &distFBO2;
            GLuint* texSet = (set == 0) ? distTextures : distTextures2;
            
            glGenFramebuffers(1, fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, *fbo);
            
            GLenum drawBuffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
            for (int i = 0; i < 3; i++) {
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
                                      GL_TEXTURE_2D, texSet[i], 0);
            }
            glDrawBuffers(1, drawBuffers);
            
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "ERROR: Distribution FBO " << set << " incomplete!" << std::endl;
            }
        }
        
        // Macro FBO
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
    glBindFramebuffer(GL_FRAMEBUFFER, distFBO);
    
    // Clear everything to 0 first
    float clearZero[] = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearBufferfv(GL_COLOR, 0, clearZero);
    glClearBufferfv(GL_COLOR, 1, clearZero);
    glClearBufferfv(GL_COLOR, 2, clearZero);
    
    initShader.bind();
    
    // CRITICAL: Make sure we're drawing to ALL attachments
    GLenum drawBuffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, drawBuffers);
    
    gl.draw_mesh(screenQuad);
    glFinish();
    
    // Now check
    float test[4];
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glReadPixels(NX/2, NY/2, 1, 1, GL_RGBA, GL_FLOAT, test);
    std::cout << "After shader - Tex0: " << test[0] << ", " << test[1] 
              << ", " << test[2] << ", " << test[3] << std::endl;
    
    glReadBuffer(GL_COLOR_ATTACHMENT1);
    glReadPixels(NX/2, NY/2, 1, 1, GL_RGBA, GL_FLOAT, test);
    std::cout << "After shader - Tex1: " << test[0] << ", " << test[1] 
              << ", " << test[2] << ", " << test[3] << std::endl;
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
    
    void runCollision() {
        GLuint srcFBO = pingPong ? distFBO2 : distFBO;
        GLuint dstFBO = pingPong ? distFBO : distFBO2;
        GLuint* srcTex = pingPong ? distTextures2 : distTextures;
        
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, dstFBO);
        
        collisionShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, srcTex[0]);
        ShaderHelper::setUniform1i("distTex0", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, srcTex[1]);
        ShaderHelper::setUniform1i("distTex1", 1);
        
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, srcTex[2]);
        ShaderHelper::setUniform1i("distTex2", 2);
        
        gl.draw_mesh(screenQuad);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        pingPong = !pingPong;
    }
    
    void computeMacroscopic() {
        GLuint* currentTex = pingPong ? distTextures2 : distTextures;
        
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, macroFBO);
        
        macroscopicShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTex[0]);
        ShaderHelper::setUniform1i("distTex0", 0);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, currentTex[1]);
        ShaderHelper::setUniform1i("distTex1", 1);
        
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, currentTex[2]);
        ShaderHelper::setUniform1i("distTex2", 2);
        
        gl.draw_mesh(screenQuad);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void checkDensity() {
        glBindFramebuffer(GL_FRAMEBUFFER, macroFBO);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        
        float density[9];
        glReadPixels(NX/2-1, NY/2-1, 3, 3, GL_RED, GL_FLOAT, density);
        
        float min = density[0], max = density[0], avg = 0;
        for (int i = 0; i < 9; i++) {
            avg += density[i];
            if (density[i] < min) min = density[i];
            if (density[i] > max) max = density[i];
        }
        avg /= 9.0f;
        
        std::cout << "Frame " << frameCount << " - Density: ["
                  << std::fixed << std::setprecision(4) 
                  << min << ", " << max << "], avg=" << avg;
        
        if (std::abs(avg - 1.0f) > 0.01f) {
            std::cout << " ⚠️ WARNING!";
        }
        std::cout << std::endl;
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void render() {
        glViewport(0, 0, window.width, window.height);
        
        displayShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, densityTexture);
        ShaderHelper::setUniform1i("densityTex", 0);
        
        gl.draw_mesh(screenQuad);
    }
    
    void update() {
        frameCount++;
        
        // Run collision step
        runCollision();
        
        // Recompute macroscopic
        computeMacroscopic();
        
        // Check every 100 frames
        if (frameCount % 100 == 0) {
            checkDensity();
        }
    }
    
    void cleanup() {
        glDeleteTextures(3, distTextures);
        glDeleteTextures(3, distTextures2);
        glDeleteTextures(1, &densityTexture);
        glDeleteTextures(1, &velocityTexture);
        glDeleteFramebuffers(1, &distFBO);
        glDeleteFramebuffers(1, &distFBO2);
        glDeleteFramebuffers(1, &macroFBO);
    }
};

int main() {
    gl.init();
    window.create("Minimal LBM Test", 800, 800);
    
    SimpleLBM sim;
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
