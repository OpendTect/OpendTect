/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoForegroundTranslation.cc,v 1.6 2009/07/22 16:01:35 cvsbert Exp $";


#include "SoForegroundTranslation.h"

#include "Inventor/actions/SoGLRenderAction.h"
#include "Inventor/actions/SoGetBoundingBoxAction.h"
#include "Inventor/actions/SoCallbackAction.h"
#include "Inventor/actions/SoGetMatrixAction.h"
#include "Inventor/actions/SoPickAction.h"
#include "Inventor/actions/SoGetPrimitiveCountAction.h"
#include "Inventor/elements/SoModelMatrixElement.h"
#include "Inventor/elements/SoViewVolumeElement.h"


SO_NODE_SOURCE(SoForegroundTranslation);

void SoForegroundTranslation::initClass()
{
    SO_NODE_INIT_CLASS(SoForegroundTranslation, SoNode, "Node" );

    SO_ENABLE(SoGLRenderAction, SoModelMatrixElement );
    SO_ENABLE(SoGLRenderAction, SoViewVolumeElement );

    SO_ENABLE(SoGetBoundingBoxAction, SoModelMatrixElement );
    SO_ENABLE(SoGetBoundingBoxAction, SoViewVolumeElement );

    SO_ENABLE(SoCallbackAction, SoModelMatrixElement );
    SO_ENABLE(SoCallbackAction, SoViewVolumeElement );

    SO_ENABLE(SoGetMatrixAction, SoModelMatrixElement );
    SO_ENABLE(SoGetMatrixAction, SoViewVolumeElement );

    SO_ENABLE(SoPickAction, SoModelMatrixElement );
    SO_ENABLE(SoPickAction, SoViewVolumeElement );

    SO_ENABLE(SoGetPrimitiveCountAction, SoModelMatrixElement );
    SO_ENABLE(SoGetPrimitiveCountAction, SoViewVolumeElement );
}


SoForegroundTranslation::SoForegroundTranslation()
{
    SO_NODE_CONSTRUCTOR(SoForegroundTranslation);
    SO_NODE_ADD_FIELD( lift, ( 1 ) );
}


void SoForegroundTranslation::doAction( SoAction* action )
{
    const float liftvalue = lift.getValue();
    if ( !liftvalue ) return;

    SoState* state = action->getState();
    const SbViewVolume& vv = SoViewVolumeElement::get(state);
    const SbMatrix& mat = SoModelMatrixElement::get(state);

    SbVec3f projectiondir = vv.getProjectionDirection();
    projectiondir.normalize();
    projectiondir *=-liftvalue;

    SbVec3f localprojdir;
    mat.inverse().multDirMatrix( projectiondir, localprojdir );
    
    SoModelMatrixElement::translateBy(state, this,localprojdir);
}

    
#define mImplAction( func, actiontype ) \
void SoForegroundTranslation::func( actiontype* action ) \
{ \
    SoForegroundTranslation::doAction( action ); \
}


mImplAction( GLRender, SoGLRenderAction );
mImplAction( getBoundingBox, SoGetBoundingBoxAction );
mImplAction( callback, SoCallbackAction );
mImplAction( getMatrix, SoGetMatrixAction );
mImplAction( pick, SoPickAction );
mImplAction( getPrimitiveCount, SoGetPrimitiveCountAction );



