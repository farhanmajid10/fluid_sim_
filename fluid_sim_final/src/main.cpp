// main.cpp - Step 1.0: Gradient Test
#include <flgl.h>
#include <flgl/tools.h>
#include <flgl/geometry.h>
#include <iostream>

class SimpleTest {
private:
    Mesh<Vt_2Dclassic> screenQuad;
    Shader testShader;
    
    std::vector<Vt_2Dclassic> quadVertices = {
        {{-1.0f,  1.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 1.0f}}
    };
    
    std::vector<uint32_t> quadIndices = {0, 1, 2, 0, 2, 3};

public:
    void initialize() {
        std::cout << "=== Step 1.0: Gradient Test ===" << std::endl;
        
        screenQuad = Mesh<Vt_2Dclassic>::from_vectors(quadVertices, quadIndices);
        std::cout << "✓ Quad created" << std::endl;
        
        testShader.create("gradient_test", "gradient_test_frag");
        std::cout << "✓ Shader loaded" << std::endl;
    }
    
    void render() {
        testShader.bind();
        gl.draw_mesh(screenQuad);
    }
};

int main() {
    gl.init();
    window.create("LBM Step 1.0 - Gradient Test", 800, 800);
    
    SimpleTest test;
    test.initialize();
    
    gl.set_clear_color(0.1f, 0.1f, 0.1f, 1.0f);
    
    while (!window.should_close()) {
        gl.clear(GL_COLOR_BUFFER_BIT);
        test.render();
        window.update();
    }
    
    gl.destroy();
    return 0;
}
