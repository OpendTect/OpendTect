#ifndef visinvisiblelinedragger_h
#define visinvisiblelinedragger_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		October 2007
 RCS:		$Id: visinvisiblelinedragger.h,v 1.3 2009-07-22 16:01:24 cvsbert Exp $
________________________________________________________________________


-*/

#include "visobject.h"


template <class T> class Interval;

class SoInvisibleLineDragger;
class SoDragger;
class SoCallbackList;

namespace visBase
{

/*!\brief

*/

mClass InvisibleLineDragger : public VisualObjectImpl
{
public:
    static InvisibleLineDragger*	create()
				mCreateDataObj(InvisibleLineDragger);

    Coord3			getStartPos() const;
    Coord3			getTranslation() const;
    void			setDirection(const Coord3&);

    void			setDisplayTransformation( Transformation* );
    Transformation*		getDisplayTransformation();

    void			setShape(DataObject*);

    Notifier<InvisibleLineDragger>  started;
    Notifier<InvisibleLineDragger>  motion;
    Notifier<InvisibleLineDragger>  finished;
    Notifier<InvisibleLineDragger>  needsDirection;

protected:
    				~InvisibleLineDragger();

    SoInvisibleLineDragger*	dragger_;
    DataObject*			shape_;

    Transformation*		transform_;

private:
    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );
    static void			finishCB( void*, SoDragger* );
    static void			needsDirectionCB(void*, void* );
};

};

#endif
