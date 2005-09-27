#ifndef vismpeseedcatcher_h
#define vismpeseedcatcher_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismpeseedcatcher.h,v 1.3 2005-09-27 21:56:26 cvskris Exp $
________________________________________________________________________


-*/

#include "visobject.h"

#include "cubesampling.h"
#include "emposid.h"
#include "geomelement.h"


namespace Geometry { class ElementEditor; };
namespace MPE { class ObjectEditor; };
namespace EM { class EdgeLineSet; }
namespace Attrib { class SelSpec; class SliceSet; }

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
    const CubeSampling&		clickedObjectCS() const { return clickedcs; }
    const Attrib::SliceSet*	clickedObjectData() const { return sliceset; }
    const Attrib::SelSpec*	clicedObjectDataSelSpec() const { return as; }

protected:
    				~MPEClickCatcher();
    void			clickCB( CallBacker* );

    void			sendClickEvent( const Coord3&, int objid,
						const CubeSampling& cs,
	   					const Attrib::SliceSet* =0,
	   					const Attrib::SelSpec* =0 );

    Coord3			clickedpos;
    int				clickedobjid;
    CubeSampling		clickedcs;
    const Attrib::SliceSet*	sliceset;
    const Attrib::SelSpec*	as;

    visBase::EventCatcher*	eventcatcher;
    visBase::Transformation*	transformation;
};

};

#endif
