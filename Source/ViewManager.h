///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport - camera, projection
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "camera.h"

// GLFW library
#include "GLFW/glfw3.h" 

struct ViewConfig {
    int windowWidth  = 1000;
    int windowHeight = 800;
    float defaultZoom = 80.0f;
    float movementSpeed = 20.0f;
    glm::vec3 camPos  = glm::vec3(0.0f, 5.0f, 12.0f);
    glm::vec3 camFront= glm::vec3(0.0f, -0.5f, -2.0f);
    glm::vec3 camUp   = glm::vec3(0.0f, 1.0f, 0.0f);
};


class ViewManager
{
public:
    explicit ViewManager(ShaderManager* pShaderManager, const ViewConfig& cfg = {});
    ~ViewManager();

    static void Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos);
    static void Scroll_Callback(GLFWwindow* window, double xoffset, double yoffset);

    GLFWwindow* CreateDisplayWindow(const char* windowTitle);
    void PrepareSceneView();

private:
    ShaderManager* m_pShaderManager = nullptr;
    GLFWwindow*    m_pWindow        = nullptr;

    // Replaces file-scope globals:
    std::unique_ptr<Camera> m_camera;
    ViewConfig m_cfg{};
    float m_lastX;
    float m_lastY;
    bool  m_firstMouse = true;
    float m_deltaTime  = 0.0f;
    float m_lastFrame  = 0.0f;
    bool  m_isOrtho    = false;

    void ProcessKeyboardEvents();
};
