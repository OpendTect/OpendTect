#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "emposid.h"
#include "flatview.h"
#include "integerid.h"
#include "seisdatapack.h"
#include "trckeyzsampling.h"

#include "uigeom.h"
#include "uistring.h"

class uiFlatViewAuxDataEditor;
class uiFlatViewStdControl;
class uiFlatViewWin;
class uiMainWin;
class uiODMain;
class uiODView2DTreeTop;
class uiParent;
class uiSlicePos2DView;
class uiToolBar;
class uiTreeItem;
class uiTreeFactorySet;

class MouseCursorExchange;
class RegularFlatDataPack;
class TaskRunner;
class DataManager;
class ZAxisTransform;

namespace Attrib	{ class SelSpec; }
namespace View2D	{ class DataManager; class DataObject; }
namespace ZDomain	{ class Def; }

/*!
\brief A 2D Viewer.
*/

mExpClass(uiODMain) uiODViewer2D : public CallBacker
{ mODTextTranslationClass(uiODViewer2D);
public:
				uiODViewer2D(uiODMain&,VisID);
				~uiODViewer2D();

    mDeclInstanceCreatedNotifierAccess(uiODViewer2D);

    Viewer2DID			ID() const	{ return id_; }
    VisID			visID() const	{ return visid_; }

    void			makeUpView(FlatDataPack*,
					   FlatView::Viewer::VwrDest);
    void			setSelSpec(const Attrib::SelSpec*,
					   FlatView::Viewer::VwrDest);
    void			setMouseCursorExchange(MouseCursorExchange*);

    uiParent*			viewerParent();
    uiFlatViewWin*		viewwin()		{ return viewwin_; }
    const uiFlatViewWin*	viewwin() const		{ return viewwin_; }
    View2D::DataManager*	dataMgr()		{ return datamgr_; }
    const View2D::DataManager*	dataMgr() const		{ return datamgr_; }

    const View2D::DataObject*	getObject(Vis2DID) const;
    View2D::DataObject*		getObject(Vis2DID);
    void			getObjects(ObjectSet<View2D::DataObject>&)const;

    uiODView2DTreeTop*		treeTop()		{ return treetp_; }

    const uiTreeFactorySet*	uiTreeItemFactorySet() const { return tifs_; }
    bool			isItemPresent(const uiTreeItem*) const;

    const ObjectSet<uiFlatViewAuxDataEditor>&	dataEditor()
				{ return auxdataeditors_; }

    Attrib::SelSpec&		selSpec( bool wva )
				{ return wva ? wvaselspec_ : vdselspec_; }
    const Attrib::SelSpec&	selSpec( bool wva ) const
				{ return wva ? wvaselspec_ : vdselspec_; }
    RefMan<SeisFlatDataPack>	getDataPack(bool wva) const;
				/*!< Returns reference to DataPack of specified
				 display if it has a valid one. Returns
				 refernce ot DataPack of other display if both
				 have same Attrib::SelSpec. Else, returns
				 uiODViewer2D::createDataPack.*/
    RefMan<SeisFlatDataPack>	createDataPackRM(bool wva) const;
    RefMan<SeisFlatDataPack>	createDataPackRM(const Attrib::SelSpec&)
									const;
				/*!< Creates and returns reference to a
				 RegularFlatDataPack by getting TrcKeyZSampling
				 from slicepos_. Uses the existing
				 TrcKeyZSampling, if there is no slicepos_.
				 Also transforms data if the 2D Viewer
				 hasZAxisTransform(). */
    RefMan<SeisFlatDataPack>	createFlatDataPackRM(const SeisDataPack&,
						       int comp) const;
				/*!< Creates a SeisFlatDataPack from a
				 SeisDataPack. Either a transformed or a
				 non-transformed datapack can be passed. The
				 returned datapack will always be in transformed
				  domain if the viewer hasZAxisTransform(). */
    RefMan<SeisFlatDataPack>	createFlatDataPackRM(DataPackID,int comp) const;
				/*!< Creates a SeisFlatDataPack from a
				 SeisDataPack. Either a transformed or a
				 non-transformed datapack can be passed. The
				 returned datapack will always be in transformed
				  domain if the viewer hasZAxisTransform(). */
    RefMan<MapDataPack>		createMapDataPackRM(const RegularFlatDataPack&);

    bool			useStoredDispPars(FlatView::Viewer::VwrDest);
    bool			isVertical() const	{ return isvertical_; }

    ZAxisTransform*		getZAxisTransform() const
				{ return datatransform_; }
    bool			setZAxisTransform(ZAxisTransform*);
    bool			hasZAxisTransform() const
				{ return datatransform_; }
    virtual void		setPos(const TrcKeyZSampling&);
    void			setRandomLineID( RandomLineID id )
				{ rdmlineid_ = id; }
    RandomLineID		getRandomLineID() const
				{ return rdmlineid_; }
    void			setTrcKeyZSampling(const TrcKeyZSampling&,
						   TaskRunner* =0);
    const TrcKeyZSampling&	getTrcKeyZSampling() const
				{ return tkzs_; }
    Pos::GeomID			geomID() const;

    void			setInitialCentre( const uiWorldPoint& wp )
				{ initialcentre_ = wp; }
    void			setInitialX1PosPerCM( float val )
				{ initialx1pospercm_ = val; }
    void			setInitialX2PosPerCM( float val )
				{ initialx2pospercm_ = val; }
    void			setUpAux();

    const uiFlatViewStdControl* viewControl() const
				{ return viewstdcontrol_; }
    uiFlatViewStdControl*	viewControl()
				{ return viewstdcontrol_; }
    uiSlicePos2DView*		slicePos()
				{ return slicepos_; }
    const ZDomain::Def&		zDomain() const;
    SceneID			getSyncSceneID() const;

    virtual void		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;
    virtual void		setWinTitle(bool fromvisobjinfo);
				/*!<\param fromvisobjinfo if true, window title
				  will be set from visBase::DataObject info.*/

    static const char*		sKeyVDSelSpec()  { return "VD SelSpec"; }
    static const char*		sKeyWVASelSpec() { return "WVA SelSpec"; }
    static const char*		sKeyPos()	 { return "Position"; }

    Notifier<uiODViewer2D>	viewWinAvailable;
    Notifier<uiODViewer2D>	viewWinClosed;
    Notifier<uiODViewer2D>	dataChanged;
    Notifier<uiODViewer2D>	posChanged;

    void			getVwr2DObjIDs(TypeSet<Vis2DID>&) const;

    //Horizon 3D
    void			getHor3DVwr2DIDs(EM::ObjectID emid,
						 TypeSet<Vis2DID>&) const;
    void			removeHorizon3D(EM::ObjectID emid);
    void			getLoadedHorizon3Ds(
					TypeSet<EM::ObjectID>&) const;
    void			addHorizon3Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTrackingHorizon3D(EM::ObjectID);
    void			setupTrackingHorizon3D(EM::ObjectID);

    //Horizon2D
    void			getHor2DVwr2DIDs(EM::ObjectID emid,
						 TypeSet<Vis2DID>&) const;
    void			removeHorizon2D(EM::ObjectID emid);
    void			getLoadedHorizon2Ds(
					TypeSet<EM::ObjectID>&) const;
    void			addHorizon2Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTrackingHorizon2D(EM::ObjectID emid);
    void			setupTrackingHorizon2D(EM::ObjectID);

    //Fault
    void			getFaultVwr2DIDs(EM::ObjectID emid,
						 TypeSet<Vis2DID>&) const;
    void			removeFault(EM::ObjectID emid);
    void			getLoadedFaults(
					TypeSet<EM::ObjectID>&) const;
    void			addFaults(const TypeSet<EM::ObjectID>&);
    void			addNewTempFault(EM::ObjectID emid);
    void			setupNewTempFault(EM::ObjectID emid);

    //FaultStickSet
    void			getFaultSSVwr2DIDs(EM::ObjectID emid,
						   TypeSet<Vis2DID>&) const;
    void			removeFaultSS(EM::ObjectID emid);
    void			getLoadedFaultSSs(
					TypeSet<EM::ObjectID>&) const;
    void			addFaultSSs(const TypeSet<EM::ObjectID>&);
    void			addNewTempFaultSS(EM::ObjectID emid);
    void			setupNewTempFaultSS(EM::ObjectID emid);

    //FaultStickSet2D
    void			removeFaultSS2D(EM::ObjectID emid);
    void			getLoadedFaultSS2Ds(
					TypeSet<EM::ObjectID>&) const;
    void			addFaultSS2Ds(const TypeSet<EM::ObjectID>&);
    void			addNewTempFaultSS2D(EM::ObjectID emid);
    void			getFaultSS2DVwr2DIDs(EM::ObjectID emid,
						TypeSet<Vis2DID>&) const;
    void			setupNewTempFaultSS2D(EM::ObjectID emid);


    //PickSets
    void			getPickSetVwr2DIDs(const MultiID& mid,
						   TypeSet<Vis2DID>&) const;
    void			removePickSet(const MultiID&);
    void			getLoadedPickSets(TypeSet<MultiID>&) const;
    void			addPickSets(const TypeSet<MultiID>&);
    void			setupNewPickSet(const MultiID&);

protected:

    Viewer2DID			id_; /*!<Unique identifier */
    VisID			visid_; /*!<ID from 3D visualization */

    uiSlicePos2DView*				slicepos_	= nullptr;
    uiFlatViewStdControl*			viewstdcontrol_ = nullptr;
    ObjectSet<uiFlatViewAuxDataEditor>		auxdataeditors_;

    Attrib::SelSpec&		wvaselspec_;
    Attrib::SelSpec&		vdselspec_;
    RefMan<SeisFlatDataPack>	wvadp_;
    RefMan<SeisFlatDataPack>	vddp_;

    View2D::DataManager*	datamgr_;
    uiTreeFactorySet*		tifs_				= nullptr;
    uiODView2DTreeTop*		treetp_				= nullptr;
    uiFlatViewWin*		viewwin_			= nullptr;
    MouseCursorExchange*	mousecursorexchange_		= nullptr;
    FlatView::AuxData*		marker_				= nullptr;
    ZAxisTransform*		datatransform_			= nullptr;
    TrcKeyZSampling		tkzs_;
    uiString			basetxt_;
    uiODMain&			appl_;
    RandomLineID		rdmlineid_;
    int				voiidx_				= -1;
    SceneID			syncsceneid_;

    uiWorldPoint		initialcentre_;
    float			initialx1pospercm_		= mUdf(float);
    float			initialx2pospercm_		= mUdf(float);

    int				polyseltbid_			= -1;
    int				picksettingstbid_		= -1;
    bool			ispolyselect_			= true;
    bool			isvertical_			= true;

    mDeprecated("Use createDataPackForTransformedZSliceRM")
    DataPackID			createDataPackForTransformedZSlice(
						const Attrib::SelSpec&) const;
    RefMan<SeisFlatDataPack>	createDataPackForTransformedZSliceRM(
						const Attrib::SelSpec&) const;
    virtual void		createViewWin(bool isvert,bool needslicepos);
    virtual void		createTree(uiMainWin*);
    virtual void		createPolygonSelBut(uiToolBar*);
    void			createViewWinEditors();
    void			setDataPack(FlatDataPack*,
					    FlatView::Viewer::VwrDest,
					    bool isnew);
    void			setDataPack(FlatDataPack*,bool wva,
					    bool isnew);
    void			setDataPack(DataPackID,bool wva,bool isnew);
    void			setDataPack(DataPackID,
					    FlatView::Viewer::VwrDest,
					    bool isnew);
    void			adjustOthrDisp(bool wva,bool isnew);
    void			adjustOthrDisp(FlatView::Viewer::VwrDest,
					       bool isnew);
    void			removeAvailablePacks();
    void			rebuildTree();

    uiString			getInfoTitle() const;
    void			dispPropChangedCB(CallBacker*);
    void			winCloseCB(CallBacker*);
    void			applClosed(CallBacker*);
    void			posChg(CallBacker*);
    void			itmSelectionChangedCB(CallBacker*);
    void			selectionMode(CallBacker*);
    void			trackSetupCB(CallBacker*);
    void			handleToolClick(CallBacker*);
    void			removeSelected(CallBacker*);
    void			mouseCursorCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);

public:
    mDeprecated("No longer used")
    void		makeUpView(DataPackID,FlatView::Viewer::VwrDest);
    mDeprecated("Use method that takes FlatView::Viewer::VwrDest enum")
    void			setSelSpec(const Attrib::SelSpec*,bool wva);
    mDeprecated("No longer used")
    virtual void		setUpView(DataPackID,bool wva);
    mDeprecated("No longer used")
    DataPackID			getDataPackID(bool wva) const;
				/*!<Returns DataPackID of specified display if
				it has a valid one. Returns DataPackID of
				other display if both have same Attrib::SelSpec.
				Else, returns uiODViewer2D::createDataPack.*/
    mDeprecated("No longer used")
    DataPackID			createDataPack(bool wva) const;
    mDeprecated("No longer used")
    DataPackID			createDataPack(const Attrib::SelSpec&) const;
				/*!< Creates RegularFlatDataPack by getting
				TrcKeyZSampling from slicepos_. Uses the
				existing TrcKeyZSampling, if there is no
				slicepos_. Also transforms data if the 2D Viewer
				hasZAxisTransform(). */
    mDeprecated("No longer used")
    DataPackID			createFlatDataPack(DataPackID,int comp) const;
				/*!< Creates a FlatDataPack from SeisDataPack.
				Either a transformed or a non-transformed
				datapack can be passed. The returned datapack
				will always be in transformed domain if the
				viewer hasZAxisTransform(). */
    mDeprecated("No longer used")
    DataPackID			createFlatDataPack(const SeisDataPack&,
						   int comp) const;
				/*!< Creates a FlatDataPack from SeisDataPack.
				Either a transformed or a non-transformed
				datapack can be passed. The returned datapack
				will always be in transformed domain if the
				viewer hasZAxisTransform(). */
    mDeprecated("No longer used")
    DataPackID			createMapDataPack(const RegularFlatDataPack&);
    mDeprecated("Use method that takes FlatView::Viewer::VwrDest enum")
    bool			useStoredDispPars(bool wva);

};
