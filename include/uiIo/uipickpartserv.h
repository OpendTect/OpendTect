#ifndef uipickpartserv_h
#define uipickpartserv_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "uiapplserv.h"
#include "uistring.h"

#include "bufstringset.h"
#include "trckeysampling.h"
#include "ranges.h"
#include "multiid.h"

class BinIDValueSet;
class DataPointSet;
class RandLocGenPars;
class SurfaceInfo;
class uiImpExpPickSet;
class uiPickSetMan;
class uiPickSetMgr;
namespace Pick { class Set; class SetMgr; }
namespace PosInfo { class Line2DData; }


/*! \brief Service provider for application level - seismics */

mExpClass(uiIo) uiPickPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiPickPartServer);
public:
				uiPickPartServer(uiApplService&);
				~uiPickPartServer();

    const char*			name() const		{ return "Picks"; }

				// Services
    void			managePickSets();
    Pick::Set*			pickSet()		{ return ps_; }
    void			importSet();
    void			exportSet();

    bool			storePickSets();
    bool			storeNewPickSet(Pick::Set*&);
    bool			storePickSet(const Pick::Set&);
    bool			storePickSetAs(const Pick::Set&);
    bool			pickSetsStored() const;
    void			mergePickSets(MultiID&);

    void			fetchHors(bool);
    Pick::Set*			loadSet(const MultiID&);
    bool			reLoadSet(const MultiID&);
    bool			loadSets(TypeSet<MultiID>&,bool ispolygon);
    				//!< Load set(s) by user sel
    const Pick::Set*		createEmptySet(bool aspolygon);
    bool			create3DGenSet();
    bool			createRandom2DSet();
    void			setMisclassSet(const DataPointSet&);
    void			setPickSet(const Pick::Set&);
    void			fillZValsFrmHor(Pick::Set*,int);

    static int			evGetHorInfo2D();
    static int			evGetHorInfo3D();
    static int			evGetHorDef3D();
    static int		        evGetHorDef2D();
    static int		        evFillPickSet();
    static int			evDisplayPickSet();

				// Interaction stuff
    BinIDValueSet&			genDef()	{ return gendef_; }
    MultiID			pickSetID() const	{ return picksetid_; }

    ObjectSet<SurfaceInfo>& 	horInfos()		{ return hinfos_; }
    const ObjectSet<MultiID>&	selHorIDs() const	{ return selhorids_; }
    TrcKeySampling		selTrcKeySampling() const
				{ return selhs_; }
    MultiID			horID()			{ return horid_; }

    TypeSet<BufferStringSet>&	lineNames()		{ return linenms_; }
    BufferStringSet&		selectLines()		{ return selectlines_; }
    TypeSet<Coord>&		getPos2D()		{ return coords2d_; }
    TypeSet< Interval<float> >& getHor2DZRgs()		{ return hor2dzrgs_; }

protected:

    Pick::SetMgr&		psmgr_;
    uiPickSetMgr&		uipsmgr_;
    BinIDValueSet& 		gendef_;

    ObjectSet<SurfaceInfo> 	hinfos_;
    ObjectSet<MultiID>		selhorids_;
    TrcKeySampling			selhs_;
    Pick::Set*			ps_;
    MultiID			picksetid_;
    MultiID			horid_;

    TypeSet<BufferStringSet>	linenms_;
    BufferStringSet		selectlines_;
    TypeSet<Coord>		coords2d_;
    TypeSet< Interval<float> >	hor2dzrgs_;

    uiImpExpPickSet*		imppsdlg_;
    uiImpExpPickSet*		exppsdlg_;
    uiPickSetMan*		manpicksetsdlg_;

    void			survChangedCB(CallBacker*);
    void			importReadyCB(CallBacker*);
    bool                        mkRandLocs2D(Pick::Set&,const RandLocGenPars&);
};

#endif
