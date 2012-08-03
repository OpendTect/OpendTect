#ifndef SoLineSet3D_h
#define SoLineSet3D_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		March 2011
 RCS:		$Id: SoLineSet3D.h,v 1.2 2012-08-03 13:00:40 cvskris Exp $
________________________________________________________________________


-*/

#include "soodmod.h"
#include <Inventor/nodes/SoNonIndexedShape.h>

#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/elements/SoGLDisplayList.h>

#include "soodbasic.h"
#include "SoIndexedLineSet3D.h"

//#define USE_DISPLAYLIST_LINESET

class SbBox3f;
class SbVec3f;

mClass(SoOD) SoLineSet3D : public SoNonIndexedShape
{
    SO_NODE_HEADER(SoLineSet3D);
public:

    static void		initClass();
    			SoLineSet3D();

    SoMFInt32		numVertices;

    SoSFFloat		radius;
    SoSFBool		screenSize;
    			//!<Specifies wether radius is on screen or in display
			//!<Coordinates.
    SoSFFloat		maxRadius;
    			//!<Specifies maximum world radius if screenSize
			//!<is enabled. Less than zero means there is no
			//!<maximum
			//!<Coordinates.

protected:
    			~SoLineSet3D();
    void		generatePrimitives(SoAction*);
    void		GLRender(SoGLRenderAction*);
    void		rayPick (SoRayPickAction *action);
    void		computeBBox(SoAction*, SbBox3f&, SbVec3f&);

private:

    SoIndexedLineSet3D::LineSet3DData		data_;
};


#endif

