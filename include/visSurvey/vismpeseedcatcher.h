#ifndef vismpeseedcatcher_h
#define vismpeseedcatcher_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "visobject.h"

#include "attribsel.h"
#include "attribdataholder.h"
#include "trckeyzsampling.h"
#include "datapack.h"
#include "emposid.h"
#include "geomelement.h"

namespace Geometry { class ElementEditor; }
namespace MPE { class ObjectEditor; }
namespace EM { class EdgeLineSet; }
namespace Attrib { class SelSpec; }
namespace visBase { class Dragger; }

class RegularSeisDataPack;

namespace visSurvey
{

class EMObjectDisplay;
class MPEEditor;


mExpClass(visSurvey) MPEClickInfo
{
    friend class MPEClickCatcher;
public:
    				MPEClickInfo();

    bool			isLegalClick() const;

    bool			isCtrlClicked() const;
    bool			isShiftClicked() const;
    bool			isAltClicked() const;
    bool			isDoubleClicked() const;
    const EM::PosID&		getNode() const;
    const EM::PosID&		getPickedNode() const;
    void			setPickedNode(const EM::PosID&);
    const Coord3&		getPos() const;
    int				getObjID() const;
    const TrcKeyZSampling&		getObjCS() const;
    DataPack::ID		getObjDataPackID() const;
    const RegularSeisDataPack*	getObjData() const;
    const Attrib::SelSpec*	getObjDataSelSpec() const;

    Pos::GeomID 		getGeomID() const;
    const MultiID&		getObjLineSet() const;
    const char*			getObjLineName() const;
    const Attrib::Data2DHolder*	getObjLineData() const;

protected:
    void			clear();

    void			setLegalClick(bool);

    void			setCtrlClicked(bool); 
    void			setShiftClicked(bool);
    void			setAltClicked(bool);
    void			setDoubleClicked(bool);
    void			setNode(const EM::PosID&);
    void			setPos(const Coord3&);
    void			setObjID(int);
    void			setObjCS(const TrcKeyZSampling&);
    void			setObjDataPackID(DataPack::ID);
    void			setObjData(const RegularSeisDataPack*);
    void			setObjDataSelSpec(const Attrib::SelSpec&);

    void			setObjLineSet(const MultiID&);
    void			setGeomID(Pos::GeomID);
    void			setObjLineName(const char*);
    void			setObjLineData(const Attrib::Data2DHolder*);

    bool				legalclick_;
    bool				ctrlclicked_;
    bool				shiftclicked_;
    bool				altclicked_;
    bool				doubleclicked_;
    EM::PosID				pickednode_;
    EM::PosID				clickednode_;
    Coord3				clickedpos_;
    int					clickedobjid_;
    TrcKeyZSampling			clickedcs_;
    const RegularSeisDataPack*		attrdata_;
    Attrib::SelSpec			attrsel_;

    ConstRefMan<Attrib::Data2DHolder>	linedata_;
    Pos::GeomID 			geomid_;
    MultiID				lineset_;
    BufferString			linename_;
    DataPack::ID			datapackid_;
};


mExpClass(visSurvey) MPEClickCatcher : public visBase::VisualObjectImpl
{
public:
    static MPEClickCatcher*	create()
				mCreateDataObj(MPEClickCatcher);

    void			setSceneEventCatcher(visBase::EventCatcher*);
    void			setDisplayTransformation(const mVisTrans*);

    const mVisTrans*		getDisplayTransformation() const;

    Notifier<MPEClickCatcher>	click;

    const MPEClickInfo&		info() const;
    MPEClickInfo&		info();

    void			setTrackerType(const char*);
    static bool			isClickable(const char* trackertype,int visid);

    void			setEditor(MPEEditor*);
    bool			activateSower(const Color&,
					      const TrcKeySampling* =0);
    bool			sequentSowing() const;
    bool			moreToSow() const;
    void			stopSowing();

protected:
				~MPEClickCatcher();
    void			clickCB(CallBacker*);

    void 			sendUnderlying2DSeis(
					const EMObjectDisplay*,
					const visBase::EventInfo&);
    void 			sendUnderlyingPlanes(
					const EMObjectDisplay*,
					const visBase::EventInfo&);

    void			allowPickBasedReselection();

    visBase::EventCatcher*	eventcatcher_;
    const mVisTrans*		transformation_;
    MPEEditor*			editor_;
    const visBase::EventInfo*	cureventinfo_;

    MPEClickInfo		info_;
    const char*			trackertype_;
};

} // namespace visSurvey

#endif

