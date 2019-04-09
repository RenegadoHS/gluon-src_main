// VertexAnimation.h: interface for the CVertexAnimation class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VERTEXANIMATION_H__C76EA445_1CC8_4AF7_A3B2_D4B4420F855A__INCLUDED_)
#define AFX_VERTEXANIMATION_H__C76EA445_1CC8_4AF7_A3B2_D4B4420F855A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include <SIBCUtil.h>
#include <SIBCString.h>
#include <SIBCArray.h>
#include <SIBCVector.h>
#include <SIBCMatrix.h>

#include <Mesh.h>
#include <BaseShape.h>

#include "SMDNode.h"

typedef struct _tagExplodedVertex
{
	CSIBCVector3D	m_vPosition;
	CSIBCVector3D	m_vNormal;
} ExplodedVertex;


/** 
* CShapeAnimationFrame object
*
* Holds the vertex + normal deltas between two shapes for a given frame
*
*/
class CShapeAnimationFrame
{
public:
	CShapeAnimationFrame();
	~CShapeAnimationFrame();

	/*!	Sets the frame time
	\param	SI_Float	Time ( frames )
	\return	nothing
	*/
	void		SetTime			(float in_fTime) { m_fTime = in_fTime; };

	/*!	Builds a list of vertex and normals that represents the delta between the 2 shapes passed
	\param	CSLBaseShape*	Semantic Layer BaseShape object 
	\param	CSLBaseShape*	Semantic Layer BaseShape object 
	\return	nothing
	*/
	void		ComputeDelta	( CSLMesh*	in_pMesh, CSLBaseShape* in_pLastFrame, CSLBaseShape* in_pFrame );

	/*!	Writes the lists of vertex and normals for that frame to a file
	\param	FILE*	File pointer
	\return	nothing
	*/
	SI_Error	Write ( FILE* l_fptr, CSIBCMatrix4x4 in_mGMatrix );

	void		ExplodeMesh	( CSLMesh*	in_pMesh, CSLBaseShape* in_pLastFrame, CSLBaseShape* in_pFrame, CSIBCArray<ExplodedVertex>* in_pE1, CSIBCArray<ExplodedVertex>* in_pE2 );


private:

	SI_Float	m_fTime;
	CSIBCArray<int>				m_pIndexList;
	CSIBCArray<CSIBCVector3D>	m_pVertexList;
	CSIBCArray<CSIBCVector3D>	m_pNormalList;

};

/** 
* CVertexAnimation object
*
* Holds a list of CShapeAnimationFrame objects
*
*/
class CVertexAnimation  
{
public:
	CVertexAnimation();
	virtual ~CVertexAnimation();

	/*!	Adds a single ShapeAnimation frame to the list
	\param	SI_Float	Time ( frames )
	\param	CSLBaseShape*	Semantic Layer BaseShape object 
	\param	CSLBaseShape*	Semantic Layer BaseShape object 
	\return	nothing
	*/
	void	AddAnimationFrame( float in_fTime, CSLMesh*	in_pMesh, CSLBaseShape*	l_pShape1, CSLBaseShape*	l_pShape2 ); 

	/*!	Gets the number of frames in the sequence
	\return	int
	*/
	int	GetNbFrames () { return m_pFrames.GetUsed(); };

	/*!	Sets the CSLModel which has shapeanimation
	\param	CSLModel*	The model
	*/
	void SetModel ( CSLModel* in_pModel ) { m_pModel = in_pModel ; };



	/*!	Loops and calls CShapeAnimationFrame::Write for the entire list of CShapeAnimationFrame
	\param	FILE*	File pointer
	\return	nothing
	*/
	SI_Error	Write ( FILE* l_fptr, SMDNodeList* );

private:


	CSIBCArray<CShapeAnimationFrame*>	m_pFrames;

	CSLModel	*m_pModel;
	

};

#endif // !defined(AFX_VERTEXANIMATION_H__C76EA445_1CC8_4AF7_A3B2_D4B4420F855A__INCLUDED_)
