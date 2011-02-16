/*+
________________________________________________________________________

  (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
  Author:        Kristofer
  Date:          July 2004
________________________________________________________________________

-*/
static const char* rcsID = "$Id: SoOneSideRender.cc,v 1.1 2011-02-16 03:22:57 cvskris Exp $";

#include "SoOneSideRender.h"

#include <Inventor/SbLinear.h>
//#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
//#include <Inventor/bundles/SoTextureCoordinateBundle.h>
//#include <Inventor/bundles/SoMaterialBundle.h>
//#include <Inventor/details/SoFaceDetail.h>
//#include <Inventor/details/SoPointDetail.h>

//#include <Inventor/elements/SoCoordinateElement.h>
//#include <Inventor/elements/SoMaterialBindingElement.h>
//#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
//#include <Inventor/elements/SoTextureCoordinateBindingElement.h>
//#include <Inventor/elements/SoViewingMatrixElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
//#include <Inventor/elements/SoViewportRegionElement.h>
//#include <Inventor/system/gl.h>

//#include <Inventor/errors/SoDebugError.h>

//#include "SoCameraInfo.h"
//#include "SoCameraInfoElement.h"

SO_NODE_SOURCE(SoOneSideRender);

void SoOneSideRender::initClass()
{
    SO_NODE_INIT_CLASS(SoOneSideRender, SoNode, "Node");
    //SO_ENABLE( SoGLRenderAction, SoCameraInfoElement );
    //SO_ENABLE( SoGLRenderAction, SoModelMatrixElement );
    //SO_ENABLE( SoGLRenderAction, SoViewportRegionElement );
    //SO_ENABLE( SoGLRenderAction, SoViewVolumeElement );
    //SO_ENABLE( SoGLRenderAction, SoCacheElement );
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
    if ( idx>=nodes.getNum() || idx>=positions.getNum() || idx>=normals.getNum() )
	return false;

    const SbVec3f position = positions[idx];
    const SbVec3f normal = normals[idx];

    const SbViewVolume& vv = SoViewVolumeElement::get(state);
    const SbVec3f worldcamerapos = vv.getProjectionPoint();
    const SbMatrix& mat = SoModelMatrixElement::get(state).inverse();
    SbVec3f localcamerapos;
    mat.multVecMatrix( worldcamerapos, localcamerapos );

    const SbVec3f cameradir = localcamerapos-position;
    return cameradir.dot(normal)>=0;
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
	    nodes[idx]->func( action );			\
    }							\
}
    

mImplementFunc( GLRender, SoGLRenderAction);
mImplementFunc( callback, SoCallbackAction);
mImplementFunc( getBoundingBox, SoGetBoundingBoxAction);
mImplementFunc( getMatrix, SoGetMatrixAction);
mImplementFunc( handleEvent , SoHandleEventAction);
mImplementFunc( pick, SoPickAction);
mImplementFunc( rayPick, SoRayPickAction);
mImplementFunc( getPrimitiveCount, SoGetPrimitiveCountAction);

