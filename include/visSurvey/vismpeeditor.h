#ifndef vismpeeditor_h
#define vismpeeditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismpeeditor.h,v 1.3 2005-01-17 08:02:42 kristofer Exp $
________________________________________________________________________


-*/

#include "visobject.h"

#include "emposid.h"
#include "geomelement.h"


namespace Geometry { class ElementEditor; };
namespace MPE { class ObjectEditor; };

namespace visBase
{

class Marker;
class Dragger;
};


namespace visSurvey
{

/*!\brief
*/


class MPEEditor : public visBase::VisualObjectImpl
{
public:
    static MPEEditor*	create()
			mCreateDataObj( MPEEditor );

    void		setEditor( Geometry::ElementEditor* );
    void		setEditor( MPE::ObjectEditor* );
    void		setSceneEventCatcher( visBase::EventCatcher* );

    void		setDisplayTransformation( mVisTrans* );
    mVisTrans*		getDisplayTransformation() {return transformation;}

    const ObjectSet<visBase::Marker>&	markerNodes() const { return markers; }
    EM::PosID				markerId(const visBase::Marker*) const;
    bool				isDraggerShown() const;

    CNotifier<MPEEditor, int>	noderightclick;
    				/*!<\ the clicked position can be retrieved
				      with getNodePosID(int) */
    EM::PosID			getNodePosID(int visid) const;

protected:
    				~MPEEditor();
    void			changeNumNodes( CallBacker* );
    void			nodeMovement( CallBacker* );
    void			clickCB( CallBacker* );
    void			dragStart( CallBacker* );
    void			dragMotion( CallBacker* );
    void			dragStop( CallBacker* );
    void			updateDraggers();
    void			updateNodePos(int, const Coord3& );

    int				issettingpos;
    Geometry::ElementEditor*	geeditor;
    MPE::ObjectEditor*		emeditor;

    visBase::Dragger*		translationdragger;
    EM::PosID			activenode;

    ObjectSet<visBase::Marker>	markers;
    TypeSet<EM::PosID>		posids;

    visBase::EventCatcher*	eventcatcher;
    visBase::Transformation*	transformation;
};

};

#endif
