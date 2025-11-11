#include <flgl.h>
#include <flgl/tools.h>
#include <flgl/geometry.h>
#include <shader_helper.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <chrono>
#include <iomanip>

class VelocityFieldTest {
private:
    Mesh<Vt_2Dclassic> screenQuad;
    Shader velocityDisplayShader;
    Shader densityDisplayShader;
    GLuint densityTexture;
    GLuint velocityTexture;
    
    static constexpr int NX = 256;
    static constexpr int NY = 256;
    
    bool showVelocity = true;
    
    // Time tracking
    std::chrono::steady_clock::time_point startTime;
    std::chrono::steady_clock::time_point lastSwitch;
    float switchInterval = 2.0f; // seconds
    
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
        std::cout << "=== Step 2.0: Velocity Field Visualization ===" << std::endl;
        
        // Initialize timers
        startTime = std::chrono::steady_clock::now();
        lastSwitch = startTime;
        
        // Create quad
        screenQuad = Mesh<Vt_2Dclassic>::from_vectors(quadVertices, quadIndices);
        std::cout << "✓ Quad created" << std::endl;
        
        // Create textures
        createDensityTexture();
        std::cout << "✓ Density texture created" << std::endl;
        
        createVelocityTexture();
        std::cout << "✓ Velocity texture created" << std::endl;
        
        // Load shaders
        velocityDisplayShader.create("velocity_display", "velocity_display_frag");
        densityDisplayShader.create("density_display", "density_display_frag");
        std::cout << "✓ Shaders loaded" << std::endl;
        
        std::cout << "\nThe display alternates every " << switchInterval << " seconds:" << std::endl;
        std::cout << "  - Velocity Field (swirling colors)" << std::endl;
        std::cout << "  - Density (4 blobs)" << std::endl;
        std::cout << "✓ Initialization complete!\n" << std::endl;
    }
    
    void createDensityTexture() {
        float* densityData = new float[NX * NY];
        
        for (int i = 0; i < NX * NY; i++) {
            densityData[i] = 1.0f;
        }
        
        addBlob(densityData, NX/4, NY/2, 30, 2.0f);
        addBlob(densityData, 3*NX/4, NY/2, 30, 2.0f);
        addBlob(densityData, NX/2, NY/4, 25, 1.8f);
        addBlob(densityData, NX/2, 3*NY/4, 25, 1.8f);
        
        glGenTextures(1, &densityTexture);
        glBindTexture(GL_TEXTURE_2D, densityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, NX, NY, 0, GL_RED, GL_FLOAT, densityData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        delete[] densityData;
    }
    
    void createVelocityTexture() {
        // Create swirling velocity field (counterclockwise rotation)
        float* velocityData = new float[NX * NY * 2];
        
        for (int y = 0; y < NY; y++) {
            for (int x = 0; x < NX; x++) {
                int idx = (y * NX + x) * 2;
                
                // Center-relative coordinates
                float dx = x - NX/2.0f;
                float dy = y - NY/2.0f;
                float r = sqrt(dx*dx + dy*dy);
                
                if (r > 0.1f) {
                    // Tangential velocity (perpendicular to radius)
                    // For counterclockwise: vx = -dy/r, vy = dx/r
                    float speed = 10.0f;  // Pixels per frame
                    
                    velocityData[idx] = (-dy / r) * speed;     // vx
                    velocityData[idx+1] = (dx / r) * speed;    // vy
                    
                    // Reduce speed at edges
                    float maxR = NX/2.0f;
                    if (r > maxR * 0.7f) {
                        float fade = 1.0f - (r - maxR*0.7f)/(maxR*0.3f);
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
        
        // Print sample values for debugging
        int centerIdx = (NY/2 * NX + NX/2) * 2;
        int rightIdx = (NY/2 * NX + (NX/2 + 20)) * 2;
        std::cout << "Velocity at center: (" << velocityData[centerIdx] 
                  << ", " << velocityData[centerIdx+1] << ")" << std::endl;
        std::cout << "Velocity right of center: (" << velocityData[rightIdx] 
                  << ", " << velocityData[rightIdx+1] << ")" << std::endl;
        
        glGenTextures(1, &velocityTexture);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, NX, NY, 0, GL_RG, GL_FLOAT, velocityData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        delete[] velocityData;
    }
    
    void update() {
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<float> timeSinceSwitch = currentTime - lastSwitch;
        
        if (timeSinceSwitch.count() >= switchInterval) {
            showVelocity = !showVelocity;
            lastSwitch = currentTime;
            
            // Show elapsed time for debugging
            std::chrono::duration<float> totalTime = currentTime - startTime;
            std::cout << "[" << std::fixed << std::setprecision(1) << totalTime.count() 
                     << "s] Showing: " << (showVelocity ? "Velocity Field" : "Density") << std::endl;
        }
    }
    
    void render() {
        if (showVelocity) {
            // Show velocity field
            velocityDisplayShader.bind();
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, velocityTexture);
            ShaderHelper::setUniform1i("velocityTex", 0);
            
        } else {
            // Show density
            densityDisplayShader.bind();
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, densityTexture);
            ShaderHelper::setUniform1i("densityTex", 0);
        }
        
        gl.draw_mesh(screenQuad);
    }
    
    void cleanup() {
        glDeleteTextures(1, &densityTexture);
        glDeleteTextures(1, &velocityTexture);
    }
};

int main() {
    gl.init();
    window.create("LBM Step 2.0 - Velocity Field", 800, 800);
    
    VelocityFieldTest test;
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
