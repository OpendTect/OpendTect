#ifndef visboxdragger_h
#define visboxdragger_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	N. Hemstra
 Date:		August 2002
 RCS:		$Id: visboxdragger.h,v 1.1 2002-08-20 07:34:53 nanne Exp $
________________________________________________________________________


-*/

#include "visobject.h"

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

    void			setCenter(const Geometry::Pos&);
    Geometry::Pos		center() const;
    
    void			setScale(const Geometry::Pos&);
    Geometry::Pos		scale() const;

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
