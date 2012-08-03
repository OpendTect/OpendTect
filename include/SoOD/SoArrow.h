#ifndef SoArrow_h
#define SoArrow_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2003
 RCS:           $Id: SoArrow.h,v 1.4 2012-08-03 13:00:39 cvskris Exp $
________________________________________________________________________

-*/

#include "soodmod.h"
#include <Inventor/nodes/SoShape.h>
#include <Inventor/fields/SoSFFloat.h>

#include "soodbasic.h"


/*! \brief
#### Short description
\par
#### Detailed description.
*/


mClass(SoOD) SoArrow : public SoShape
{
    typedef SoShape inherited;
    SO_NODE_HEADER(SoArrow);

public:
    			SoArrow();
    static void		initClass();

    SoSFFloat		lineLength;
    SoSFFloat		lineWidth;
    SoSFFloat		headWidth;
    SoSFFloat		headHeight;
    
protected:
    
    virtual		~SoArrow();
    virtual void	computeBBox(SoAction*,SbBox3f&,SbVec3f&);
    virtual void	generatePrimitives(SoAction*);
    virtual void	GLRender(SoGLRenderAction* action);
};

#endif

