#pragma once
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
________________________________________________________________________

-*/

#include "uiodmainmod.h"

#include "datapack.h"
#include "emposid.h"
#include "integerid.h"
#include "trckeyzsampling.h"

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
class RegularFlatDataPack;
class TaskRunner;
class Vw2DDataManager;
class ZAxisTransform;

namespace Attrib	{ class SelSpec; }
namespace FlatView	{ class AuxData; }
namespace ZDomain	{ class Def; }

/*!
\brief A 2D Viewer.
*/

mExpClass(uiODMain) uiODViewer2D : public CallBacker
{ mODTextTranslationClass(uiODViewer2D);
public:
				uiODViewer2D(uiODMain&,int visid);
				~uiODViewer2D();

    mDeclInstanceCreatedNotifierAccess(uiODViewer2D);

    virtual void		setUpView(DataPack::ID,bool wva);
    void			setSelSpec(const Attrib::SelSpec*,bool wva);
    void			setMouseCursorExchange(MouseCursorExchange*);

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
    DataPack::ID		getDataPackID(bool wva) const;
				/*!<Returns DataPack::ID of specified display if
				it has a valid one. Returns DataPack::ID of
				other display if both have same Attrib::SelSpec.
				Else, returns uiODViewer2D::createDataPack.*/
    DataPack::ID		createDataPack(bool wva) const
				{ return createDataPack(selSpec(wva)); }
    DataPack::ID		createDataPack(const Attrib::SelSpec&) const;
				/*!< Creates RegularFlatDataPack by getting
				TrcKeyZSampling from slicepos_. Uses the
				existing TrcKeyZSampling, if there is no
				slicepos_. Also transforms data if the 2D Viewer
				hasZAxisTransform(). */
    DataPack::ID		createFlatDataPack(DataPack::ID,int comp) const;
				/*!< Creates a FlatDataPack from SeisDataPack.
				Either a transformed or a non-transformed
				datapack can be passed. The returned datapack
				will always be in transformed domain if the
				viewer hasZAxisTransform(). */
    DataPack::ID		createMapDataPack(const RegularFlatDataPack&);
    bool			useStoredDispPars(bool wva);
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
    int				getSyncSceneID() const;

    int				id_; /*!<Unique identifier */
    int				visid_; /*!<ID from 3D visualization */

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

    //FaultStickSet
    void			getFaultSSVwr2DIDs(EM::ObjectID emid,
						   TypeSet<int>& vw2dids) const;
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
						TypeSet<int>& vw2ds) const;
    void			setupNewTempFaultSS2D(EM::ObjectID emid);


    //PickSets
    void			getPickSetVwr2DIDs(const MultiID& mid,
						   TypeSet<int>& vw2ids ) const;
    void			removePickSet(const MultiID&);
    void			getLoadedPickSets(TypeSet<MultiID>&) const;
    void			addPickSets(const TypeSet<MultiID>&);
    void			setupNewPickSet(const MultiID&);

protected:

    uiSlicePos2DView*				slicepos_;
    uiFlatViewStdControl*			viewstdcontrol_;
    ObjectSet<uiFlatViewAuxDataEditor>		auxdataeditors_;

    Attrib::SelSpec&		wvaselspec_;
    Attrib::SelSpec&		vdselspec_;

    Vw2DDataManager*		datamgr_;
    uiTreeFactorySet*		tifs_;
    uiODVw2DTreeTop*		treetp_;
    uiFlatViewWin*		viewwin_;
    MouseCursorExchange*	mousecursorexchange_;
    FlatView::AuxData*		marker_;
    ZAxisTransform*		datatransform_;
    TrcKeyZSampling		tkzs_;
    uiString			basetxt_;
    uiODMain&			appl_;
    RandomLineID		rdmlineid_;
    int				voiidx_;
    int				syncsceneid_;

    uiWorldPoint		initialcentre_;
    float			initialx1pospercm_;
    float			initialx2pospercm_;

    int				edittbid_;
    int				polyseltbid_;
    int				picksettingstbid_;
    bool			ispolyselect_;
    bool			isvertical_;

    DataPack::ID		createDataPackForTransformedZSlice(
						const Attrib::SelSpec&) const;

    virtual void		createViewWin(bool isvert,bool needslicepos);
    virtual void		createTree(uiMainWin*);
    virtual void		createPolygonSelBut(uiToolBar*);
    void			createViewWinEditors();
    void			setDataPack(DataPack::ID,bool wva,bool isnew);
    void			adjustOthrDisp(bool wva,bool isnew);
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
};

