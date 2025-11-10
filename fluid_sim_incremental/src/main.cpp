#include <flgl.h>
#include <flgl/tools.h>
#include <flgl/geometry.h>
#include <iostream>
#include <shader_helper.h>

class IncrementalLBM{
private:
    Mesh<Vt_2Dclassic> screenQuad;
//    Shader testShader;
//    GLuint densityTexture;
    Shader displayShader;
    Shader advectionShader;

    GLuint densityTextures[2];
    GLuint velocityTexture;

    GLuint fbo[2];

    bool pingPong = false;

    static constexpr int NX = 256;
    static constexpr int NY = 256;

    std::vector<Vt_2Dclassic> quadVertices = {
        {{-1.0f,  1.0f}, {0.0f, 1.0f}},
        {{-1.0f, -1.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f}, {1.0f, 1.0f}}
    };

    std::vector<uint32_t> quadIndices = {0, 1, 2, 0, 2, 3};

public:
    void initialize(){
        screenQuad = Mesh<Vt_2Dclassic>::from_vectors(quadVertices, quadIndices);
//        testShader.create("test", "test_frag");
        std::cout << "Step 0: Basic rendering initialized" << std::endl;

//        createDensityTexture();
//        displayShader.create("display","display_frag");


        createDensityTextures();
        createVelocityTexture();
        std::cout << "Texture created.\n" << std::endl;

        createFramebuffers();
        std::cout << "Framebuffers created.\n" << std::endl;

        displayShader.create("display", "display_frag");
        advectionShader.create("advection", "advection_frag");
        std::cout << "Shaders loaded" << std::endl;

        std::cout << "S1: initialized." << std::endl;
    }

void createVelocityTexture() {
    // Create a PROPER swirling velocity field
    float* velocityData = new float[NX * NY * 2];
    
    for (int y = 0; y < NY; y++) {
        for (int x = 0; x < NX; x++) {
            int idx = (y * NX + x) * 2;
            
            // Center-relative coordinates (in pixels)
            float dx = x - NX/2.0f;
            float dy = y - NY/2.0f;
            float r = sqrt(dx*dx + dy*dy);
            
            if (r > 0.1f) {  // Avoid division by zero
                // Create rotation: velocity perpendicular to radius
                // For counterclockwise: vx = -dy/r, vy = dx/r
                float speed = 5.0f;  // Rotation speed
                
                // Normalize and scale
                velocityData[idx] = (-dy / r) * speed;     // vx
                velocityData[idx+1] = (dx / r) * speed;    // vy
                
                // Optional: reduce speed away from center
                float maxR = NX/2.0f;
                if (r > maxR * 0.8f) {
                    float fade = 1.0f - (r - maxR*0.8f)/(maxR*0.2f);
                    fade = std::max(0.0f, fade);
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

    void createDensityTextures(){
        float* initialData = new float[NX * NY];
        for(int i = 0; i < NX * NY; i++){
            initialData[i] = 1.0f;
        }

        addBlob(initialData, NX/4,NY/2, 20,2.0f);
        addBlob(initialData, 3*NX/4, NY/2, 20,2.0f);
        addBlob(initialData, NX/2,NY/4, 15,1.5f);
        addBlob(initialData, NX/2,3*NY/4, 15,1.5f);
        
        for(int i = 0; i < 2; i++){
            glGenTextures(1,&densityTextures[i]);
            glBindTexture(GL_TEXTURE_2D, densityTextures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, NX, NY, 0, GL_RED, GL_FLOAT, initialData);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
        
        delete[] initialData;
    }

    void createFramebuffers(){
        for(int i = 0; i < 2; i++){
            glGenFramebuffers(1, &fbo[i]);
            glBindFramebuffer(GL_FRAMEBUFFER, fbo[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, densityTextures[i], 0);
            if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
                std::cerr << "Framebuffer " << i << "is not complete!" << std::endl;
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void addBlob(float* data, int cx, int cy,int radius, float value){
        for(int y = -radius; y <= radius; y++){
            for(int x = -radius; x <= radius; x++){
                if(x*x + y*y <= radius*radius){
                    int px = cx + x;
                    int py = cy + y;
                    if(px >= 0 && px < NX && py >= 0 && py < NY){
                        data[py * NX + px]= value;
                    }
                }
            }
        }
    }

    void advect(){
        int src = pingPong ? 1:0;
        int dst = pingPong ? 0:1;

        glViewport(0,0,NX,NY);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo[dst]);

        advectionShader.bind();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, densityTextures[src]);
        ShaderHelper::setUniform1i("densityTex",0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, velocityTexture);
        ShaderHelper::setUniform1i("velocityTex", 1);

        ShaderHelper::setUniform1f("dt", 0.01f);
        ShaderHelper::setUniform2f("texelSize", 1.0f/NX, 1.0f/NY);

        gl.draw_mesh(screenQuad);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        pingPong = !pingPong;
    }

    void render(){

          glViewport(0, 0, window.width, window.height);
        int current = pingPong? 1 : 0;

        displayShader.bind();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, densityTextures[current]);
        ShaderHelper::setUniform1i("densityTex", 0);

        gl.draw_mesh(screenQuad);
    }

   void update(){
        advect();
    } 

    void cleanup(){
        glDeleteTextures(2, densityTextures);
        glDeleteTextures(1, &velocityTexture);
        glDeleteFramebuffers(2, fbo);
    }

};

int main(){
    gl.init();
    window.create("Incremental LBM - S0", 800, 800);

    IncrementalLBM sim;
    sim.initialize();

    gl.set_clear_color(0.0f, 0.0f, 0.0f, 1.0f);

    while(!window.should_close()){
        gl.clear(GL_COLOR_BUFFER_BIT);
        sim.update();
        sim.render();
        window.update();
    }

    sim.cleanup();
    gl.destroy();
    return 0;
}
