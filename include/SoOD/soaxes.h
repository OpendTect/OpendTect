#ifndef SoAxes_h
#define SoAxes_h

#include <Inventor/nodes/SoShape.h>
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

    SoSFFloat			lineLength;
    SoSFFloat			baseRadius;
    
protected:
    
    virtual			~SoAxes();
    virtual void		computeBBox(SoAction*,SbBox3f&,SbVec3f&);
    virtual void		generatePrimitives(SoAction*);
    virtual void		GLRender(SoGLRenderAction* action);
    void			drawArrow(int type,float length,float radius);
    void			drawSphere(float rad,float posx,float posy,float posz );
};

#endif
