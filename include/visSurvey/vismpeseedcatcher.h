#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "vissurveymod.h"

#include "visevent.h"
#include "vismpeeditor.h"
#include "visobject.h"
#include "vistransform.h"

#include "attribdataholder.h"
#include "attribsel.h"
#include "datapackbase.h"
#include "emposid.h"
#include "geomelement.h"
#include "trckey.h"


namespace Geometry { class ElementEditor; }
namespace Attrib { class SelSpec; }

namespace visSurvey
{

class EMObjectDisplay;
class MultiTextureSurveyObject;
class Seis2DDisplay;


mExpClass(visSurvey) MPEClickInfo
{
    friend class MPEClickCatcher;
public:
				MPEClickInfo();
				~MPEClickInfo();

    bool			isLegalClick() const;

    bool			isCtrlClicked() const;
    bool			isShiftClicked() const;
    bool			isAltClicked() const;
    bool			isDoubleClicked() const;

    const TrcKey&		getNode() const;
    const TrcKey&		getPickedNode() const;
    void			setPickedNode(const TrcKey&);

    const Coord3&		getPos() const;
    VisID			getObjID() const;
    EM::ObjectID		getEMObjID() const; // avail when clicked on hor
    VisID			getEMVisID() const { return emvisids_; }
				// avail when clicked on hor
    const TrcKeyZSampling&	getObjCS() const;
    ConstRefMan<VolumeDataPack> getObjData() const;
    const Attrib::SelSpec*	getObjDataSelSpec() const;

    Pos::GeomID			getGeomID() const;
    const char*			getObjLineName() const;
    const Attrib::Data2DHolder*	getObjLineData() const;

protected:
    void			clear();

    void			setLegalClick(bool);

    void			setCtrlClicked(bool);
    void			setShiftClicked(bool);
    void			setAltClicked(bool);
    void			setDoubleClicked(bool);
    void			setNode(const TrcKey&);
    void			setPos(const Coord3&);
    void			setEMObjID(const EM::ObjectID&);
    void			setEMVisID(const VisID&);
    void			setObjID(const VisID&);
    void			setObjCS(const TrcKeyZSampling&);
    void			setObjData(const VolumeDataPack*);
    void			setObjDataSelSpec(const Attrib::SelSpec&);

    void			setGeomID(const Pos::GeomID&);
    void			setObjLineName(const char*);
    void			setObjLineData(const Attrib::Data2DHolder*);

    bool				legalclick_;
    bool				ctrlclicked_;
    bool				shiftclicked_;
    bool				altclicked_;
    bool				doubleclicked_;

    TrcKey				pickednode_;
    TrcKey				clickednode_;
    Coord3				clickedpos_;

    EM::ObjectID			clickedemobjid_;
    VisID				clickedobjid_;
    TrcKeyZSampling			clickedcs_;
    WeakPtr<VolumeDataPack>		attrdata_;
    Attrib::SelSpec			attrsel_;
    const TrcKeySet*			rdltkpath_;
    RandomLineID			rdlid_;
    VisID				emvisids_;

    ConstRefMan<Attrib::Data2DHolder>	linedata_;
    Pos::GeomID				geomid_;
    BufferString			linename_;

    void			setObjTKPath(const TrcKeySet*);
    void			setObjRandomLineID(const RandomLineID&);

public:
    const TrcKeySet*		getObjTKPath() const;
    RandomLineID		getObjRandomLineID() const;
};


mExpClass(visSurvey) MPEClickCatcher : public visBase::VisualObjectImpl
{
public:
    static RefMan<MPEClickCatcher> create();
				mCreateDataObj(MPEClickCatcher);

    void			setSceneEventCatcher(
					visBase::EventCatcher*) override;
    void			setDisplayTransformation(
					const mVisTrans*) override;

    const mVisTrans*		getDisplayTransformation() const override;

    Notifier<MPEClickCatcher>	click;
    Notifier<MPEClickCatcher>	endSowing;
    Notifier<MPEClickCatcher>	sowing;

    const MPEClickInfo&		info() const;
    MPEClickInfo&		info();
    const visBase::EventInfo*	visInfo() const { return cureventinfo_; }

    void			setTrackerType(const char*);
    static bool			isClickable(const char* trackertype,
					    const VisID&);

    void			setEditor(MPEEditor*);
    const MPEEditor*		getEditor() const	{ return editor_.ptr();}
    bool			activateSower(const OD::Color&,
					      const TrcKeySampling* =nullptr);
    bool			sequentSowing() const;
    bool			moreToSow() const;
    void			stopSowing();

    bool			forceAttribute(const Attrib::SelSpec&);

protected:
				~MPEClickCatcher();
    void			clickCB(CallBacker*);

    void			sendUnderlying2DSeis(
					const EMObjectDisplay*,
					const visBase::EventInfo&);
    void			sendUnderlyingPlanes(
					const EMObjectDisplay*,
					const visBase::EventInfo&);
    void			handleObjectOnSeis2DDisplay(Seis2DDisplay*,
							    const Coord3);
    int				handleAttribute(const MultiTextureSurveyObject&,
						const Coord3& worldpickedpos);

    void			allowPickBasedReselection();
    void			sowingEnd(CallBacker*);
    void			sowingCB(CallBacker*);

    RefMan<visBase::EventCatcher> eventcatcher_;
    ConstRefMan<mVisTrans>	transformation_;
    RefMan<MPEEditor>		editor_;
    const visBase::EventInfo*	cureventinfo_		= nullptr;

    MPEClickInfo		info_;
    const char*			trackertype_		= nullptr;
};

} // namespace visSurvey
