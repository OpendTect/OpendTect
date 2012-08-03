#ifndef SoAxes_h
#define SoAxes_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		2009
 RCS:		$Id: SoAxes.h,v 1.4 2012-08-03 13:00:39 cvskris Exp $
________________________________________________________________________

-*/

#include "soodmod.h"
#include <Inventor/nodes/SoShape.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFFloat.h>
#include "soodbasic.h"

// SoAxes class for drawing coloured annotated axes
mClass(SoOD) SoAxes : public SoShape
{
    typedef SoShape inherited;
    SO_NODE_HEADER(SoAxes);

public:
				SoAxes();
    static void			initClass();

    SoSFFloat			linelength_;
    SoSFFloat			baseradius_;
    SoSFColor			textcolor_;

protected:
    
    virtual			~SoAxes();
    virtual void		computeBBox(SoAction*,SbBox3f&,SbVec3f&);
    virtual void		generatePrimitives(SoAction*);
    virtual void		GLRender(SoGLRenderAction* action);
    void			drawArrow(int type,float length,float radius);
    void			drawSphere(float rad,float posx,
	    				   float posy,float posz );
};

#endif

