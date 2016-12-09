#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "trckeyzsampling.h"
#include "datapack.h"
#include "emposid.h"
#include "probe.h"
#include "uigeom.h"
#include "uistring.h"

class uiFlatViewAuxDataEditor;
class uiFlatViewStdControl;
class uiFlatViewWin;
class uiMainWin;
class uiODMain;
class uiODVw2DTreeTop;
class uiParent;
class uiSlicePos2DView;
class uiToolBar;
class uiTreeItem;
class uiTreeFactorySet;
class MouseCursorExchange;
class TaskRunner;
class Vw2DDataManager;

namespace Attrib	{ class SelSpec; }
namespace FlatView	{ class AuxData; }
namespace ZDomain	{ class Def; }

static OD::ViewerTypeID sViewer2DMgrTypeID( OD::ViewerTypeID::get(1) );
/*!
\brief A 2D Viewer.
*/

mExpClass(uiODMain) uiODViewer2D : public OD::PresentationManagedViewer
{ mODTextTranslationClass(uiODViewer2D);
public:

    mStruct(uiODMain) DispSetup
    {
				DispSetup()
				    : initialcentre_(uiWorldPoint::udf())
				    , initialx1pospercm_(mUdf(float))
				    , initialx2pospercm_(mUdf(float))	{}
	uiWorldPoint		initialcentre_;
	float			initialx1pospercm_;
	float			initialx2pospercm_;
    };
				uiODViewer2D(uiODMain&,Probe&,
					     DispSetup su=DispSetup());
				~uiODViewer2D();

    mDeclInstanceCreatedNotifierAccess(uiODViewer2D);

    OD::ViewerTypeID		viewerTypeID() const
				{ return sViewer2DMgrTypeID; }
    virtual void		setUpView(ProbeLayer::ID id=
					  ProbeLayer::ID::getInvalid());
				//Invalid means set all layers
    void			setSelSpec(const Attrib::SelSpec*,bool wva);

    uiParent*			viewerParent();
    uiFlatViewWin*		viewwin()		{ return viewwin_; }
    const uiFlatViewWin*	viewwin() const		{ return viewwin_; }
    Vw2DDataManager*		dataMgr()		{ return datamgr_; }
    const Vw2DDataManager*	dataMgr() const		{ return datamgr_; }

    uiODVw2DTreeTop*		treeTop()		{ return treetp_; }

    const uiTreeFactorySet*	uiTreeItemFactorySet() const { return tifs_; }
    bool			isItemPresent(const uiTreeItem*) const;

    const ObjectSet<uiFlatViewAuxDataEditor>&	dataEditor()
				{ return auxdataeditors_; }

    Attrib::SelSpec&		selSpec( bool wva )
				{ return wva ? wvaselspec_ : vdselspec_; }
    const Attrib::SelSpec&	selSpec( bool wva ) const
				{ return wva ? wvaselspec_ : vdselspec_; }
    DataPack::ID		createDataPack(bool wva)
				{ return createDataPack(selSpec(wva)); }
    DataPack::ID		createDataPack(const Attrib::SelSpec&);
				/*!< Creates RegularDataPack by getting
				TrcKeyZSampling from slicepos_. Uses the
				existing TrcKeyZSampling, if there is no
				slicepos_. Also transforms data if the 2D Viewer
				hasZAxisTransform(). */

    bool			useStoredDispPars(bool wva);
    bool			isVertical() const;

    TrcKeyZSampling		getTrcKeyZSampling() const
				{ return probe_.position(); }
    Pos::GeomID			geomID() const;

    void			setUpAux();

    const uiFlatViewStdControl* viewControl() const
				{ return viewstdcontrol_; }
    uiFlatViewStdControl*	viewControl()
				{ return viewstdcontrol_; }
    uiSlicePos2DView*		slicePos()
				{ return slicepos_; }

    virtual void		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;
    virtual void		setWinTitle();

    static const char*		sKeyVDSelSpec()  { return "VD SelSpec"; }
    static const char*		sKeyWVASelSpec() { return "WVA SelSpec"; }
    static const char*		sKeyPos()	 { return "Position"; }

    Notifier<uiODViewer2D>	viewWinAvailable;
    Notifier<uiODViewer2D>	viewWinClosed;
    Notifier<uiODViewer2D>	dataChanged;
    Notifier<uiODViewer2D>	posChanged;

    void			getVwr2DObjIDs(TypeSet<int>& vw2dobjids) const;

    //Horizon 3D
    void			getHor3DVwr2DIDs(EM::ObjectID emid,
						 TypeSet<int>& vw2dids) const;
    void			removeHorizon3D(EM::ObjectID emid);
    void			getLoadedHorizon3Ds(
					TypeSet<EM::ObjectID>&) const;
    void			addHorizon3Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTrackingHorizon3D(EM::ObjectID);
    void			setupTrackingHorizon3D(EM::ObjectID);

    //Horizon2D
    void			getHor2DVwr2DIDs(EM::ObjectID emid,
						 TypeSet<int>& vw2dids) const;
    void			removeHorizon2D(EM::ObjectID emid);
    void			getLoadedHorizon2Ds(
					TypeSet<EM::ObjectID>&) const;
    void			addHorizon2Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTrackingHorizon2D(EM::ObjectID emid);
    void			setupTrackingHorizon2D(EM::ObjectID);

    //Fault
    void			getFaultVwr2DIDs(EM::ObjectID emid,
						 TypeSet<int>& vw2dids) const;
    void			removeFault(EM::ObjectID emid);
    void			getLoadedFaults(
					TypeSet<EM::ObjectID>&) const;
    void			addFaults(const TypeSet<EM::ObjectID>&);
    void			addNewTempFault(EM::ObjectID emid);
    void			setupNewTempFault(EM::ObjectID emid);

    //FaultStickeSet
    void			getFaultSSVwr2DIDs(EM::ObjectID emid,
						   TypeSet<int>& vw2dids) const;
    void			removeFaultSS(EM::ObjectID emid);
    void			getLoadedFaultSSs(
					TypeSet<EM::ObjectID>&) const;
    void			addFaultSSs(const TypeSet<EM::ObjectID>&);
    void			addNewTempFaultSS(EM::ObjectID emid);
    void			setupNewTempFaultSS(EM::ObjectID emid);

    //FaultStickeSet2D
    void			getFaultSS2DVwr2DIDs(EM::ObjectID emid,
						     TypeSet<int>& vw2ds) const;
    void			removeFaultSS2D(EM::ObjectID emid);
    void			getLoadedFaultSS2Ds(
					TypeSet<EM::ObjectID>&) const;
    void			addFaultSS2Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTempFaultSS2D(EM::ObjectID emid);
    void			setupNewTempFaultSS2D(EM::ObjectID emid);

    //PickSets
    void			getPickSetVwr2DIDs(const DBKey& mid,
						   TypeSet<int>& vw2ids ) const;
    void			removePickSet(const DBKey&);
    void			getLoadedPickSets(DBKeySet&) const;
    void			addPickSets(const DBKeySet&);
    void			setupNewPickSet(const DBKey&);

    void			emitPRRequest(OD::PresentationRequestType);
    OD::ObjPresentationInfo*	getObjPRInfo() const;
    Probe&			getProbe()		{ return probe_; }
    const Probe&		getProbe() const	{ return probe_; }

protected:

    uiSlicePos2DView*				slicepos_;
    uiFlatViewStdControl*			viewstdcontrol_;
    ObjectSet<uiFlatViewAuxDataEditor>		auxdataeditors_;

    Attrib::SelSpec&		wvaselspec_;
    Attrib::SelSpec&		vdselspec_;

    Probe&			probe_;
    DispSetup			dispsetup_;
    Vw2DDataManager*		datamgr_;
    uiTreeFactorySet*		tifs_;
    uiODVw2DTreeTop*		treetp_;
    uiFlatViewWin*		viewwin_;
    MouseCursorExchange&	mousecursorexchange_;
    FlatView::AuxData*		marker_;
    uiString			basetxt_;
    uiODMain&			appl_;
    int				voiidx_;


    int				edittbid_;
    int				polyseltbid_;
    int				picksettingstbid_;
    bool			ispolyselect_;

    DataPack::ID		createDataPackForTransformedZSlice(
						const Attrib::SelSpec&);

    DataPack::ID		createFlatDataPack(DataPack::ID,int comp);
				/*!< Creates a FlatDataPack from VolumeDataPack
				Either a transformed or a non-transformed
				datapack can be passed. The returned datapack
				will always be in transformed domain if the
				viewer hasZAxisTransform(). */

    virtual void		createViewWin();
    virtual void		createTree(uiMainWin*);
    virtual void		createPolygonSelBut(uiToolBar*);
    void			createViewWinEditors();
    void			setDataPack(DataPack::ID,bool wva,bool isnew);
    void			removeAvailablePacks();
    void			rebuildTree();
    void			updateSlicePos();
    void			updateTransformData();

    void			winCloseCB(CallBacker*);
    void			probeChangedCB(CallBacker*);
    void			posChg(CallBacker*);
    void			itmSelectionChangedCB(CallBacker*);
    void			selectionMode(CallBacker*);
    void			trackSetupCB(CallBacker*);
    void			handleToolClick(CallBacker*);
    void			removeSelected(CallBacker*);
    void			mouseCursorCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);
};
