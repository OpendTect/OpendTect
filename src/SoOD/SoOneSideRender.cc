/*+
________________________________________________________________________

  (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  Author:        Kristofer
  Date:          July 2004
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "SoOneSideRender.h"

#include <Inventor/SbLinear.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>

#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>


SO_NODE_SOURCE(SoOneSideRender);

void SoOneSideRender::initClass()
{
    SO_NODE_INIT_CLASS(SoOneSideRender, SoNode, "Node");
    SO_ENABLE( SoGLRenderAction, SoModelMatrixElement );
    SO_ENABLE( SoGLRenderAction, SoViewVolumeElement );
}


SoOneSideRender::SoOneSideRender()
{
    SO_NODE_CONSTRUCTOR(SoOneSideRender);
    SO_NODE_ADD_FIELD( nodes, (0) );
    SO_NODE_ADD_FIELD( positions, (0,0,0) );
    SO_NODE_ADD_FIELD( normals, (0,0,1) );
}


bool SoOneSideRender::shouldRender( int idx, SoState* state ) const
{
    if ( idx>=nodes.getNum() || idx>=positions.getNum() ||
	 idx>=normals.getNum() || !nodes[idx] )
	return false;

    const SbVec3f normal = normals[idx];
    const SbViewVolume& vv = SoViewVolumeElement::get(state);
    const SbMatrix& mat = SoModelMatrixElement::get(state).inverse();

    if ( vv.getProjectionType()==SbViewVolume::PERSPECTIVE )
    {
	const SbVec3f worldcamerapos = vv.getProjectionPoint();
	SbVec3f localcamerapos;
	mat.multVecMatrix( worldcamerapos, localcamerapos );

	const SbVec3f position = positions[idx];
	const SbVec3f cameradir = localcamerapos-position;
	return cameradir.dot(normal)>=0;
    }

    //Orthographic
    const SbVec3f& worldprojdir = vv.getProjectionDirection();
    SbVec3f localprojdir;
    mat.multDirMatrix( worldprojdir, localprojdir );
    return localprojdir.dot(normal)<=0;
}

#define mImplementFunc( func, actiontype )		\
void SoOneSideRender::func( actiontype* action )	\
{							\
    SoState* state = action->getState();		\
    const int nrnodes = nodes.getNum();			\
							\
    for ( int idx=0; idx<nrnodes; idx++ )		\
    {							\
	if ( shouldRender( idx, state ) )		\
	    action->traverse( nodes[idx] );		\
    }							\
}
    

mImplementFunc( GLRender, SoGLRenderAction);
mImplementFunc( callback, SoCallbackAction);
mImplementFunc( getMatrix, SoGetMatrixAction);
mImplementFunc( handleEvent , SoHandleEventAction);
mImplementFunc( pick, SoPickAction);
mImplementFunc( rayPick, SoRayPickAction);
mImplementFunc( getPrimitiveCount, SoGetPrimitiveCountAction);



void SoOneSideRender::getBoundingBox( SoGetBoundingBoxAction* action )
{
    int nrnodes = nodes.getNum();	

    int numindices;
    const int * indices;

    if (action->getPathCode(numindices, indices) == SoAction::IN_PATH)
	nrnodes = indices[numindices-1]+1;

    SoState* state = action->getState();

    // Initialize accumulation variables.
    SbVec3f acccenter(0.0f, 0.0f, 0.0f);
    int numcenters = 0;

    for ( int idx=0; idx<nrnodes; idx++ )
    {
	if ( !shouldRender( idx, state ) )
	    continue;

	action->traverse( nodes[idx] );
	if ( action->isCenterSet() )
	{
	    acccenter += action->getCenter();
	    numcenters++;
	    action->resetCenter();
    	}
    }

    if ( numcenters != 0 )
	action->setCenter(acccenter / float(numcenters), FALSE);
}
