#ifndef vismpeseedcatcher_h
#define vismpeseedcatcher_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismpeseedcatcher.h,v 1.6 2006-03-30 16:11:26 cvsjaap Exp $
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
namespace Attrib { class SelSpec; class DataCubes; }
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

    mVisTrans*			getDisplayTransformation() 
							{return transformation;}

    Notifier<MPEClickCatcher>	click;
    EM::PosID			clickedNode() const     {return clickednode;}
    bool                        ctrlClicked() const     {return ctrlclicked;}
    bool                        shiftClicked() const    {return shiftclicked;}
    const Coord3&		clickedPos() const;
    int				clickedObjectID() const {return clickedobjid;}
    const CubeSampling&		clickedObjectCS() const {return clickedcs;}
    const Attrib::DataCubes*	clickedObjectData() 
						  const {return attrdata;}
    const Attrib::SelSpec*	clickedObjectDataSelSpec() 
						  const {return as;}

protected:
				~MPEClickCatcher();
    void			clickCB(CallBacker*);

    void 			sendPlanesContainingNode(
					int visid, const EMObjectDisplay*,
					const visBase::EventInfo&);

    void			sendClickEvent( const EM::PosID,
	    				bool ctrl,bool shift,const Coord3&,
	    				int objid,const CubeSampling&,
					const Attrib::DataCubes* =0,
					const Attrib::SelSpec* =0);

    EM::PosID			clickednode;
    bool			ctrlclicked;
    bool			shiftclicked;
    Coord3			clickedpos;
    int				clickedobjid;
    CubeSampling		clickedcs;
    RefMan<const 
	   Attrib::DataCubes>   attrdata;
    const Attrib::SelSpec*	as;

    visBase::EventCatcher*	eventcatcher;
    visBase::Transformation*	transformation;
};

};

#endif
