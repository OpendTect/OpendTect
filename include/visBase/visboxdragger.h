#ifndef visboxdragger_h
#define visboxdragger_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: visboxdragger.h,v 1.2 2002-10-14 14:25:26 niclas Exp $
________________________________________________________________________


-*/

#include "visobject.h"
#include "position.h"

class SoTranslation;
class SoTabBoxDragger;
class SoDragger;

namespace Geometry { class Pos; };

namespace visBase
{

class BoxDragger : public SceneObject
{
public:
    static BoxDragger*		create()
				mCreateDataObj0arg(BoxDragger);

    void			setCenter(const Coord3&);
    Coord3			center() const;
    
    void			setScale(const Coord3&);
    Coord3			scale() const;

    Notifier<BoxDragger>	started;
    Notifier<BoxDragger>	motion;
    Notifier<BoxDragger>	changed;
    Notifier<BoxDragger>	finished;

    SoNode*			getData();

protected:
				~BoxDragger();

    static void			startCB( void*, SoDragger* );
    static void			motionCB( void*, SoDragger* );
    static void			valueChangedCB(void*, SoDragger* );
    static void			finishCB( void*, SoDragger* );

    SoSeparator*		root;
    SoTabBoxDragger*		boxdragger;
};

};
	
#endif
