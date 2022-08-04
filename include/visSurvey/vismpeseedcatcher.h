#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "vissurveymod.h"
#include "visobject.h"

#include "attribdataholder.h"
#include "attribsel.h"
#include "datapack.h"
#include "emposid.h"
#include "geomelement.h"
#include "integerid.h"
#include "trckey.h"
#include "trckeyzsampling.h"


namespace Geometry { class ElementEditor; }
namespace MPE { class ObjectEditor; }
namespace Attrib { class SelSpec; }
namespace visBase { class Dragger; }

class RegularSeisDataPack;

namespace visSurvey
{

class EMObjectDisplay;
class MPEEditor;
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
    int				getObjID() const;
    EM::ObjectID		getEMObjID() const; // avail when clicked on hor
    int				getEMVisID() const { return emvisids_; }
                    // avail when clicked on hor
    const TrcKeyZSampling&	getObjCS() const;
    DataPack::ID		getObjDataPackID() const;
    const RegularSeisDataPack*	getObjData() const;
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
    void			setEMObjID(EM::ObjectID);
    void			setEMVisID(int);
    void			setObjID(int);
    void			setObjCS(const TrcKeyZSampling&);
    void			setObjDataPackID(DataPack::ID);
    void			setObjData(const RegularSeisDataPack*);
    void			setObjDataSelSpec(const Attrib::SelSpec&);

    void			setGeomID(Pos::GeomID);
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
    int					clickedobjid_;
    TrcKeyZSampling			clickedcs_;
    const RegularSeisDataPack*		attrdata_;
    Attrib::SelSpec			attrsel_;
    const TrcKeyPath*			rdltkpath_;
    RandomLineID			rdlid_;
    int					emvisids_ = -1;

    ConstRefMan<Attrib::Data2DHolder>	linedata_;
    Pos::GeomID				geomid_;
    BufferString			linename_;
    DataPack::ID			datapackid_;

    void			setObjTKPath(const TrcKeyPath*);
    void			setObjRandomLineID(RandomLineID);

public:
    const TrcKeyPath*		getObjTKPath() const;
    RandomLineID		getObjRandomLineID() const;
};


mExpClass(visSurvey) MPEClickCatcher : public visBase::VisualObjectImpl
{
public:
    static MPEClickCatcher*	create()
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
    const visBase::EventInfo*   visInfo() const { return cureventinfo_; }

    void			setTrackerType(const char*);
    static bool			isClickable(const char* trackertype,int visid);

    void			setEditor(MPEEditor*);
    const MPEEditor*		getEditor() const	{ return editor_; }
    bool			activateSower(const OD::Color&,
					      const TrcKeySampling* =0);
    bool			sequentSowing() const;
    bool			moreToSow() const;
    void			stopSowing();

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

    void			allowPickBasedReselection();
    void			sowingEnd(CallBacker*);
    void			sowingCB(CallBacker*);

    visBase::EventCatcher*	eventcatcher_;
    const mVisTrans*		transformation_;
    MPEEditor*			editor_;
    const visBase::EventInfo*	cureventinfo_;

    MPEClickInfo		info_;
    const char*			trackertype_;
};

} // namespace visSurvey

