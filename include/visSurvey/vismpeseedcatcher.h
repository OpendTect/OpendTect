#ifndef vismpeseedcatcher_h
#define vismpeseedcatcher_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismpeseedcatcher.h,v 1.11 2006-12-01 16:33:07 cvsjaap Exp $
________________________________________________________________________


-*/

#include "visobject.h"

#include "attribdatacubes.h"
#include "cubesampling.h"
#include "emposid.h"
#include "geomelement.h"


namespace Geometry { class ElementEditor; }
namespace MPE { class ObjectEditor; }
namespace EM { class EdgeLineSet; }
namespace Attrib { class SelSpec; class DataCubes; class Data2DHolder; }
namespace visBase { class Marker; class Dragger; }


namespace visSurvey
{

/*!\brief
*/
class EMObjectDisplay;

class MPEClickCatcher : public visBase::VisualObjectImpl
{
public:
    static MPEClickCatcher*	create()
				mCreateDataObj(MPEClickCatcher);

    void			setSceneEventCatcher(visBase::EventCatcher*);
    void			setDisplayTransformation(mVisTrans*);

    mVisTrans*			getDisplayTransformation();

    Notifier<MPEClickCatcher>	click;
    EM::PosID			clickedNode() const;
    bool                        ctrlClicked() const;
    bool                        shiftClicked() const;
    const Coord3&		clickedPos() const;
    int				clickedObjectID() const;
    const CubeSampling&		clickedObjectCS() const;
    const Attrib::DataCubes*	clickedObjectData() const;
    const Attrib::SelSpec*	clickedObjectDataSelSpec() const;

    const MultiID&		clickedObjectLineSet() const;
    const char*			clickedObjectLineName() const;
    const Attrib::Data2DHolder*	clickedObjectLineData() const;

    static bool			isClickable(int visid);

protected:
				~MPEClickCatcher();
    void			clickCB(CallBacker*);

    void 			sendUnderlyingPlanes(
					const EMObjectDisplay*,
					const visBase::EventInfo&);

    void			sendClickEvent( const EM::PosID,
	    				bool ctrl,bool shift,const Coord3&,
	    				int objid,const CubeSampling&,
					const Attrib::DataCubes* =0,
					const Attrib::SelSpec* =0,
					const Attrib::Data2DHolder* =0,
					const char* linename = 0,
					const MultiID* lineset = 0 );

    EM::PosID				clickednode_;
    bool				ctrlclicked_;
    bool				shiftclicked_;
    Coord3				clickedpos_;
    int					clickedobjid_;
    CubeSampling			clickedcs_;
    RefMan<const Attrib::DataCubes>	attrdata_;
    const Attrib::SelSpec*		as_;

    RefMan<const Attrib::Data2DHolder>	linedata_;
    MultiID				lineset_;
    BufferString			linename_;

    visBase::EventCatcher*		eventcatcher_;
    visBase::Transformation*		transformation_;
};

};

#endif
