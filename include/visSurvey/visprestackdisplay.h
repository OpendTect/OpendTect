#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		May 2007
 RCS:		$Id$
________________________________________________________________________

-*/

#include "mousecursor.h"
#include "vissurveymod.h"
#include "vissurvobj.h"
#include "visobject.h"
#include "iopar.h"

class IOObj;
class SeisPSReader;

namespace PreStack { class ProcessManager; }
namespace visBase
{
    class DepthTabPlaneDragger;
    class FaceSet;
    class FlatViewer;
}

namespace visSurvey
{

class PlaneDataDisplay;
class Seis2DDisplay;


mExpClass(visSurvey) PreStackDisplay : public visBase::VisualObjectImpl,
		 public visSurvey::SurveyObject
{
public:

				PreStackDisplay();
				mDefaultFactoryInstantiation(
				    visSurvey::SurveyObject,PreStackDisplay,
				    "PreStackDisplay", 
				    toUiString(sFactoryKeyword()) );

    void			allowShading(bool yn);
    void			setMultiID(const MultiID& mid);
    BufferString		getObjectName() const;
    bool			isInlCrl() const	{ return true; }
    bool			isOrientationInline() const;
    const Coord			getBaseDirection() const;
    StepInterval<int>		getTraceRange(const BinID&,
					      bool oncurrentline=true) const;

				//for 3D only at present
    DataPack::ID		preProcess();

    bool			is3DSeis() const;
    virtual DataPack::ID	getDataPackID(int i=0) const;

    visBase::FlatViewer*	flatViewer()	{ return flatviewer_; }
    const visBase::FlatViewer*	flatViewer() const { return flatviewer_; }
    PreStack::ProcessManager&	procMgr()	{ return preprocmgr_; }

				//3D case
    bool			setPosition(const BinID&);
    const BinID&		getPosition() const;
    void			setSectionDisplay(PlaneDataDisplay*);
    const PlaneDataDisplay*	getSectionDisplay() const;
    PlaneDataDisplay*		getSectionDisplay();

    Notifier<PreStackDisplay>	draggermoving;
    NotifierAccess*		getMovementNotifier() { return &draggermoving;}
    const BinID			draggerPosition() const	{ return draggerpos_; }

    bool			hasPosModeManipulator() const	{ return true; }

				//2D case
    const Seis2DDisplay*	getSeis2DDisplay() const;
    bool			setSeis2DData(const IOObj* ioobj);
    bool			setSeis2DDisplay(Seis2DDisplay*,int trcnr);
    void			setTraceNr(int trcnr);
    int				traceNr() const	  { return trcnr_; }
    const char*			lineName() const;

    bool                        displayAutoWidth() const { return autowidth_; }
    void                        displaysAutoWidth(bool yn);
    bool                        displayOnPositiveSide() const {return posside_;}
    void                        displaysOnPositiveSide(bool yn);
    float                       getFactor() { return factor_; }
    void			setFactor(float scale);
    float                       getWidth() { return width_; }
    void			setWidth(float width);
    BinID			getBinID() const { return bid_; }
    virtual MultiID		getMultiID() const { return mid_; }

    const MouseCursor*		getMouseCursor() const { return &mousecursor_; }
    virtual void		getMousePosInfo( const visBase::EventInfo& ei,
						 IOPar& iop ) const
				{ SurveyObject::getMousePosInfo(ei,iop); }
    virtual void		getMousePosInfo(const visBase::EventInfo&,
						Coord3&,
						BufferString& val,
						BufferString& info) const;
    void			otherObjectsMoved(
					const ObjectSet<const SurveyObject>&,
					int whichobj );


    void			fillPar(IOPar&) const;
    bool			usePar(const IOPar&);
    bool			updateDisplay() { return updateData(); }

    static const char*		sKeyParent()	{ return "Parent"; }
    static const char*		sKeyFactor()	{ return "Factor"; }
    static const char*		sKeyWidth()	{ return "Width"; }
    static const char*		sKeyAutoWidth() { return "AutoWidth"; }
    static const char*		sKeySide()	{ return "ShowSide"; }

protected:
				~PreStackDisplay();
    void			setDisplayTransformation(const mVisTrans*);
    void			setSceneEventCatcher(visBase::EventCatcher*);
    void			updateMouseCursorCB(CallBacker*);
    void			dataChangedCB(CallBacker*);
    void			sectionMovedCB(CallBacker*);
    void			seis2DMovedCB(CallBacker*);
    bool			updateData();
    int				getNearTraceNr(int) const;
    BinID			getNearBinID(const BinID& pos) const;

    void			draggerMotion(CallBacker*);
    void			finishedCB(CallBacker*);

    BinID			bid_;
    BinID			draggerpos_;
    visBase::EventCatcher*	eventcatcher_;
    MouseCursor			mousecursor_;
    visBase::DepthTabPlaneDragger*	planedragger_;
    visBase::FlatViewer*	flatviewer_;
    PreStack::ProcessManager&	preprocmgr_;

    MultiID			mid_;
    PlaneDataDisplay*		section_;
    Seis2DDisplay*		seis2d_;
    int				trcnr_;
    Coord			basedirection_;
    Coord			seis2dpos_;
    Coord			seis2dstoppos_;

    bool			posside_;
    bool			autowidth_;
    float			factor_;
    float			width_;
    Interval<float>		offsetrange_;
    Interval<float>		zrg_;

    SeisPSReader*		reader_;
    IOObj*			ioobj_;
    Notifier<PreStackDisplay>	movefinished_;
};

} // namespace visSurvey

