#include <flgl.h>
#include <flgl/tools.h>
#include <flgl/geometry.h>
#include <shader_helper.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <chrono>
#include <iomanip>

class AdvectionTest {
private:
    Mesh<Vt_2Dclassic> screenQuad;
    Shader displayShader;
    Shader advectionShader;
    
    GLuint densityTextures[2];  // Ping-pong buffers
    GLuint velocityTexture;
    GLuint fbo[2];  // Framebuffers for ping-pong
    
    static constexpr int NX = 256;
    static constexpr int NY = 256;
    
    bool pingPong = false;
    int frameCount = 0;
    float dt = 0.01f;  // Time step
    
    std::vector<Vt_2Dclassic> quadVertices = {
        {{-1.0f,  1.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 1.0f}}
    };
    
    std::vector<uint32_t> quadIndices = {0, 1, 2, 0, 2, 3};

    void addBlob(float* data, int cx, int cy, int radius, float value) {
        for (int y = -radius; y <= radius; y++) {
            for (int x = -radius; x <= radius; x++) {
                if (x*x + y*y <= radius*radius) {
                    int px = cx + x;
                    int py = cy + y;
                    if (px >= 0 && px < NX && py >= 0 && py < NY) {
                        float dist = sqrt(x*x + y*y) / float(radius);
                        float amount = (1.0f - dist) * (value - 1.0f);
                        data[py * NX + px] += amount;
                    }
                }
            }
        }
    }

public:
    void initialize() {
        std::cout << "=== Step 2.1: Advection Test ===" << std::endl;
        
        // Create quad
        screenQuad = Mesh<Vt_2Dclassic>::from_vectors(quadVertices, quadIndices);
        std::cout << "✓ Quad created" << std::endl;
        
        // Create textures
        createDensityTextures();
        std::cout << "✓ Density textures created (ping-pong)" << std::endl;
        
        createVelocityTexture();
        std::cout << "✓ Velocity texture created" << std::endl;
        
        createFramebuffers();
        std::cout << "✓ Framebuffers created" << std::endl;
        
        // Load shaders
        displayShader.create("density_display", "density_display_frag");
        advectionShader.create("advection", "advection_frag");
        std::cout << "✓ Shaders loaded" << std::endl;
        
        std::cout << "\nYou should see 4 blobs swirling around in a circle!" << std::endl;
        std::cout << "✓ Initialization complete!\n" << std::endl;
    }
    
    void createDensityTextures() {
        float* densityData = new float[NX * NY];
        
        // Initialize with base density
        for (int i = 0; i < NX * NY; i++) {
            densityData[i] = 1.0f;
        }
        
        // Add 4 blobs
        addBlob(densityData, NX/4, NY/2, 20, 2.0f);      // Left
        addBlob(densityData, 3*NX/4, NY/2, 20, 2.0f);    // Right
        addBlob(densityData, NX/2, NY/4, 20, 1.8f);      // Bottom
        addBlob(densityData, NX/2, 3*NY/4, 20, 1.8f);    // Top
        
        // Create TWO textures for ping-pong
        for (int i = 0; i < 2; i++) {
            glGenTextures(1, &densityTextures[i]);
            glBindTexture(GL_TEXTURE_2D, densityTextures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, NX, NY, 0, GL_RED, GL_FLOAT, densityData);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        
        delete[] densityData;
    }
    
    void createVelocityTexture() {
        float* velocityData = new float[NX * NY * 2];
        
        // Create circular velocity field
        for (int y = 0; y < NY; y++) {
            for (int x = 0; x < NX; x++) {
                int idx = (y * NX + x) * 2;
                
                // Center-relative coordinates
                float dx = x - NX/2.0f;
                float dy = y - NY/2.0f;
                float r = sqrt(dx*dx + dy*dy);
                
                if (r > 1.0f) {
                    // Rotation speed proportional to radius
                    float speed = 5.0f;
                    
                    // Tangential velocity
                    velocityData[idx] = (-dy / r) * speed;     // vx
                    velocityData[idx+1] = (dx / r) * speed;    // vy
                    
                    // Fade at edges
                    float maxR = NX/2.0f;
                    if (r > maxR * 0.8f) {
                        float fade = 1.0f - (r - maxR*0.8f)/(maxR*0.2f);
                        fade = std::max(0.0f, std::min(1.0f, fade));
                        velocityData[idx] *= fade;
                        velocityData[idx+1] *= fade;
                    }
                } else {
                    velocityData[idx] = 0.0f;
                    velocityData[idx+1] = 0.0f;
                }
            }
        }
        
        glGenTextures(1, &velocityTexture);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, NX, NY, 0, GL_RG, GL_FLOAT, velocityData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        delete[] velocityData;
    }
    
    void createFramebuffers() {
        for (int i = 0; i < 2; i++) {
            glGenFramebuffers(1, &fbo[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                  GL_TEXTURE_2D, densityTextures[i], 0);
            
            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                std::cerr << "ERROR: Framebuffer " << i << " incomplete!" << std::endl;
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    
    void advect() {
        // Source and destination textures
        int src = pingPong ? 1 : 0;
        int dst = pingPong ? 0 : 1;
        
        // Render to destination texture
        glViewport(0, 0, NX, NY);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[dst]);
        
        advectionShader.bind();
        
        // Bind source density texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, densityTextures[src]);
        ShaderHelper::setUniform1i("densityTex", 0);
        
        // Bind velocity texture
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        ShaderHelper::setUniform1i("velocityTex", 1);
        
        // Set uniforms
        ShaderHelper::setUniform1f("dt", dt);
        ShaderHelper::setUniform2f("texelSize", 1.0f/NX, 1.0f/NY);
        
        gl.draw_mesh(screenQuad);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        // Swap buffers
        pingPong = !pingPong;
    }
    
    void render() {
        // Reset viewport for window
        glViewport(0, 0, window.width, window.height);
        
        displayShader.bind();
        
        // Display current density texture
        int current = pingPong ? 1 : 0;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, densityTextures[current]);
        ShaderHelper::setUniform1i("densityTex", 0);
        
        gl.draw_mesh(screenQuad);
    }
    
    void update() {
        frameCount++;
        advect();
        
        // Print status every 100 frames
        if (frameCount % 100 == 0) {
            std::cout << "Frame " << frameCount << " - Advection running smoothly" << std::endl;
        }
    }
    
    void cleanup() {
        glDeleteTextures(2, densityTextures);
        glDeleteTextures(1, &velocityTexture);
        glDeleteFramebuffers(2, fbo);
    }
};

int main() {
    gl.init();
    window.create("LBM Step 2.1 - Advection", 800, 800);
    
    AdvectionTest test;
    test.initialize();
    
    gl.set_clear_color(0.1f, 0.1f, 0.1f, 1.0f);
    
    while (!window.should_close()) {
        test.update();
        gl.clear(GL_COLOR_BUFFER_BIT);
        test.render();
        window.update();
    }
    
    test.cleanup();
    gl.destroy();
    return 0;
}
