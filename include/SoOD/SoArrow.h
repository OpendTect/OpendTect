#ifndef SoArrow_h
#define SoArrow_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          November 2003
 RCS:           $Id: SoArrow.h,v 1.1 2003-11-28 15:39:11 nanne Exp $
________________________________________________________________________

-*/

#include <Inventor/nodes/SoShape.h>
#include <Inventor/fields/SoSFFloat.h>


/*! \brief
#### Short description
\par
#### Detailed description.
*/


class SoArrow : public SoShape
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
