#pragma once

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		Apr 2010
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "uiodviewer2d.h"
#include "callback.h"
#include "datapack.h"
#include "flatview.h"
#include "geom2dintersections.h"
#include "odpresentationmgr.h"
#include "uigeom.h"
#include "uigraphicsviewbase.h"
#include "uiodapplmgr.h"
#include "uiodviewer2dposgrp.h"

class Probe;
class uiFlatViewer;
class uiTreeFactorySet;
class MouseEventHandler;
class TrcKeyZSampling;
class Vw2DDataObject;
namespace Attrib	{ class SelSpec; }
using Presentation::ViewerTypeID;
using Presentation::ViewerObjID;
using Presentation::ViewerID;

#define mViewer2DViewerTypeID ViewerTypeID::get(1)


mExpClass(uiODMain) uiODViewer2DMgr : public Presentation::VwrTypeMgr
{ mODTextTranslationClass(uiODViewer2DMgr);
public:

    static ViewerTypeID	theViewerTypeID()
				{ return mViewer2DViewerTypeID; }
    virtual ViewerTypeID	viewerTypeID()	const
				{ return mViewer2DViewerTypeID; }

    static void			initClass();
    struct SelectedAuxAnnot
    {
				SelectedAuxAnnot(int auxposidx=-1,
					bool isx1=true,bool selected=false)
				    : auxposidx_(auxposidx)
				    , isx1_(isx1)
				    , oldauxpos_(mUdf(float))
				    , isselected_(selected)	{}
	int			auxposidx_;
	bool			isx1_;
	bool			isselected_;
	float			oldauxpos_;
	bool			isValid() const		{ return auxposidx_>=0;}
    };

    uiODViewer2D*		getParent2DViewer(int vw2dobjid);
    uiODViewer2D*		find2DViewer(ViewerObjID id);
    uiODViewer2D*		find2DViewer(const MouseEventHandler&);
    uiODViewer2D*		find2DViewer(const Pos::GeomID&);
    uiODViewer2D*		find2DViewer(const TrcKeyZSampling&);
    int				nr2DViewers() const;

    ViewerObjID			displayIn2DViewer(
					Probe&,ProbeLayer::ID curid
						=ProbeLayer::ID::getInvalid(),
					uiODViewer2D::DispSetup su
						=uiODViewer2D::DispSetup() );
    void			remove2DViewer(ViewerObjID);

    uiTreeFactorySet*		treeItemFactorySet2D()	{ return tifs2d_; }
    uiTreeFactorySet*		treeItemFactorySet3D()	{ return tifs3d_; }
    bool			isItemPresent(const uiTreeItem*) const;

    //3D Horizons
    void			getHor3DVwr2DIDs(const DBKey& emid,
						  TypeSet<int>& vw2dids) const;
    void			removeHorizon3D(const DBKey& emid);
    void			addHorizon3Ds(const DBKeySet&);
    void			addNewTrackingHorizon3D(const DBKey& mid);
    void			addNewTrackingHorizon3D(const DBKey& mid,
							int sceneid);
    void			getLoadedHorizon3Ds(DBKeySet&) const;
    // 2D Horizons
    void			getHor2DVwr2DIDs(const DBKey& emid,
						  TypeSet<int>& vw2dids) const;
    void			removeHorizon2D(const DBKey& emid);
    void			getLoadedHorizon2Ds(DBKeySet&) const;
    void			addHorizon2Ds(const DBKeySet&);
    void			addNewTrackingHorizon2D(const DBKey& mid);
    void			addNewTrackingHorizon2D(const DBKey& mid,
							int sceneid);

    //Faults
    void			removeFault(const DBKey& emid);
    void			addFaults(const DBKeySet&);
    void			addNewTempFault(const DBKey& mid);
    void			addNewTempFault(const DBKey& mid,int sceneid);
    void			getLoadedFaults(DBKeySet&) const;

    //FaultStickSet
    void			getFaultSSVwr2DIDs(const DBKey& emid,
						    TypeSet<int>& vw2ids) const;
    void			removeFaultSS(const DBKey& emid);
    void			addFaultSSs(const DBKeySet&);
    void			addNewTempFaultSS(const DBKey& mid);
    void			addNewTempFaultSS(const DBKey& mid,int sceneid);
    void			getLoadedFaultSSs(DBKeySet&) const;

    void			getVwr2DObjIDs(TypeSet<int>& vw2ids) const;
    //FaultStickSet2D
    void			getFaultSS2DVwr2DIDs( const DBKey& emid,
						    TypeSet<int>& vw2ids) const;
    void			removeFaultSS2D(const DBKey& emid);
    void			addFaultSS2Ds(const DBKeySet&);
    void			addNewTempFaultSS2D(const DBKey& mid);
    void			addNewTempFaultSS2D(const DBKey& mid,
						    int sceneid);
    void			getLoadedFaultSS2Ds(DBKeySet&) const;

    //PickSets
    void			getPickSetVwr2DIDs(const DBKey& mid,
						   TypeSet<int>& vw2ids) const;
    void			removePickSet(const DBKey&);
    void			getLoadedPickSets(DBKeySet&) const;
    void			addPickSets(const DBKeySet&);

    virtual void		handleRequest(ViewerID originvwrid,
					      RequestType,
					      const IOPar& prinfopar);


    static int			cNameColumn()		{ return 0; }
    static int			cColorColumn()		{ return 1; }

    static const char*		sKeyVisID()		{ return "VisID"; }
    static const char*		sKeyAttrID()		{ return "Attrib ID"; }
    static const char*		sKeyWVA()		{ return "WVA"; }

    CNotifier<uiODViewer2DMgr,int>  vw2dObjAdded;
    CNotifier<uiODViewer2DMgr,int>  vw2dObjToBeRemoved;

protected:

				uiODViewer2DMgr(uiODMain*);
				~uiODViewer2DMgr();

    uiODViewer2D*		getViewer2D(int idx);
    const uiODViewer2D*		getViewer2D(int idx) const;
    Line2DInterSectionSet*	l2dintersections_;
    SelectedAuxAnnot		selauxannot_;
    GeomIDSet			geom2dids_;

    uiTreeFactorySet*		tifs2d_;
    uiTreeFactorySet*		tifs3d_;

    uiODMain&			appl_;

    inline uiODApplMgr&         applMgr()     { return appl_.applMgr(); }
    inline uiVisPartServer&     visServ()     { return *applMgr().visServer(); }

    void			surveyChangedCB(CallBacker*);
    void			viewObjAdded(CallBacker*);
    void			viewObjToBeRemoved(CallBacker*);
    void			viewWinClosedCB(CallBacker*);
    void			vw2DPosChangedCB(CallBacker*);
    void			homeZoomChangedCB(CallBacker*);
    void			mouseClickCB(CallBacker*);
    void			mouseClickedCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);

    Probe*			createNewProbe(const TrcKeyZSampling&) const;
    void			fillProbeFromExisting(
					Probe&,const uiODViewer2D&) const;
    void			attachNotifiersAndSetAuxData(uiODViewer2D*);
    Line2DInterSection::Point	intersectingLineID(const uiODViewer2D*,
						   float pos) const;
    int				intersection2DIdx(Pos::GeomID) const;
    void			getVWR2DDataGeomIDs(const uiODViewer2D*,
						    GeomIDSet&) const;
    void			reCalc2DIntersetionIfNeeded(Pos::GeomID);
    void			setAllIntersectionPositions();
    void			setVWR2DIntersectionPositions(uiODViewer2D*);
    void			handleLeftClick(uiODViewer2D*);
    void			setAuxAnnotLineStyles(uiFlatViewer&,bool forx1);
    void			setupHorizon3Ds(uiODViewer2D*);
    void			setupHorizon2Ds(uiODViewer2D*);
    void			setupFaults(uiODViewer2D*);
    void			setupFaultSSs(uiODViewer2D*);
    void			setupPickSets(uiODViewer2D*);
    void			setupCurInterpItem(uiODViewer2D*);
    void			reSetPrevDragMode(uiODViewer2D*);

    void			fillPar(IOPar&) const;
    void			usePar(const IOPar&);

    friend class                uiODMain;

};
