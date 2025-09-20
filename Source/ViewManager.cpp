///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport - camera, projection
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
ViewManager::ViewManager(ShaderManager *pShaderManager, const ViewConfig& cfg)
{
    m_pShaderManager = pShaderManager;
    m_cfg = cfg;
    m_camera = std::make_unique<Camera>();
    m_camera->Position       = m_cfg.camPos;
    m_camera->Front          = m_cfg.camFront;
    m_camera->Up             = m_cfg.camUp;
    m_camera->Zoom           = m_cfg.defaultZoom;
    m_camera->MovementSpeed  = m_cfg.movementSpeed;

    m_lastX = static_cast<float>(m_cfg.windowWidth)  / 2.0f;
    m_lastY = static_cast<float>(m_cfg.windowHeight) / 2.0f;
}


/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
    m_pShaderManager = nullptr;
    m_pWindow = nullptr;
    // m_camera auto-deletes via unique_ptr
}


/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
    GLFWwindow* window = glfwCreateWindow(m_cfg.windowWidth, m_cfg.windowHeight, windowTitle, NULL, NULL);
    ...
}



/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// when the first mouse move event is received, this needs to be recorded so that
	// all subsequent mouse moves can correctly calculate the X position offset and Y
	// position offset for proper operation
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	// calculate the X offset and Y offset values for moving the 3D camera accordingly
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos; // reversed since y-coordinates go from bottom to top

	// set the current positions into the last position variables
	gLastX = xMousePos;
	gLastY = yMousePos;

	// move the 3D camera according to the calculated offsets
	g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// Close the window if the Escape key is pressed.
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// Ensure the camera object is valid.
	if (g_pCamera == NULL)
	{
		return;
	}

	// ----------------------------
	// Horizontal & Depth Navigation (WASD)
	// ----------------------------
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		// Move forward (zoom in)
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		// Move backward (zoom out)
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		// Pan left
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		// Pan right
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}

	// ----------------------------
	// Vertical Navigation (QE)
	// ----------------------------
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		// Move upward
		g_pCamera->ProcessKeyboard(UP, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		// Move downward
		g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);
	}

	// ----------------------------
	// Projection Mode Toggle: P for Perspective, O for Orthographic
	// ----------------------------
	// Note: To avoid rapid switching from continuous key press, you might later
	// add a debounce mechanism.
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
	{
		// Set to perspective projection.
		bOrthographicProjection = false;
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
	{
		// Set to orthographic projection.
		bOrthographicProjection = true;
	}
}


/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// Update timing information.
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// Process keyboard events for camera movement and projection toggling.
	ProcessKeyboardEvents();

	// Get the current view matrix from the camera.
	view = g_pCamera->GetViewMatrix();

	// Calculate the projection matrix based on the selected projection mode.
	if (bOrthographicProjection)
	{
		// For orthographic projection, define the view volume.
		// Adjust the dimensions (orthoWidth and orthoHeight) to your scene.
		float aspect = (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT;
		float orthoHeight = 10.0f;  // For example, 10 units tall.
		float orthoWidth = orthoHeight * aspect;

		// The near and far planes are set to include the scene.
		projection = glm::ortho(-orthoWidth / 2.0f, orthoWidth / 2.0f,
			-orthoHeight / 2.0f, orthoHeight / 2.0f,
			0.1f, 100.0f);
	}
	else
	{
		// Perspective projection using the camera's Zoom value.
		projection = glm::perspective(glm::radians(g_pCamera->Zoom),
			(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT,
			0.1f, 100.0f);
	}

	// Pass the matrices to the shader.
	if (m_pShaderManager != NULL)
	{
		m_pShaderManager->setMat4Value(g_ViewName, view);
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}
void ViewManager::Scroll_Callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// Adjust the camera's movement speed based on the scroll input.
	// The Camera's ProcessMouseScroll() method should modify MovementSpeed.
	g_pCamera->ProcessMouseScroll((float)yoffset);
}
