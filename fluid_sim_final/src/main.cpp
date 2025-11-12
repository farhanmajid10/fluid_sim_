#include <flgl.h>
#include <flgl/tools.h>
#include <flgl/geometry.h>
#include <shader_helper.h>
#include <iostream>
#include <cmath>
#include <vector>
#include <iomanip>

class LBMInteractive {
private:
    Mesh<Vt_2Dclassic> screenQuad;
    
    // Shaders
    Shader initShader;
    Shader collisionShader;
    Shader streamingShader;
    Shader forceShader;
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
    
    // Mouse state
    float mouseX = 0.5f;
    float mouseY = 0.5f;
    float prevMouseX = 0.5f;
    float prevMouseY = 0.5f;
    bool mousePressed = false;
    bool wasPressed = false;
    
    // Grid size
    static constexpr int NX = 256;
    static constexpr int NY = 256;
    
    // LBM parameters
    static constexpr float TAU = 0.52f;  // Adjusted for better wave interaction
    
    std::vector<Vt_2Dclassic> quadVertices = {
        {{-1.0f,  1.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 1.0f}}
    };
    
    std::vector<uint32_t> quadIndices = {0, 1, 2, 0, 2, 3};

public:
    void initialize() {
        std::cout << "=== LBM Interactive Fluid Simulation ===" << std::endl;
        std::cout << "Grid: " << NX << "x" << NY << std::endl;
        std::cout << "Tau: " << TAU << std::endl;
        
        screenQuad = Mesh<Vt_2Dclassic>::from_vectors(quadVertices, quadIndices); //creating a quad that covers the screen.
        std::cout << "✓ Quad created" << std::endl;
        
        createDistributionTextures();  //create GPU textures for f0 - f8
        std::cout << "✓ Distribution textures created" << std::endl;
        
        createMacroscopicTextures();  //create textures for density/velocity.
        std::cout << "✓ Macroscopic textures created" << std::endl;
        
        createFramebuffers();  //setup render targets.
        std::cout << "✓ Framebuffers created" << std::endl;
        
        // load shaders, vertex and frag. the frag files get handled in the CMakelists.txt
        initShader.create("lbm_init_multi", "lbm_init_multi_frag");
        collisionShader.create("lbm_collision", "lbm_collision_frag");
        streamingShader.create("lbm_streaming", "lbm_streaming_frag");
        forceShader.create("lbm_force", "lbm_force_frag");
        macroscopicShader.create("lbm_macro", "lbm_macro_frag");
        displayShader.create("lbm_water", "lbm_water_frag");
        std::cout << "✓ Shaders loaded" << std::endl;
        
        initializeLBM();  //set initial fluid state.
        std::cout << "✓ LBM initialized" << std::endl; 
        
        computeMacroscopic();  //calculate initial density/veloclity.
        
        std::cout << "\n=== INTERACTIVE FLUID ===" << std::endl;
        std::cout << "CLICK and DRAG to create waves!" << std::endl;
        std::cout << "Create multiple waves to see them interact!" << std::endl;
        std::cout << "✓ Ready!\n" << std::endl;
    }
    
    void createDistributionTextures() {
        for (int p = 0; p < 2; p++) {  // two sets for pingpong.
            for (int i = 0; i < 3; i++) {  // 4 in 2 textures and 1 in the other texture- for efficiency. total 9 velocity directions. 
                glGenTextures(1, &distTextures[p][i]);  //creates uniques ID.
                glBindTexture(GL_TEXTURE_2D, distTextures[p][i]);  // binds texture using ID.
                
                if (i < 2) {  //glTexImage2D parameters (target texture, minmap_level, internal format- channels RGBA- 32 bit floating - 128bits/pixel, width, height, border, format of data we are uploading, datatype of each component, pointer to data(set later))
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, NX, NY, 0, 
                               GL_RGBA, GL_FLOAT, nullptr);  // configures texture as RGBA.
                } else {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, NX, NY, 0, 
                               GL_RED, GL_FLOAT, nullptr);  // configures texture as R-only. this one is single channel only.
                }
                
                //no blending in magnification and minification. and use nearest valid edge and don't wrap around.
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            }
        }
    }
    
    void createMacroscopicTextures() {


        glGenTextures(1, &densityTexture);  //create unique id for density.
        glBindTexture(GL_TEXTURE_2D, densityTexture); // bind texture for the operations below.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, NX, NY, 0, GL_RED, GL_FLOAT, nullptr); //allocates gpu memory for textures. R32F, single channel, 32bit floating. one scalar value per cell. 
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        //same for velocity.
        glGenTextures(1, &velocityTexture);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, NX, NY, 0, GL_RG, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    
    void createFramebuffers() {  
/*
//Essentially, the distTextures are attached to the distFBOs and the density and velocity textues get attached to the macroFBO.
//### distFBO[0] and distFBO[1]
distFBO[0] ←──── distTextures[0][0]
           ←──── distTextures[0][1]
           ←──── distTextures[0][2]

distFBO[1] ←──── distTextures[1][0]
           ←──── distTextures[1][1]
           ←──── distTextures[1][2]


### macroFBO
macroFBO ←──── densityTexture
         ←──── velocityTexture

*/
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
                                      GL_TEXTURE_2D, distTextures[i][j], 0);  // the distribution functions are attached through the attachment points to the buffers.
            }
            
            glDrawBuffers(3, drawBuffers);  // then the distribution functions are drawn.
            
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
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);  //unbind.
    }
    
    void initializeLBM() {
        glViewport(0, 0, NX, NY); //setting pixel area. 
        glBindFramebuffer(GL_FRAMEBUFFER, distFBO[0]); //binding frame buffer. distFBO[0] -> distTextures[0][0], [0][1], [0][2],
        
        initShader.bind();  // runs init shader to set starting values.
        gl.draw_mesh(screenQuad);
        
        //copies the distFBO[0] to distFBo[1].
        glBindFramebuffer(GL_READ_FRAMEBUFFER, distFBO[0]);  
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, distFBO[1]);
        glBlitFramebuffer(0, 0, NX, NY, 0, 0, NX, NY, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0); //unbind.
    }
    
    void handleMouse() {
        // Get mouse state
        double mx, my;
        glfwGetCursorPos(glfwGetCurrentContext(), &mx, &my);  // gets the x and y coord from mouse and puts into mx and my.
        
        // Convert to normalized texture coordinates [0,1]
        mouseX = float(mx) / float(window.width);
        mouseY = 1.0f - float(my) / float(window.height);  // flipped to match texture coordinates.
        
        mousePressed = glfwGetMouseButton(glfwGetCurrentContext(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS; 
        
        // reset previous position if just started pressing
        if (mousePressed && !wasPressed) {
            prevMouseX = mouseX;
            prevMouseY = mouseY;
        }
        
        wasPressed = mousePressed;
    }
    
    void applyForce() {
        if (!mousePressed) return;
        
        int src = pingPong ? 1 : 0;
        int dst = pingPong ? 0 : 1;
        
        glViewport(0, 0, NX, NY); //match grid size.
        glBindFramebuffer(GL_FRAMEBUFFER, distFBO[dst]);  // shader writes to distFBO[dst] and other attached textures.
        
        forceShader.bind();
        
        //distTex0 writes to distTextures[src][0]   
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, distTextures[src][0]);
        ShaderHelper::setUniform1i("distTex0", 0);
        
        //distTex1 writes to distTextures[src][1]   
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, distTextures[src][1]);
        ShaderHelper::setUniform1i("distTex1", 1);
        
        //distTex2 writes to distTextures[src][2]   
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, distTextures[src][2]);
        ShaderHelper::setUniform1i("distTex2", 2);
        
        ShaderHelper::setUniform2f("mousePos", mouseX, mouseY);
        ShaderHelper::setUniform2f("mouseVel", 
            (mouseX - prevMouseX) * 100.0f, 
            (mouseY - prevMouseY) * 100.0f);
        ShaderHelper::setUniform1f("forceRadius", 0.04f);   // Larger area
        ShaderHelper::setUniform1f("forceStrength", 0.15f);   // Stronger force
        
        gl.draw_mesh(screenQuad);//execs shader on every pixel.
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        pingPong = !pingPong;
        
        prevMouseX = mouseX;
        prevMouseY = mouseY;
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
        
        gl.draw_mesh(screenQuad);  // applies the shader to all pixels.
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        pingPong = !pingPong;
    }
    
    void runStreamingWithBoundaries() {
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
        
        handleMouse();
        
        // Apply mouse force if dragging
        applyForce();
        
        // LBM steps
        runCollision();
        runStreamingWithBoundaries();
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
    gl.init();  //initialize OpenGL context.    
    window.create("LBM Water Simulation - Click & Drag!", 800, 800);
    
    LBMInteractive sim;
    sim.initialize();
    
    gl.set_clear_color(0.02f, 0.05f, 0.1f, 1.0f);  // Dark blue background
    
    while (!window.should_close()) {
        sim.update();  //physics step
        gl.clear(GL_COLOR_BUFFER_BIT); 
        sim.render();  //draw to screen.
        window.update(); //swap buffers and handle events
    }
    
    sim.cleanup();
    gl.destroy();
    return 0;
}
