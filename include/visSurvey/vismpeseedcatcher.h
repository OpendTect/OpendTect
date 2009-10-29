#ifndef vismpeseedcatcher_h
#define vismpeseedcatcher_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: vismpeseedcatcher.h,v 1.19 2009-10-29 08:49:38 cvsumesh Exp $
________________________________________________________________________


-*/

#include "visobject.h"

#include "attribdatacubes.h"
#include "attribsel.h"
#include "attribdataholder.h"
#include "cubesampling.h"
#include "datapack.h"
#include "emposid.h"
#include "geomelement.h"


namespace Geometry { class ElementEditor; }
namespace MPE { class ObjectEditor; }
namespace EM { class EdgeLineSet; }
namespace Attrib { class SelSpec; }
namespace visBase { class Marker; class Dragger; }


namespace visSurvey
{

/*!\brief
*/
class EMObjectDisplay;


mClass MPEClickInfo
{
    friend class MPEClickCatcher;
public:
    				MPEClickInfo();

    bool			isLegalClick() const;

    bool                        isCtrlClicked() const; 
    bool                        isShiftClicked() const;
    bool                        isAltClicked() const;
    const EM::PosID&		getNode() const;
    const Coord3&		getPos() const;
    int				getObjID() const;
    const CubeSampling&		getObjCS() const;
    DataPack::ID		getObjDataPackID() const;
    const Attrib::DataCubes*	getObjData() const;
    const Attrib::SelSpec*	getObjDataSelSpec() const;

    const MultiID&		getObjLineSet() const;
    const char*			getObjLineName() const;
    const Attrib::Data2DHolder*	getObjLineData() const;

protected:
    void			clear();

    void			setLegalClick(bool);

    void			setCtrlClicked(bool); 
    void			setShiftClicked(bool);
    void			setAltClicked(bool);
    void			setNode(const EM::PosID&);
    void			setPos(const Coord3&);
    void			setObjID(int);
    void			setObjCS(const CubeSampling&);
    void			setObjDataPackID(DataPack::ID);
    void			setObjData(const Attrib::DataCubes*);
    void			setObjDataSelSpec(const Attrib::SelSpec&);

    void			setObjLineSet(const MultiID&);
    void			setObjLineName(const char*);
    void			setObjLineData(const Attrib::Data2DHolder*);

    bool				legalclick_;
    bool				ctrlclicked_;
    bool				shiftclicked_;
    bool				altclicked_;
    EM::PosID				clickednode_;
    Coord3				clickedpos_;
    int					clickedobjid_;
    CubeSampling			clickedcs_;
    RefMan<const Attrib::DataCubes>	attrdata_;
    Attrib::SelSpec			attrsel_;

    RefMan<const Attrib::Data2DHolder>	linedata_;
    MultiID				lineset_;
    BufferString			linename_;
    DataPack::ID			datapackid_;
};


mClass MPEClickCatcher : public visBase::VisualObjectImpl
{
public:
    static MPEClickCatcher*	create()
				mCreateDataObj(MPEClickCatcher);

    void			setSceneEventCatcher(visBase::EventCatcher*);
    void			setDisplayTransformation(mVisTrans*);

    mVisTrans*			getDisplayTransformation();

    Notifier<MPEClickCatcher>	click;

    const MPEClickInfo&		info() const;
    MPEClickInfo&		info();

    void			setTrackerType(const char*);
    static bool			isClickable(const char* trackertype,int visid);

protected:
				~MPEClickCatcher();
    void			clickCB(CallBacker*);

    void 			sendUnderlying2DSeis(
					const EMObjectDisplay*,
					const visBase::EventInfo&);
    void 			sendUnderlyingPlanes(
					const EMObjectDisplay*,
					const visBase::EventInfo&);

    visBase::EventCatcher*	eventcatcher_;
    visBase::Transformation*	transformation_;

    MPEClickInfo		info_;
    const char*			trackertype_;
};



};

#endif
