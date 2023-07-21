#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiapplserv.h"
#include "uistring.h"

#include "bufstringset.h"
#include "trckeysampling.h"
#include "ranges.h"
#include "multiid.h"

class uiCreatePicks;
class uiGenPosPicks;
class uiGenRandPicks2D;
class uiImpExpPickSet;
class uiPickSetMan;
class uiPickSetMgr;
class uiPickSetMgrInfoDlg;

class BinIDValueSet;
class DataPointSet;
class RandLocGenPars;
class SurfaceInfo;

namespace Pick { class Set; class SetMgr; }
namespace PosInfo { class Line2DData; }


/*! \brief Service provider for application level - seismics */

mExpClass(uiIo) uiPickPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiPickPartServer);
public:
				uiPickPartServer(uiApplService&);
				~uiPickPartServer();

    const char*			name() const override	{ return "Picks"; }

				// Services
    void			managePickSets();
    void			showPickSetMgrInfo();

    RefMan<Pick::Set>		pickSet();
    void			importSet();
    void			exportSet();

    bool			storePickSets();
    bool			storeNewPickSet(const Pick::Set&);
    bool			storePickSet(const Pick::Set&);
    bool			storePickSetAs(const Pick::Set&);
    bool			pickSetsStored() const;
    void			mergePickSets(MultiID&);

    void			fetchHors(bool);
    RefMan<Pick::Set>		loadSet(const MultiID&);
    bool			reLoadSet(const MultiID&);
    bool			loadSets(TypeSet<MultiID>&,bool ispolygon);
    				//!< Load set(s) by user sel
    void			createEmptySet(bool aspolygon);
    void			create3DGenSet();
    void			createRandom2DSet();
    void			setMisclassSet(const DataPointSet&);
    void			setPickSet(const Pick::Set&);
    void			fillZValsFromHor(Pick::Set&,int);

    static int			evGetHorInfo2D();
    static int			evGetHorInfo3D();
    static int			evGetHorDef3D();
    static int			evGetHorDef2D();
    static int			evFillPickSet();
    static int			evDisplayPickSet();

				// Interaction stuff
    BinIDValueSet&		genDef()		{ return gendef_; }
    MultiID			pickSetID() const	{ return picksetid_; }

    ObjectSet<SurfaceInfo>& 	horInfos()		{ return hinfos_; }
    const ObjectSet<MultiID>&	selHorIDs() const	{ return selhorids_; }
    TrcKeySampling		selTrcKeySampling() const { return selhs_; }
    MultiID			horID()			{ return horid_; }

    TypeSet<BufferStringSet>&	lineNames()		{ return linenms_; }
    BufferStringSet&		selectLines()		{ return selectlines_; }
    TypeSet<Coord>&		getPos2D()		{ return coords2d_; }
    TypeSet<BinID>&		getTrcPos2D()		{ return trcpos2d_; }
    TypeSet< Interval<float> >& getHor2DZRgs()		{ return hor2dzrgs_; }

protected:

    Pick::SetMgr&		psmgr_;
    uiPickSetMgr&		uipsmgr_;
    BinIDValueSet& 		gendef_;

    ObjectSet<SurfaceInfo> 	hinfos_;
    ObjectSet<MultiID>		selhorids_;
    TrcKeySampling		selhs_;
    RefMan<Pick::Set>		ps_;
    MultiID			picksetid_;
    MultiID			horid_;

    TypeSet<BufferStringSet>	linenms_;
    BufferStringSet		selectlines_;
    TypeSet<Coord>		coords2d_;
    TypeSet<BinID>		trcpos2d_;
    TypeSet< Interval<float> >	hor2dzrgs_;

    uiImpExpPickSet*		imppsdlg_		= nullptr;
    uiImpExpPickSet*		exppsdlg_		= nullptr;
    uiPickSetMan*		manpicksetsdlg_		= nullptr;
    uiPickSetMgrInfoDlg*	setmgrinfodlg_		= nullptr;

    uiGenPosPicks*		genpsdlg_		= nullptr;
    uiCreatePicks*		emptypsdlg_		= nullptr;
    uiGenRandPicks2D*		genps2ddlg_		= nullptr;

    void			cleanup();
    void			survChangedCB(CallBacker*);
    void			importReadyCB(CallBacker*);

    void			create2DCB(CallBacker*);
    bool			mkRandLocs2D(Pick::Set&,const RandLocGenPars&);
    bool			mkRandLocs2DBetweenHors(Pick::Set&,
							const RandLocGenPars&);
};
