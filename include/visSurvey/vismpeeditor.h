#ifndef vismpeeditor_h
#define vismpeeditor_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismpeeditor.h,v 1.6 2005-07-15 14:00:42 cvskris Exp $
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
    MPE::ObjectEditor*	getMPEEditor() { return emeditor; }
    void		setSceneEventCatcher( visBase::EventCatcher* );

    void		moveTemporaryNode( const EM::PosID& );
    const EM::PosID&	getTemporaryNode() const;

    void		setDisplayTransformation( mVisTrans* );
    mVisTrans*		getDisplayTransformation() {return transformation;}

    const ObjectSet<visBase::Marker>&	markerNodes() const { return markers; }
    EM::PosID				markerId(const visBase::Marker*) const;
    bool				isDraggerShown() const;
    void				setMarkerSize(float);

    Notifier<MPEEditor>		noderightclick;
    				/*!<\ the clicked position can be retrieved
				      with getNodePosID(getRightClickNode) */
    int				getRightClickNode() const;
    EM::PosID			getNodePosID(int idx) const;

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

    EM::PosID			temporarynode;
    int				rightclicknode;

    int				issettingpos;
    Geometry::ElementEditor*	geeditor;
    MPE::ObjectEditor*		emeditor;

    visBase::Dragger*		translationdragger;
    EM::PosID			activenode;

    ObjectSet<visBase::Marker>	markers;
    TypeSet<EM::PosID>		posids;
    float			markersize;

    visBase::EventCatcher*	eventcatcher;
    visBase::Transformation*	transformation;
};

};

#endif
