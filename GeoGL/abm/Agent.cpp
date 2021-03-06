
#include "Agent.h"
#include "Agents.h"
#include "Link.h"
#include "LogoVariantOwns.h"
#include "agentobject.h"

#include "graph.h"

#include <list>

namespace ABM {

	//static declarations
	Agents* Agent::_pParentAgents;

	//class

	/// <summary>
	/// Constructor
	/// This could be tricky as an Agent can't create itself. It needs the Agents class to do it so that the list of agents is complete.
	/// More generally, agents are hatched out of other agents anyway?
	///</summary>
	Agent::Agent(void)
	{
		_GVertex = NULL; //agents aren't linked to the graph until you use a link function - no point in having useless disconnected vertices
	}

	/// <summary>
	/// Destructor
	/// Can an agent actually kill itself?
	/// </summary>
	Agent::~Agent(void)
	{
		_pParentAgents->Die(this);
	}



	//I might change my mind about this - all agents should have a pointer to their Mesh in the scene as anything else is
	//a performance problem.
	/// <summary>
	/// This is called when an agent is first created. It goes through the scene graph (children of the agent root) to find this agent.
	/// If it doesn't exist (by name), then a new mesh is added. The _pAgentMesh private is updated to keep the link between this agent and the 3D scene.
	/// </summary>
	//void Agent::SynchroniseWithSceneGraph(Object3D* pAgentsRoot)
	//{
	//	//TODO: need an iterator for this!
	//	//for (std::vector<Object3D*>::iterator it=pAgentsRoot->Children.begin(); it!=pAgentsRoot->Children.end(); ++it) {
	//	//}
	//}


	/// <summary>
	/// Return the agent's xyz position vector. This has been changed from returning the mesh position so that agents without a mesh can have a position.
	/// Only the SetPos now updates the mesh position to keep the two in step.
	/// </summary>
	glm::dvec3 Agent::GetXYZ()
	{
		//double X,Y,Z;
		//_pAgentMesh->GetPos(X,Y,Z);
		//return glm::dvec3(X,Y,Z);
		return position;
	}

	/// <summary>
	/// Return the agent's x coordinate
	/// </summary>
	double Agent::xcor()
	{
		//double X,Y,Z;
		//_pAgentMesh->GetPos(X,Y,Z);
		//return X;
		return position.x;
	}

	/// <summary>
	/// Return the agent's y coordinate
	/// </summary>
	double Agent::ycor()
	{
		//double X,Y,Z;
		//_pAgentMesh->GetPos(X,Y,Z);
		//return Y;
		return position.y;
	}

	/// <summary>
	/// Return the agent's z coordinate. This has been added as NetLogo doesn't have 3D coordinates
	/// </summary>
	double Agent::zcor()
	{
		//double X,Y,Z;
		//_pAgentMesh->GetPos(X,Y,Z);
		//return Z;
		return position.z;
	}

	/// <summary>
	/// Set the position of an agent in 3D space. As NetLogo was only 2D, the original function was called SetXY, so I've added the Z
	/// </summary>
	/// <param name="X"></param>
	/// <param name="Y"></param>
	/// <param name="Z"></param>
	void Agent::SetXYZ(const double X, const double Y, const double Z)
	{
		//hold XYZ locally as well as updating the mesh - that way agents without a mesh can have position
		position = glm::dvec3(X,Y,Z);
		if (_pAgentMesh)
			_pAgentMesh->SetPos(X,Y,Z);
	}

	/// <summary>
	/// Set a new colour for the agent mesh.
	/// This isn't as simple as you might think as the entire vertex colour buffer needs to be changed.
	/// Also, it's dependent on the shape how many colours you need.
	/// TODO: you might need to set multiple colours here as faces of some shapes can be different colours
	/// </summary>
	/// <param name="new_colour"></param>
	void Agent::SetColour(glm::vec3 new_colour)
	{
		//get at the mesh object in the scene
		if (_colour!=new_colour) { //it's computationally expensive to change the buffers, so only do it if you really need to
			_colour=new_colour;
			if (_pAgentMesh) _pAgentMesh->SetColour(new_colour); //set colour on the mesh, not if it doesn't have one
		}
	}

	/// <summary>
	/// </summary>
	/// <returns></returns>
	glm::vec3 Agent::GetColour()
	{
		return _colour;
	}

	/// <summary>
	/// Move the turtle forward the specified amount
	/// </summary>
	/// <param name="d">Distance to move</param>
	void Agent::Forward(float d)
	{
		//new code which can handle the absence of a model matrix (i.e. no mesh)
		//set the position on the agent matrix
		//direct manipulation of position
		agentMatrix[3][0]=(float)position.x;
		agentMatrix[3][1]=(float)position.y;
		agentMatrix[3][2]=(float)position.z;

		//original code
		//agentMatrix = glm::translate(agentMatrix,glm::vec3(0,0,-d));
		//now get the position back
		//position.x=(float)agentMatrix[3][0];
		//position.y=(float)agentMatrix[3][1];
		//position.z=(float)agentMatrix[3][2];


		//new
		glm::vec4 f = agentMatrix * glm::vec4(0,0,-d,0);
		position.x=position.x+f.x; position.y=position.y+f.y; position.z=position.z+f.z;
		agentMatrix[3][0]=position.x;
		agentMatrix[3][1]=position.y;
		agentMatrix[3][2]=position.z;
		//end new




		//and set the mesh if it exists
		if (_pAgentMesh)
			_pAgentMesh->SetMatrix(agentMatrix);

		//original code
		//TODO: need to handle pAgentMesh null - THIS IS A HACK - need an agent matrix to hold the rotation in addition to a position
		//_pAgentMesh->SetPos(position.x,position.y,position.z); //added
		//_pAgentMesh->modelMatrix = glm::translate(_pAgentMesh->modelMatrix,glm::vec3(0,0,-d)); //original line
		//_pAgentMesh->GetPos(position.x,position.y,position.z); //added
	}
	
	/// <summary>
	/// Move the turtle backwards the specified amount
	/// </summary>
	/// <param name="d">Distance to move</param>
	void Agent::Back(float d)
	{
		//TODO: need to handle pAgentMesh null
		//_pAgentMesh->modelMatrix = glm::translate(_pAgentMesh->modelMatrix,glm::vec3(0,0,d));

		//OK, this is cheating Back=-Forward obviously
		Forward(-d);
	}
	
	/// <summary>
	/// Move the turtle left the specified amount
	/// </summary>
	/// <param name="d">Distance to move</param>
	void Agent::Left(float d)
	{
		//TODO: need to handle pAgentMesh null
		agentMatrix = glm::translate(agentMatrix,glm::vec3(-d,0,0));
		_pAgentMesh->SetMatrix(agentMatrix);

		//old code
		//_pAgentMesh->modelMatrix = glm::translate(_pAgentMesh->modelMatrix,glm::vec3(-d,0,0));
	}
	
	/// <summary>
	/// Move the turtle right the specified amount
	/// </summary>
	/// <param name="d">Distance to move</param>
	void Agent::Right(float d)
	{
		//TODO: need to handle pAgentMesh null
		agentMatrix = glm::translate(agentMatrix,glm::vec3(d,0,0));
		_pAgentMesh->SetMatrix(agentMatrix);

		//old code
		//_pAgentMesh->modelMatrix = glm::translate(_pAgentMesh->modelMatrix,glm::vec3(d,0,0));
	}
	
	/// <summary>
	/// Move the turtle up the specified amount
	/// </summary>
	/// <param name="d">Distance to move</param>
	void Agent::Up(float d)
	{
		//TODO: need to handle pAgentMesh null
		//added this - netlogo isn't 3d
		agentMatrix = glm::translate(agentMatrix,glm::vec3(0,d,0));
		_pAgentMesh->SetMatrix(agentMatrix);

		//old code
		//_pAgentMesh->modelMatrix = glm::translate(_pAgentMesh->modelMatrix,glm::vec3(0,d,0));
	}
	
	/// <summary>
	/// Move the turtle down the specified amount
	/// </summary>
	/// <param name="d">Distance to move</param>
	void Agent::Down(float d)
	{
		//TODO: need to handle pAgentMesh null
		//added this - netlogo isn't 3d
		agentMatrix = glm::translate(agentMatrix,glm::vec3(0,-d,0));
		_pAgentMesh->SetMatrix(agentMatrix);

		//old code
		//_pAgentMesh->modelMatrix = glm::translate(_pAgentMesh->modelMatrix,glm::vec3(0,-d,0));
	}

	/// <summary>
	/// Move this agent to the location of the parameter agent
	/// <summary>
	/// <param name="A">The agent that this one is to move to</param>
	void Agent::MoveTo(Agent& A)
	{
		double x=A.xcor();
		double y=A.ycor();
		double z=A.zcor();
		SetXYZ(x,y,z);
	}

	/// <summary>
	/// Face this agent so that it's looking at the parameter agent.
	/// ***Assumes agents exist on xy plane with up in +ve z direction
	/// </summary>
	/// <param name="A">The agent that this one is going to look at</param>
	void Agent::Face(Agent& A)
	{
		//following assumes agent actually has a mesh that we can get the matrix from
		glm::dvec3 P1 = GetXYZ(); //this is me
		glm::dvec3 P2 = A.GetXYZ(); //this is who I want to look at

		//HACK!
		if (glm::distance(P1,P2)<0.000000001f) return; //error, asked to face an agent that I'm virtually on top of

		//_pAgentMesh->modelMatrix = glm::lookAt(P1,P2,glm::vec3(0,0,1)); //assumes agents exist on xy plane with up in +ve z direction
		glm::vec3 f(glm::normalize(P2-P1)); //center - eye
		glm::vec3 s(glm::normalize(glm::cross(f, glm::vec3(0,0,1)))); //(0,0,1)=up
		glm::vec3 u(glm::cross(s, f));

		//transpose of view matrix glm::lookAt calculation and keep the position as P1
		glm::mat4 Result(1);
		Result[0][0] = s.x;
		Result[0][1] = s.y;
		Result[0][2] = s.z;
		Result[1][0] = u.x;
		Result[1][1] = u.y;
		Result[1][2] = u.z;
		Result[2][0] =-f.x;
		Result[2][1] =-f.y;
		Result[2][2] =-f.z;
		//Result[3][0] =-dot(s, eye);
		//Result[3][1] =-dot(u, eye);
		//Result[3][2] = dot(f, eye);
		Result[3][0]=(float)P1.x;
		Result[3][1]=(float)P1.y;
		Result[3][2]=(float)P1.z;
		agentMatrix = Result;
		if (_pAgentMesh)
			_pAgentMesh->SetMatrix(Result); //and set the mesh matrix if there is actually a mesh
	}

	/// <summary>
	/// Hatch N new agents from this one by using the Agents parent pointer to do the actual creation i.e. they're not
	/// actually created at this level, but moved up to the parent.
	/// </summary>
	/// <param name="N">Number of new agents to create</param>
	/// <param name="BreedName">The breed name</param>
	/// <returns>A vector of new agents</returns>
	std::vector<Agent*> Agent::Hatch(int N, std::string BreedName)
	{
		glm::dvec3 P = GetXYZ(); //from the 3D mesh
		std::vector<Agent*> NewAgents;
		for (int i=0; i<N; i++) {
			Agent* a = _pParentAgents->Hatch(BreedName);
			//TODO: you're supposed to clone the params of the parent agent
			a->SetXYZ(P.x,P.y,P.z);
			NewAgents.push_back(a);
		}
		return NewAgents;
	}

	/// <summary>
	/// Remove the agent - don't forget to remove it from the global agent list and force the destructor
	/// <summary>
	void Agent::Die()
	{
		delete this;
	}

	/// <summary>
	/// Get all agents who are linked to this one
	/// </summary>
	/// <returns>A vector of all links coming into this agent</returns>
	std::vector<Link*> Agent::InLinks()
	{
		//get the vertex, then construct a list of Links from InEdges and the UserData
		//Agent contains a pointer to the graph vertex (_GVertex) which gives us a list of edges (in both directions, but we only want in edges).
		//The Graph Edges contain a pointer to the ABM::Link, so we just de-reference that.
		//ABM::Links therefore shadow the Graph structure where the topology is actually stored. Links are only for storing additional
		//data on the links.
		std::vector<Link*> InLinks;
	
		Vertex* V = _GVertex;
		if (V!=NULL) {
			for (std::list<Edge*>::iterator edgeIT = V->_InEdges.begin(); edgeIT!=V->_InEdges.end(); ++edgeIT) {
				Edge* E=*edgeIT;
			
				Link* L = (Link*)E->_UserData;
				//push the link
				InLinks.push_back(L);
			}
		}

		return InLinks;
	}

	/// <summary>
	/// Get all agents who are linked from this one
	/// </summary>
	/// <returns>A vector of all links going out of this agent</returns>
	std::vector<Link*> Agent::OutLinks()
	{
		//see InLinks
		std::vector<Link*> OutLinks;
	
		Vertex* V = _GVertex;
		if (V != NULL) {
			for (std::list<Edge*>::iterator edgeIT = V->_OutEdges.begin(); edgeIT != V->_OutEdges.end(); ++edgeIT) {
				Edge* E = *edgeIT;

				Link* L = (Link*)E->_UserData;
				//push the link
				OutLinks.push_back(L);
			}
		}

		return OutLinks;
	}

	/// <summary>
	/// Calculate the distance between this agent and the other one.
	/// TODO: this is simple Euclidean - need to include CRS in a proper calculation?
	/// </summary>
	double Agent::Distance(Agent& A)
	{
		glm::dvec3 P1 = GetXYZ();
		glm::dvec3 P2 = A.GetXYZ();
		//if ((P1.x!=P1.x)||(P2.x!=P2.x)) {
		//	std::cout<<"NaN Trap"<<std::endl;
		//	return 0;
		//}
		//if ((P1.x-P2.x+P1.y-P2.y+P1.z-P2.z)<glm::epsilon<double>()) return 0;
		return glm::distance<double>(P1,P2);
	}

	/// <summary>
	/// Returns true if the agent is within a box Distance units around this agent (actually, within (-D,-D,-D) and (+D,+D,+D), so 2*Distance )
	/// </summary>
	bool Agent::InBox(Agent& A,double Distance)
	{
		double dx = abs(A.position.x-position.x);
		double dy = abs(A.position.y-position.y);
		double dz = abs(A.position.z-position.z);
		return (dx<=Distance)||(dy<=Distance)||(dz<=Distance);
	}

}
