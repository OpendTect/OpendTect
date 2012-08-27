#ifndef SoInvisibleLineDragger_h
#define SoInvisibleLineDragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		October 2007
 RCS:		$Id: SoInvisibleLineDragger.h,v 1.7 2012-08-27 13:16:47 cvskris Exp $
________________________________________________________________________


-*/

#include "soodmod.h"
#include <Inventor/draggers/SoDragger.h>

#include "soodbasic.h"


class SbLineProjector;

/*!\brief
A line dragger with a shape given from outside. The shape is set by:
\code
    SoInvisibleLineDragger* dragger = new SoInvisibleLineDragger;
    dragger->ref();
    dragger->setPart( SoInvisibleLineDragger::sKeyShape(), new SoCube );
\endcode

When the shape is clicked on, the startPos will be set and the needsDirection
callbacks will be invoked. The outide world should then call the setDirection
function to set the direction of the drag:

\code
void MyClass::needsDirectionCB( void* data, void* d )
{
    SoInvisibleLineDragger* dragger = (SoInvisibleLineDragger*) d;
    SbVec3f dir = calculateDirection();
    dragger_->setDirection( dir );
}
\endcode

The class will not change the motionMatrix, so the shape itself will not be
moved. The class will not trigger the valueChanged callback on the baseclass -
use the Start, Motion and Finish callbacks. The dragged translation is
read in SoInvisibleLineDragger::translation following a Motion or Start
callback.
*/

mSoODClass SoInvisibleLineDragger : public SoDragger
{
    typedef SoDragger inherited;
    SO_KIT_HEADER(SoInvisibleLineDragger);

    SO_KIT_CATALOG_ENTRY_HEADER(shape);

public:
    static void		initClass();
    			SoInvisibleLineDragger();

    SbVec3f		translation;

    SoCallbackList	needsDirection;
    SbVec3f		startPos;

    void		setDirection(const SbVec3f&);

    static const char*	sKeyShape() { return "shape"; }

protected:
    			~SoInvisibleLineDragger();

    void		dragStart(void);
    void		drag(void);

private:

    static void		startCB(void* f, SoDragger * d);
    static void		motionCB(void* f, SoDragger * d);

    SbLineProjector*	lineProj_;
};

#endif

