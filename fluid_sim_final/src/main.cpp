#include <flgl.h>
#include <flgl/tools.h>
#include <flgl/geometry.h>
#include <shader_helper.h>
#include <iostream>
#include <cmath>
#include <vector>

class TextureTest {
private:
    Mesh<Vt_2Dclassic> screenQuad;
    Shader displayShader;
    GLuint densityTexture;
    
    static constexpr int NX = 256;
    static constexpr int NY = 256;
    
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
                        // Smooth falloff
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
        std::cout << "=== Step 1.1: Texture Display Test ===" << std::endl;
        
        // Create quad
        screenQuad = Mesh<Vt_2Dclassic>::from_vectors(quadVertices, quadIndices);
        std::cout << "✓ Quad created" << std::endl;
        
        // Create density texture
        createDensityTexture();
        std::cout << "✓ Density texture created (" << NX << "x" << NY << ")" << std::endl;
        
        // Load shader
        displayShader.create("density_display", "density_display_frag");
        std::cout << "✓ Display shader loaded" << std::endl;
        std::cout << "✓ Initialization complete!" << std::endl;
    }
    
    void createDensityTexture() {
        // Create initial density data
        float* densityData = new float[NX * NY];
        
        // Fill with base density
        for (int i = 0; i < NX * NY; i++) {
            densityData[i] = 1.0f;
        }
        
        // Add test blobs (like your Processing code)
        addBlob(densityData, NX/4, NY/2, 30, 2.0f);      // Left blob
        addBlob(densityData, 3*NX/4, NY/2, 30, 2.0f);    // Right blob
        addBlob(densityData, NX/2, NY/4, 25, 1.8f);      // Bottom blob
        addBlob(densityData, NX/2, 3*NY/4, 25, 1.8f);    // Top blob
        
        // Debug: print some values
        std::cout << "Sample density values:" << std::endl;
        std::cout << "  Center: " << densityData[(NY/2) * NX + (NX/2)] << std::endl;
        std::cout << "  Left blob center: " << densityData[(NY/2) * NX + (NX/4)] << std::endl;
        
        // Create OpenGL texture
        glGenTextures(1, &densityTexture);
        glBindTexture(GL_TEXTURE_2D, densityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, NX, NY, 0, GL_RED, GL_FLOAT, densityData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        delete[] densityData;
    }
    
    void render() {
        displayShader.bind();
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, densityTexture);
        ShaderHelper::setUniform1i("densityTex", 0);
        
        gl.draw_mesh(screenQuad);
    }
    
    void cleanup() {
        glDeleteTextures(1, &densityTexture);
    }
};

int main() {
    gl.init();
    window.create("LBM Step 1.1 - Texture Display", 800, 800);
    
    TextureTest test;
    test.initialize();
    
    gl.set_clear_color(0.1f, 0.1f, 0.1f, 1.0f);
    
    std::cout << "Starting render loop..." << std::endl;
    
    while (!window.should_close()) {
        gl.clear(GL_COLOR_BUFFER_BIT);
        test.render();
        window.update();
    }
    
    test.cleanup();
    gl.destroy();
    return 0;
}
