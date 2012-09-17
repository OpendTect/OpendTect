#ifndef SoAxes_h
#define SoAxes_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		2009
 RCS:		$Id: SoAxes.h,v 1.3 2009/07/22 16:01:18 cvsbert Exp $
________________________________________________________________________

-*/

#include <Inventor/nodes/SoShape.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFFloat.h>
#include "soodbasic.h"

// SoAxes class for drawing coloured annotated axes
mClass SoAxes : public SoShape
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
