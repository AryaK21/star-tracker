#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>

#define PI 3.14159265358979323846
#define DEG2RAD (PI / 180.0)
#define RAD2DEG (180.0 / PI)

// --- ARCHITECTURE ---
typedef struct {
    int id; double ra; double dec; float magnitude; double alt; double az;
} Star;

// --- UPGRADED SHADERS ---
// The Fragment Shader can now switch between drawing glowing stars and transparent UI grids.
const char* vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in float aMag;\n"
    "uniform mat4 view;\n"
    "uniform mat4 projection;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = projection * view * vec4(aPos, 1.0);\n"
    "   gl_PointSize = max(1.0, 7.0 - aMag);\n" 
    "}\0";

const char* fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 overrideColor;\n" // Allow CPU to send custom colors
    "uniform int useOverride;\n"    // Toggle switch
    "void main()\n"
    "{\n"
    "   if (useOverride == 1) {\n"
    "       FragColor = overrideColor;\n" // Draw UI Grids
    "   } else {\n"
    "       FragColor = vec4(1.0f, 0.98f, 0.95f, 1.0f);\n" // Draw Stars
    "   }\n"
    "}\n\0";

// --- MATH ENGINE ---
void calculate_alt_az(Star *star, double lat_deg, double lst_hours) {
    double lat = lat_deg * DEG2RAD;
    double dec = star->dec * DEG2RAD;
    double ha_hours = lst_hours - star->ra;
    double ha = ha_hours * 15.0 * DEG2RAD; 

    double sin_alt = sin(dec) * sin(lat) + cos(dec) * cos(lat) * cos(ha);
    star->alt = asin(sin_alt) * RAD2DEG;

    double y = -sin(ha);
    double x = tan(dec) * cos(lat) - sin(lat) * cos(ha);
    star->az = atan2(y, x) * RAD2DEG;
    if (star->az < 0) star->az += 360.0;
}

int main() {
    // ==========================================
    // 1. DATA INGESTION & PARSING
    // ==========================================
    FILE *file = fopen("stars.csv", "r");
    if (!file) return -1;

    char buffer[512]; int star_count = 0;
    fgets(buffer, sizeof(buffer), file); 
    while (fgets(buffer, sizeof(buffer), file)) {
        if (buffer[0] != '\n' && buffer[0] != '\r') star_count++;
    }

    Star *universe = malloc(star_count * sizeof(Star));
    rewind(file); fgets(buffer, sizeof(buffer), file); 

    int i = 0;
    while (fgets(buffer, sizeof(buffer), file) && i < star_count) {
        if (buffer[0] == '\n' || buffer[0] == '\r') continue; 
        
        char *ptr = buffer; char *fields[60]; 
        for (int k = 0; k < 60; k++) fields[k] = ""; 
        int col = 0; fields[col++] = ptr; 
        
        while (*ptr) {
            if (*ptr == ',') {
                *ptr = '\0'; 
                if (col < 60) fields[col++] = ptr + 1; 
            } else if (*ptr == '\n' || *ptr == '\r') *ptr = '\0'; 
            ptr++;
        }

        if (col >= 29) { 
            universe[i].id = atoi(fields[0]); 
            double ra_h = atof(fields[19]), ra_m = atof(fields[20]), ra_s = atof(fields[21]);
            universe[i].ra = (ra_h + (ra_m / 60.0) + (ra_s / 3600.0)) * 15.0;
            double dec_sign = (fields[22][0] == '-') ? -1.0 : 1.0;
            double dec_d = atof(fields[23]), dec_m = atof(fields[24]), dec_s = atof(fields[25]);
            universe[i].dec = dec_sign * (dec_d + (dec_m / 60.0) + (dec_s / 3600.0));
            universe[i].magnitude = atof(fields[28]);
            
            // Set for Ahilyanagar
            calculate_alt_az(&universe[i], 19.09, 14.5);
            i++;
        }
    }
    fclose(file); star_count = i; 

    // ==========================================
    // 2. BUILD THE 3D STAR ARRAY
    // ==========================================
    float *star_gpu_data = malloc(star_count * 4 * sizeof(float));
    for (int j = 0; j < star_count; j++) {
        double alt_rad = universe[j].alt * DEG2RAD;
        double az_rad = universe[j].az * DEG2RAD;
        
        star_gpu_data[j * 4 + 0] = (float)(cos(alt_rad) * sin(az_rad)) * 100.0f;
        star_gpu_data[j * 4 + 1] = (float)(sin(alt_rad)) * 100.0f;
        star_gpu_data[j * 4 + 2] = (float)(-cos(alt_rad) * cos(az_rad)) * 100.0f;
        star_gpu_data[j * 4 + 3] = universe[j].magnitude;
    }

    // ==========================================
    // 3. BUILD THE ENVIRONMENT GRID (FLOOR & COMPASS)
    // ==========================================
    int env_vertices = 1 + 361 + 8; // Center + Circle Perimeter + 4 Cardinal Lines (2 pts each)
    float *env_data = malloc(env_vertices * 3 * sizeof(float));
    int e_idx = 0;

    // We lower the floor slightly (Y = -0.5) so the camera feels like it's standing at eye-level
    float floor_y = -0.5f;

    // PART A: The Glass Horizon Floor (Triangle Fan)
    env_data[e_idx++] = 0.0f; env_data[e_idx++] = floor_y; env_data[e_idx++] = 0.0f; // Center
    for (int d = 0; d <= 360; d++) {
        float rad = d * DEG2RAD;
        env_data[e_idx++] = sin(rad) * 100.0f;
        env_data[e_idx++] = floor_y;
        env_data[e_idx++] = -cos(rad) * 100.0f;
    }

    // PART B: The Compass Lines (N, S, E, W)
    // 1. North (-Z)
    env_data[e_idx++] = 0.0f; env_data[e_idx++] = floor_y + 0.1f; env_data[e_idx++] = 0.0f;
    env_data[e_idx++] = 0.0f; env_data[e_idx++] = floor_y + 0.1f; env_data[e_idx++] = -100.0f;
    // 2. South (+Z)
    env_data[e_idx++] = 0.0f; env_data[e_idx++] = floor_y + 0.1f; env_data[e_idx++] = 0.0f;
    env_data[e_idx++] = 0.0f; env_data[e_idx++] = floor_y + 0.1f; env_data[e_idx++] = 100.0f;
    // 3. East (+X)
    env_data[e_idx++] = 0.0f; env_data[e_idx++] = floor_y + 0.1f; env_data[e_idx++] = 0.0f;
    env_data[e_idx++] = 100.0f; env_data[e_idx++] = floor_y + 0.1f; env_data[e_idx++] = 0.0f;
    // 4. West (-X)
    env_data[e_idx++] = 0.0f; env_data[e_idx++] = floor_y + 0.1f; env_data[e_idx++] = 0.0f;
    env_data[e_idx++] = -100.0f; env_data[e_idx++] = floor_y + 0.1f; env_data[e_idx++] = 0.0f;

    // ==========================================
    // 4. GRAPHICS INITIALIZATION
    // ==========================================
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "The Dark Architect - Horizon Engine", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glEnable(GL_PROGRAM_POINT_SIZE); 
    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Enables transparency
    glEnable(GL_DEPTH_TEST); 

    unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vertexShaderSource, NULL); glCompileShader(vShader);
    unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fragmentShaderSource, NULL); glCompileShader(fShader);
    
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vShader); glAttachShader(shaderProgram, fShader);
    glLinkProgram(shaderProgram);

    // --- SETUP STAR BUFFERS ---
    unsigned int starVAO, starVBO;
    glGenVertexArrays(1, &starVAO); glGenBuffers(1, &starVBO);
    glBindVertexArray(starVAO); glBindBuffer(GL_ARRAY_BUFFER, starVBO);
    glBufferData(GL_ARRAY_BUFFER, star_count * 4 * sizeof(float), star_gpu_data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // --- SETUP ENVIRONMENT BUFFERS ---
    unsigned int envVAO, envVBO;
    glGenVertexArrays(1, &envVAO); glGenBuffers(1, &envVBO);
    glBindVertexArray(envVAO); glBindBuffer(GL_ARRAY_BUFFER, envVBO);
    glBufferData(GL_ARRAY_BUFFER, env_vertices * 3 * sizeof(float), env_data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // (We don't need attribute 1 (Magnitude) for the environment)

    // ==========================================
    // 5. CAMERA & UNIFORM SETUP
    // ==========================================
    float yaw = 180.0f; // Start looking South
    float pitch = 0.0f; 

    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "overrideColor");
    unsigned int useColorLoc = glGetUniformLocation(shaderProgram, "useOverride");

    // ==========================================
    // 6. THE RENDER LOOP
    // ==========================================
    while (!glfwWindowShouldClose(window)) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, 1);

        float cameraSpeed = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  yaw -= cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) yaw += cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    pitch += cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  pitch -= cameraSpeed;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f; 

        vec3 cameraPos   = {0.0f, 0.0f, 0.0f}; 
        vec3 cameraFront;
        cameraFront[0] = cos(glm_rad(yaw)) * cos(glm_rad(pitch));
        cameraFront[1] = sin(glm_rad(pitch));
        cameraFront[2] = sin(glm_rad(yaw)) * cos(glm_rad(pitch));
        glm_vec3_normalize(cameraFront);
        
        vec3 cameraUp = {0.0f, 1.0f, 0.0f};
        vec3 target; glm_vec3_add(cameraPos, cameraFront, target);
        
        mat4 view, projection;
        glm_lookat(cameraPos, target, cameraUp, view);

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h); 
        glViewport(0, 0, display_w, display_h);                 
        glm_perspective(glm_rad(45.0f), (float)display_w / (float)display_h, 0.1f, 1000.0f, projection);

        glClearColor(0.01f, 0.01f, 0.03f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, (float *)view);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, (float *)projection);

        // --- LAYER 1: DRAW ALL STARS ---
        glUniform1i(useColorLoc, 0); // Use native star glowing color
        glBindVertexArray(starVAO);
        glDrawArrays(GL_POINTS, 0, star_count); 

        // --- LAYER 2: DRAW ENVIRONMENT (TRANSPARENT BOUNDARIES) ---
        // We turn off the depth mask. This allows the transparent floor to render OVER the southern stars without hiding them.
        glDepthMask(GL_FALSE); 
        glBindVertexArray(envVAO);

        // A. Draw the Semi-Transparent Glass Floor
        glUniform1i(useColorLoc, 1); 
        glUniform4f(colorLoc, 0.05f, 0.15f, 0.25f, 0.6f); // Deep blue glass, 60% opacity
        glDrawArrays(GL_TRIANGLE_FAN, 0, 362);

        // B. Draw the North Compass Line (Bright Red)
        glUniform4f(colorLoc, 0.9f, 0.1f, 0.1f, 0.8f); 
        glDrawArrays(GL_LINES, 362, 2);

        // C. Draw the S, E, W Compass Lines (Bright Cyan)
        glUniform4f(colorLoc, 0.1f, 0.8f, 0.9f, 0.5f); 
        glDrawArrays(GL_LINES, 364, 6);

        // Turn depth mask back on for the next frame
        glDepthMask(GL_TRUE); 

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    free(universe); free(star_gpu_data); free(env_data); 
    glfwTerminate();
    return 0;
}