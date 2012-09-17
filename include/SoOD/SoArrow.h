#ifndef SoArrow_h
#define SoArrow_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          November 2003
 RCS:           $Id: SoArrow.h,v 1.3 2009/07/22 16:01:18 cvsbert Exp $
________________________________________________________________________

-*/

#include <Inventor/nodes/SoShape.h>
#include <Inventor/fields/SoSFFloat.h>

#include "soodbasic.h"


/*! \brief
#### Short description
\par
#### Detailed description.
*/


mClass SoArrow : public SoShape
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
