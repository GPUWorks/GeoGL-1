/**
 *	@program		GeoGL: Geographic Graphics Engine, part of MapTube (http://www.maptube.org)
 *	@description	A tool for building 3D representations from geographic data, animations over time and a 3D GIS.
 *	@author			Richard Milton
 *	@organisation	Centre for Advanced Spatial Analysis (CASA), University College London
 *	@website		http://www.casa.ucl.ac.uk
 *	@date			9 May 2014
 *	@licence		Free for all commercial and academic use (including teaching). Not for commercial use. All rights are held by the author.
 */


#include "globe.h"

#include <iostream>

#include "gengine/ogldevice.h"
#include "gengine/graphicscontext.h"
#include "gengine/scenedataobject.h"
#include "gengine/shader.h"
#include "gengine/shaderuniformcollection.h"
//#include "opengl4.h"
#include "object3d.h"
#include "sphere.h"
#include "gengine/events/EventManager.h"
#include "OrbitController.h"
#include "EllipsoidOrbitController.h"
#include "cuboid.h"

#include "Model.h"

#include "geojson.h"

using namespace std;

using namespace gengine;
using namespace gengine::events;

Globe::Globe(void)
{
	ellipsoid = Ellipsoid(); //bit naughty, but the default is the WGS 84 definition

	int WindowWidth=512,WindowHeight=512;
	GC = OGLDevice::XCreateWindow(WindowWidth,WindowHeight); //create graphics context that we can render to

	//initialise font rendering
	_FontFace = GC->LoadFont("C:\\richard\\GitHub\\GeoGL\\externals\\fonts\\FreeSans.ttf",48);

	//start graphics context
	//if (!openglContext.create30ContextGLFW()) { // Create a window and an OpenGL context to run in it
	//	std::cerr<<"Error creating OpenGL context - abort"<<std::endl;
	//	exit(-1);
	//}
	//openglContext.setupScene(); // Setup our OpenGL scene
	
	//Shader* shader = new Shader("shader.vert", "shader.frag"); //--no! use the device
	Shader* shader = OGLDevice::CreateShaderProgram("shader.vert", "shader.frag");
	_Shaders.push_back(shader);

	Shader* diffuse = OGLDevice::CreateShaderProgram("diffuse.vert", "diffuse.frag");
	_Shaders.push_back(diffuse);

	//add sphere representing the earth and shade using diffuse shader
	Sphere* sphere=new Sphere(ellipsoid.A(),ellipsoid.B(),ellipsoid.C(),40,40);
	sphere->SetColour(glm::vec3(0.0,0.4,0.05));
	sphere->AttachShader(diffuse,false);
	SceneGraph.push_back(sphere);

	Cuboid* cuboid=new Cuboid(ellipsoid.A()*1.5,ellipsoid.B()*1.5,ellipsoid.C()*1.5);
	cuboid->AttachShader(shader,false);
	SceneGraph.push_back(cuboid);

	//setup debug object - this is a cube that we can position in the scene as a marker
	//Cube1 is Red, Cube2 is Blue
	debugCube1 = new Cuboid(100000,100000,100000);
	debugCube1->SetColour(glm::vec3(1.0,0,0));
	debugCube1->AttachShader(shader,false);
	SceneGraph.push_back(debugCube1);
	debugCube2 = new Cuboid(100000,100000,100000);
	debugCube2->SetColour(glm::vec3(0,0,1.0));
	debugCube2->AttachShader(shader,false);
	SceneGraph.push_back(debugCube2);

//HACKED perspective and object locations and size to avoid Z fighting
	//set up the camera
	//camera.SetupPerspective(openglContext.GetWindowWidth(), openglContext.GetWindowHeight(), 0.01f, 200.0f);  // Create our perspective matrix
	camera.SetupPerspective(WindowWidth, WindowHeight, 1000.0f, 27000000.0f); //was 1.0f near
	//camera.viewMatrix = openglContext.fitViewMatrix(); //not much point in doing this as there's nothing there yet
	camera.SetCameraPos(glm::dvec3(0.0f,0.0f,16000000.0f));
	//glm::vec3 vCam = camera.GetCameraPos();
	//std::cout<<"Globe Setup: camera ("<<vCam.x<<","<<vCam.y<<","<<vCam.z<<")"<<std::endl;
	_sdo = new SceneDataObject();
	_sdo->_camera=&camera;

	//set up events manager - should this be in the GC code?
	EventManager& eventmanager = EventManager::getInstance(); //singleton pattern
	eventmanager.SetWindow(GC->window); //initialise event system with OpenGL window handle
	//eventmanager.AddWindowSizeEventListener(this);

	//set up camera controller
	//controller = new OrbitController(&camera);
	//controller.centre=glm::vec3(-1.0f,0.0f,0.0f);
	//controller.centre=glm::vec3(-0.11464,51.46258,0); //Brixton
	controller = new EllipsoidOrbitController(&camera,&ellipsoid);
	controller->globe = this; //debug only

	//initialise last model run time to something
	_lastModelRunTime = glfwGetTime();
}


Globe::~Globe(void)
{
	//destroy opengl context
	//TODO: I really hope this destroys all the buffers..... ??????
	//openglContext.destroyContextGLFW();
	//glfwTerminate();

	DestroyScene();
	delete _sdo;

	//free the cached shader program objects
	for (vector<gengine::Shader*>::iterator it = _Shaders.begin(); it!=_Shaders.end(); ++it) {
		delete (*it);
	}

	delete controller;

	delete GC; //destroy graphics context and window
	OGLDevice::Destroy();
}

/// <summary>
/// Move the debug cube position on the globe. It's just a marker in the scene which we can use for testing purposes.
/// </summary>
void Globe::debugPositionCube(int Num, double X, double Y, double Z)
{
	switch (Num) {
		case 0: break; //it's only 1 or 2!!!
		case 1: debugCube1->SetPos(X,Y,Z); break;
		case 2: debugCube2->SetPos(X,Y,Z); break;
	}
}

/// <summary>
/// Return false if glfwWindowShouldClose(GC->window)
/// </summary>
/// <returns>True if the application should terminate due to the fact that the globe window has been closed by the user</returns>
bool Globe::IsRunning(void) {
	return !glfwWindowShouldClose(GC->window);
}

/// <summary>
/// Destroy all the objects in the scene and their associated buffers
/// </summary>
void Globe::DestroyScene(void) {
	for (vector<Object3D*>::iterator sceneIT=SceneGraph.begin(); sceneIT!=SceneGraph.end(); ++sceneIT) {
		delete (*sceneIT);
	}
	SceneGraph.clear();
}

/// <summary>
/// Load data from a GeoJSON file and add it to the globe's scene graph for rendering.
/// This is really a utility function as all it does is create a GeoJSON object, load the file and push the object
/// onto the scene graph and return it.
/// </summary>
/// <param name="Filename">The geojson file to load</param>
/// <returns>The GeoJSON object that has been added to the scene graph</returns>
GeoJSON* Globe::LoadLayerGeoJSON(std::string Filename)
{
	//create a geojson object, load and add to scene graph
	GeoJSON* geoj = new GeoJSON();
	geoj->LoadFile(Filename);
	//make it a random colour?
	//thames->SetColour(glm::vec3(0.0f,0.0f,1.0f)); //better make it blue
	
	//Take the first shader defined by the globe and attach to all the objects we've just created.
	//Presumably we know that the first defined shader is suitable?
	Shader* pShader = _Shaders[0];
	geoj->AttachShader(pShader,true);
	SceneGraph.push_back(geoj);
	return geoj;
}

void Globe::LoadLayerKML(std::string Filename)
{
	//TODO:
}

void Globe::LoadLayerShapefile(std::string Filename)
{
	//TODO:
}

/// <summary>
/// Add an ABM Model to the globe as a data layer that animates.
/// TODO: this is a real kludge as the model needs access to the scene graph in its constructor, which it gets from
/// the globe's GetSceneGraph. Then the model adds itself as a layer (which it doesn't really need to do as it's got
/// direct access to the scene graph). An ideal solution would be to just create the model and push it as a layer, not
/// worrying about any scene graph problems which are then handled by the globe.
/// </summary>
/// <param name="model">The model to push</param>
void Globe::AddLayerModel(ABM::Model* model)
{
	modelLayers.push_back(model);

	//attach shader to the top of the agents hierarchy
	Shader* pShader = _Shaders[0];
	model->_agents._pSceneRoot->AttachShader(pShader,true);

	//attach shader to the top of the links hierarchy
	//TODO: need to check how the links are actually working - suspect a netgraph geom is being attached to the scene - YES
	//TODO: check whether this is the same root as the agents? It shouldn't be really - they could have different shaders. - NO
	model->_links._pSceneRoot->AttachShader(pShader,true);
}

/// <summary>
/// Change the camera position and zoom so that all the data fits into the view
/// </summary>
void Globe::FitViewToLayers(void)
{
	//camera.viewMatrix = FitViewMatrix();
	camera.viewMatrix = FitViewMatrix2();
}

//recursive descent find object by name
Object3D* FindByName(std::string Name, Object3D* O3D)
{
	if (O3D->Name==Name)
	{
		return O3D;
	}
	else
	{
		//look at child objects in turn and if we get a hit return it
		for (vector<Object3D*>::const_iterator childIT=O3D->BeginChild(); childIT!=O3D->EndChild(); ++childIT)
		{
			Object3D* child = FindByName(Name,*childIT);
			if (child!=NULL) return child;
		}
	}
	return NULL;
}

/// <summary>
/// Look at a named object in the scene graph.
/// </summary>
/// <param name="Name">The name of the object which we want to fill the window.</param>
void Globe::LookAt(std::string Name)
{
	//step 1: find the object
	Object3D* O3D=NULL;
	for (vector<Object3D*>::iterator sceneIT=SceneGraph.begin(); sceneIT!=SceneGraph.end(); ++sceneIT) {
		O3D = FindByName(Name,*sceneIT);
		if (O3D!=NULL) break; //found it!
	}
	if (O3D==NULL) return; //not found

	cout<<"Zoom to "<<Name<<endl;
	//step 2: zoom to the bounds of the object
	BBox box = O3D->GetGeometryBounds();
	camera.viewMatrix = FitViewMatrix2(box);
}

/// <summary>
/// Return a view matrix which fits everything in the scene graph into the view.
/// Code borrowed from OpenGL4.cpp
/// </summary>
/// <returns>A view matrix that fits everything in the scene into the view.</returns>
glm::mat4 Globe::FitViewMatrix(void)
{
	float my_near = 1.0f; //0.1f; //near and far are #defined somewhere
	float my_far = 27000000.0f; //100.0f;

	int width, height;
	glfwGetFramebufferSize(GC->window, &width, &height);

	//walk the scene and union all the boxes
	BBox box;
	for (vector<Object3D*>::iterator sceneIT=SceneGraph.begin(); sceneIT!=SceneGraph.end(); ++sceneIT) {
		Object3D* o3d = *sceneIT;
		BBox geom_box = o3d->GetGeometryBounds();
		box.Union(geom_box); //this should return the bounds for the object and all its children
		//OutputDebugStringA("Node: ");
	}
	std::cout<<"View Box: "<<box.min.x<<","<<box.min.y<<"   "<<box.max.x<<","<<box.max.y<<std::endl;

	double x1=box.min.x,
		x2=box.max.x,
		y1=box.min.y,
		y2=box.max.y,
		z1=box.min.z,
		z2=box.max.z;
	//find centre of x, y and z axes which is the centre on the earth sphere (z is height)
	double cx=(x1+x2)/2;
	double cy=(y1+y2)/2;
	double cz=(z1+z2)/2;
	glm::dvec3 vc(cx,cy,cz); //vector from origin to point we want to look at on surface (centre)
	//calculate d distance from object, which guarantees all the max dimension of the scene box is within the width of the viewport
	double size = max(max(x2-x1,y2-y1),z2-z1);
	double fov = (double)width/2/tan(30.0*glm::pi<double>()/180.0);
	double d = size*fov/(double)width;
	////point the viewpoint from the origin to the centre of the objects
	//glm::mat4 eye_mat = glm::lookAt(glm::vec3(0,0,0),vc,glm::vec3(0,1,0)); //look from position 5 z back from object
	////move the viewpoint along its eye direction so that the distance from view (origin) to vc is d
	//glm::vec3 view_p = vc - glm::vec3(0,0,-d) * glm::mat3(eye_mat); //this is the position the viewpoint needs to be
	//view = glm::translate(view,view_p); //translate view to its new location

	//alternative, point along Z axis straight towards object, note view initialised to mat4(1) first
	glm::mat4 view(1);
	view = glm::translate(view, glm::vec3((float)-cx,(float)-cy,(float)(-cz-d)));

	return view;
}

/// <summary>
/// Second attempt at a decent view matrix fit based on data.
/// TODO: the field of view is hard coded - MUST get this from the projection matrix! (camera)
/// <summary>
glm::mat4 Globe::FitViewMatrix2(void)
{
	//int width, height;
	//glfwGetFramebufferSize(GC->window, &width, &height);

	//Step 1: 
	//walk the scene and union all the boxes
	BBox box;
	for (vector<Object3D*>::iterator sceneIT=SceneGraph.begin(); sceneIT!=SceneGraph.end(); ++sceneIT) {
		Object3D* o3d = *sceneIT;
		BBox geom_box = o3d->GetGeometryBounds();
		box.Union(geom_box); //this should return the bounds for the object and all its children
	}
	std::cout<<"View Box: "<<box.min.x<<","<<box.min.y<<"   "<<box.max.x<<","<<box.max.y<<std::endl;

	/*//find centre of box and maximum dimension
	float x1=box.min.x,
		x2=box.max.x,
		y1=box.min.y,
		y2=box.max.y,
		z1=box.min.z,
		z2=box.max.z;
	//find centre of x, y and z axes which is the centre on the earth sphere (z is height)
	float cx=(x1+x2)/2;
	float cy=(y1+y2)/2;
	float cz=(z1+z2)/2;
	glm::vec3 vc(cx,cy,cz); //vector from origin to point we want to look at (centre)
	float size = max(max(x2-x1,y2-y1),z2-z1);
	float fov = (float)width/2/tan(30.0*glm::pi<float>()/180.0);	//TODO: hardcoded projection matrix!
	float d = size*fov/(float)width;

	glm::vec3 vEye = vc + glm::normalize(vc)*d; //move eye to centre of object we want to look at, plus another d units along the origin to object centre vector
	glm::mat4 view = glm::lookAt(vEye,vc,glm::vec3(0,0,1)); //and look at the object from the new eye position - NOTE (0,0,1) up vector used as the Ellipse.toVector makes +ve Z UP (i.e. 90 degree rotation)
	return view;*/

	return FitViewMatrix2(box);
}

glm::mat4 Globe::FitViewMatrix2(BBox& box)
{
	int width, height;
	glfwGetFramebufferSize(GC->window, &width, &height);

	//find centre of box and maximum dimension
	double x1=box.min.x,
		x2=box.max.x,
		y1=box.min.y,
		y2=box.max.y,
		z1=box.min.z,
		z2=box.max.z;
	//find centre of x, y and z axes which is the centre on the earth sphere (z is height)
	double cx=(x1+x2)/2;
	double cy=(y1+y2)/2;
	double cz=(z1+z2)/2;
	glm::dvec3 vc(cx,cy,cz); //vector from origin to point we want to look at (centre)
	double size = max(max(x2-x1,y2-y1),z2-z1);
	double fov = (double)width/2/tan(30.0*glm::pi<double>()/180.0);	//TODO: hardcoded projection matrix!
	double d = size*fov/(double)width;

	glm::dvec3 vEye = vc + glm::normalize(vc)*d; //move eye to centre of object we want to look at, plus another d units along the origin to object centre vector
	glm::dmat4 view = glm::lookAt(vEye,vc,glm::dvec3(0,0,1)); //and look at the object from the new eye position - NOTE (0,0,1) up vector used as the Ellipse.toVector makes +ve Z UP (i.e. 90 degree rotation)
	return glm::mat4(view); //conversion down from dmat4 to mat4 - loss of precision
}

/// <summary>
/// Render the scene using the open GL context and the currently selected camera
/// Multi-frustum technique using f/n=1000 and n=[1,1000,1000000,1000000000]
/// TODO: the way this is supposed to work is that it holds a list of objects to draw in each frustum range and anything that is completely enclosed in that range is dropped, while
/// anything that splits two frustums is kept and checked next time around again. At this point you can also do the pyramid clip.
/// </summary>
void Globe::RenderScene(void)
{
	//glm::vec3 vCam = camera.GetCameraPos();
	//std::cout<<"Render: camera ("<<vCam.x<<","<<vCam.y<<","<<vCam.z<<")"<<std::endl;
	//openglContext.renderScene(&camera);

	//set render state
	//bind shaders and variables and buffers
	//setup matrices
	//camera?
	//fallback?

	//TODO: need to get this gl reference out!!!
	glClearColor(0.4f, 0.6f, 0.9f, 0.0f); // Set the clear color based on Microsoft's CornflowerBlue (default in XNA)
	GC->Clear();

	glm::dvec3 vCam = camera.GetCameraPos();
	for (double farClip=1e9; farClip>=1000; farClip/=1000)
	{
		//set up camera for this clip frustum based on f/n ratio of 1000
		//TODO: need some overlap between camera settings?
		double nearClip=farClip/1000;
		GC->ClearZ(); //reset the Z buffer for this range
		camera.SetupPerspective((int)camera._width,(int)camera._height,nearClip,farClip);
		//cout<<"Multi-frustum: near="<<farClip/1000<<" far="<<farClip<<endl;

		//this assumes all the matrices are right

		//go through all scene objects and render each in turn (modelMatrix should be identity really?)
		for (vector<Object3D*>::iterator sceneIT=SceneGraph.begin(); sceneIT!=SceneGraph.end(); ++sceneIT) {
			Object3D* o3d=(*sceneIT);
			if (o3d->HasGeometry()) {
				const DrawObject& dobj = o3d->GetDrawObject();
				glm::dvec3 P = glm::dvec3(dobj._ModelMatrix[3])-vCam;
				//OK, this is a box test, which is better, this, sqrt or a d^2 comparison with VERY large numbers? AND we might draw twice.
				if (((abs(P.x)>=nearClip)&&(abs(P.x)<=nearClip))||((abs(P.y)>=nearClip)&&(abs(P.y)<=farClip))||((abs(P.z)>=nearClip)&&(abs(P.z)<=farClip)))
					GC->Render(dobj,*_sdo);
			}
			RenderChildren(o3d, nearClip, farClip);
		}
	}

	////GC->ClearZ();

	//glClearColor(1, 1, 1, 1);
	//glClearDepth(0.0);
	//glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT|GL_ACCUM_BUFFER_BIT);

	/* Enable blending, necessary for our alpha texture */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GC->_FontShader->bind();
	
	//ShaderUniformCollection suc;
	//suc.BuildFromShaderProgram(_Shaders[2]->id());
	//glUniform1i(uniform_tex, 0);
	//suc.SetUniform1i("tex",0);
	
	//GLuint uniform_color = 0;
	//GLfloat red[4] = { 1, 0, 0, 1 };
	//glUniform4fv(uniform_color, 1, red);

	//GLuint vbo;
	//glGenBuffers(1, &vbo);
	//glBindBuffer(GL_ARRAY_BUFFER, vbo);
	
	float sx=2.0/512.0; float sy=2.0/512.0;
	GC->RenderText(_FontFace,"text text text text",-1.0+8.0*sx,1.0-50.0*sy,sx,sy);
	GC->RenderText(_FontFace,"text text text text",0,0,sx,sy);

	//glDeleteBuffers(1,&vbo);

	GC->_FontShader->unbind();

	GC->SwapBuffers();
}

/// <summary>
/// Render the child objects of an Object3D parent recursively i.e. render everything from this object down the hierarchy.
/// TODO: need to handle the matrix hierarchy properly - this doesn't
/// </summary>
/// <param name="Parent">The parent Object3D of this hierarchy</param>
/// <param name="nearClip">Only draw the object if its centre lies within near clip and far clip</param>
/// <param name="farClip">Only draw the object if its centre lies within near clip and far clip</param>
void Globe::RenderChildren(Object3D* Parent, double nearClip, double farClip)
{
	glm::dvec3 vCam = camera.GetCameraPos();
	for (vector<Object3D*>::const_iterator childIT=Parent->BeginChild(); childIT!=Parent->EndChild(); ++childIT) {
		Object3D* child = *childIT;
		if (child->HasGeometry()) {
			const DrawObject& dobj = child->GetDrawObject();
			glm::dvec3 P = glm::dvec3(dobj._ModelMatrix[3])-vCam;
			if (((abs(P.x)>=nearClip)&&(abs(P.x)<=nearClip))||((abs(P.y)>=nearClip)&&(abs(P.y)<=farClip))||((abs(P.z)>=nearClip)&&(abs(P.z)<=farClip)))
				GC->Render(dobj,*_sdo);
		}
		RenderChildren(child,nearClip,farClip);
	}
}

/// <summary>
/// Simulate one timestep of all the attached models. This works by storing the last time that the models 
/// </summary>
void Globe::Step(void)
{
	double timeNow = glfwGetTime();
	double delta = timeNow-_lastModelRunTime;
	if (delta<0.001) delta=0.001; //limit minimum delta time to 1ms, so you will never get any timesteps less than this e.g. on the first run
	//call step on each attached model layer in turn
	for (vector<ABM::Model*>::iterator modelIT = modelLayers.begin(); modelIT!=modelLayers.end(); ++modelIT) {
		(*modelIT)->Step(delta*10);
	}
	_lastModelRunTime = timeNow;
}



