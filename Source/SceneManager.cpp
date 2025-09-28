///////////////////////////////////////////////////////////////////////////////
// scenemanager.cpp
// ============
// manage the preparing and rendering of 3D scenes - textures, materials, lighting
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////



#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
// Loads a file, creates a GL texture, and stores it under `tag`.
bool SceneManager::CreateGLTexture(const std::string& tag,
                                   const std::string& filePath,
                                   bool flipVertically)
{
    GLuint tex = 0;
    if (!loadTextureFromFile(filePath, tex, flipVertically)) {
        std::cerr << "[SceneManager] Failed to create texture for tag '"
                  << tag << "' from '" << filePath << "'\n";
        return false;
    }

    // If we already had a texture under this tag, delete it to prevent leaks
    auto it = m_textureMap.find(tag);
    if (it != m_textureMap.end() && it->second != 0) {
        glDeleteTextures(1, &it->second);
    }

    m_textureMap[tag] = tex;
    return true;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
// Correctly deletes all GL textures created by SceneManager.
void SceneManager::DestroyGLTextures()
{
    for (auto& kv : m_textureMap) {
        if (kv.second != 0) {
            glDeleteTextures(1, &kv.second);
        }
    }
    m_textureMap.clear();
}


/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
GLuint SceneManager::FindTextureID(const std::string& tag) const
{
    auto it = m_textureMap.find(tag);
    if (it != m_textureMap.end()) {
        return it->second;
    }
    return 0; // 0 means "no texture" in OpenGL
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationZ * rotationY * rotationX * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

bool SceneManager::BindTextureByTag(const std::string& tag, GLenum target) const
{
    const GLuint id = FindTextureID(tag);
    if (id == 0) return false;
    glBindTexture(target, id);
    return true;
}

bool SceneManager::TextureExists(const std::string& tag) const
{
    return m_textureMap.find(tag) != m_textureMap.end();
}

// =====================
//  Private: File Loader
// =====================

bool SceneManager::loadTextureFromFile(const std::string& filePath,
                                       GLuint& outTex,
                                       bool flipVertically)
{
    stbi_set_flip_vertically_on_load(flipVertically ? 1 : 0);

    int width = 0, height = 0, channels = 0;
    unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "[SceneManager] stbi_load failed for " << filePath << "\n";
        return false;
    }

    GLenum format = GL_RGB;
    if (channels == 1)       format = GL_RED;
    else if (channels == 3)  format = GL_RGB;
    else if (channels == 4)  format = GL_RGBA;
    else {
        std::cerr << "[SceneManager] Unsupported channel count (" << channels
                  << ") for " << filePath << "\n";
        stbi_image_free(data);
        return false;
    }

    glGenTextures(1, &outTex);
    glBindTexture(GL_TEXTURE_2D, outTex);

    // Texture upload
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height,
                 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Default sampling/wrapping — tune to your project’s needs
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,     GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Unbind and free CPU data
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);

    return true;
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

GLuint SceneManager::LoadTexture(const char* filepath)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Load the image using stb_image
	int width, height, nrChannels;
	unsigned char* data = stbi_load(filepath, &width, &height, &nrChannels, 0);
	if (data)
	{
		GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
		stbi_image_free(data);
		return textureID;
	}
	else
	{
		std::cout << "Failed to load texture: " << filepath << std::endl;
		stbi_image_free(data);
		return 0;
	}
}

// ---------- Define Material Properties ----------
void SceneManager::DefineObjectMaterials()
{
	m_pShaderManager->setVec3Value("material.diffuseColor", glm::vec3(1.0f, 1.0f, 1.0f));
	m_pShaderManager->setVec3Value("material.specularColor", glm::vec3(0.6f, 0.6f, 0.6f));
	m_pShaderManager->setFloatValue("material.shininess", 32.0f);
}

// ---------- Set Up Lighting ----------
void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue("bUseLighting", true);

	m_pShaderManager->setBoolValue("directionalLight.bActive", true);
	m_pShaderManager->setVec3Value("directionalLight.direction", glm::vec3(-0.3f, -1.0f, -0.3f));
	m_pShaderManager->setVec3Value("directionalLight.ambient", glm::vec3(0.3f));
	m_pShaderManager->setVec3Value("directionalLight.diffuse", glm::vec3(0.6f));
	m_pShaderManager->setVec3Value("directionalLight.specular", glm::vec3(1.0f));

	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);
	m_pShaderManager->setVec3Value("pointLights[0].position", glm::vec3(1.0f, 3.0f, 2.0f));
	m_pShaderManager->setVec3Value("pointLights[0].ambient", glm::vec3(0.2f, 0.1f, 0.1f));
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", glm::vec3(0.9f, 0.3f, 0.3f));
	m_pShaderManager->setVec3Value("pointLights[0].specular", glm::vec3(0.9f, 0.3f, 0.3f));

	for (int i = 1; i < 5; ++i)
		m_pShaderManager->setBoolValue("pointLights[" + std::to_string(i) + "].bActive", false);

	m_pShaderManager->setBoolValue("spotLight.bActive", false);
}
/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
 // ---------- Prepare Scene ----------
void SceneManager::PrepareScene()
{
	LoadSceneTextures();
	DefineObjectMaterials();
	SetupSceneLights();

	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadTorusMesh();
}
void SceneManager::LoadSceneTextures()
{
	m_textureWood = LoadTexture("textures/wood_seamless.jpeg");
	m_textureMouseBody = LoadTexture("textures/grey_mouse_body.jpeg");
	m_textureMouseButtons = LoadTexture("textures/dark_mouse_buttons.jpeg");
}


/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
 // RenderScene() - 7-1 Final Project Milestone 5

// RenderScene() - 7-1 Final Project Milestone 5

void SceneManager::RenderScene()
{
	glm::vec3 scale, position;
	float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;

	// === Desk Plane (Textured Wood) ===
	scale = glm::vec3(20.0f, 1.0f, 10.0f);
	position = glm::vec3(0.0f);
	SetTransformations(scale, 0.0f, 0.0f, 0.0f, position);
	m_pShaderManager->setBoolValue("bUseTexture", true);
	glBindTexture(GL_TEXTURE_2D, m_textureWood);
	m_basicMeshes->DrawPlaneMesh();

	// === Mouse Body (Textured Sphere) ===
	scale = glm::vec3(0.9f, 0.5f, 1.3f);
	position = glm::vec3(-2.0f, 0.5f, 0.0f);
	SetTransformations(scale, 0.0f, 0.0f, -15.0f, position);
	m_pShaderManager->setBoolValue("bUseTexture", true);
	glBindTexture(GL_TEXTURE_2D, m_textureMouseBody);
	m_basicMeshes->DrawSphereMesh();

	// === Mouse Buttons (Tapered Cylinders) ===
	m_pShaderManager->setBoolValue("bUseTexture", true);
	glBindTexture(GL_TEXTURE_2D, m_textureMouseButtons);
	for (int i = 0; i < 2; i++) {
		scale = glm::vec3(0.2f, 0.05f, 0.2f);
		position = glm::vec3(-2.0f + 0.1f * i, 0.65f, 0.2f);
		SetTransformations(scale, 90.0f, 0.0f, 0.0f, position);
		m_basicMeshes->DrawTaperedCylinderMesh();
	}

	// === Keyboard (Box) ===
	scale = glm::vec3(3.0f, 0.3f, 1.5f);
	position = glm::vec3(1.0f, 0.15f, 0.0f);
	SetTransformations(scale, 0.0f, 0.0f, 0.0f, position);
	m_pShaderManager->setBoolValue("bUseTexture", false);
	SetShaderColor(0.9f, 0.9f, 0.9f, 1.0f);
	m_basicMeshes->DrawBoxMesh();

	// === Cloud Wrist Rest (Overlapping White Spheres) ===
	m_pShaderManager->setBoolValue("bUseTexture", false);
	SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);
	for (int i = 0; i < 3; i++) {
		scale = glm::vec3(0.6f);
		position = glm::vec3(-0.5f + i * 0.6f, 0.35f, -0.6f);
		SetTransformations(scale, 0.0f, 0.0f, 0.0f, position);
		m_basicMeshes->DrawSphereMesh();
	}

	// === Glasses (Torus + Cylinders) ===
	m_pShaderManager->setBoolValue("bUseTexture", false);
	SetShaderColor(0.1f, 0.1f, 0.1f, 1.0f);
	for (int i = 0; i < 2; i++) {
		scale = glm::vec3(0.3f);
		position = glm::vec3(-0.5f + i * 0.8f, 0.5f, 1.0f);
		SetTransformations(scale, 90.0f, 0.0f, 0.0f, position);
		m_basicMeshes->DrawTorusMesh();
	}

	// Glasses arm (bridge)
	scale = glm::vec3(0.8f, 0.05f, 0.05f);
	position = glm::vec3(-0.1f, 0.5f, 1.0f);
	SetTransformations(scale, 0.0f, 0.0f, 0.0f, position);
	m_basicMeshes->DrawBoxMesh();
}

