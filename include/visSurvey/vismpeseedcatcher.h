#ifndef vismpeseedcatcher_h
#define vismpeseedcatcher_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismpeseedcatcher.h,v 1.2 2005-09-19 21:50:34 cvskris Exp $
________________________________________________________________________


-*/

#include "visobject.h"

#include "emposid.h"
#include "geomelement.h"


namespace Geometry { class ElementEditor; };
namespace MPE { class ObjectEditor; };
namespace EM { class EdgeLineSet; }

namespace visBase
{

class Marker;
class Dragger;
};


namespace visSurvey
{

/*!\brief
*/


class MPEClickCatcher : public visBase::VisualObjectImpl
{
public:
    static MPEClickCatcher*	create()
			mCreateDataObj( MPEClickCatcher );

    void		setSceneEventCatcher( visBase::EventCatcher* );
    void		setDisplayTransformation( mVisTrans* );
    mVisTrans*		getDisplayTransformation() {return transformation;}

    Notifier<MPEClickCatcher>	click;
    const Coord3&		clickedPos() const;
    int				clickedObjectID() const { return clickedobjid; }

protected:
    				~MPEClickCatcher();
    void			clickCB( CallBacker* );

    const visBase::EventInfo*	eventinfo_;
    int				clickedobjid;

    visBase::EventCatcher*	eventcatcher;
    visBase::Transformation*	transformation;
};

};

#endif
