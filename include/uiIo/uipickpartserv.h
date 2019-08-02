#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2002
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uiapplserv.h"
#include "uistring.h"

#include "bufstringset.h"
#include "trckeysampling.h"
#include "ranges.h"
#include "dbkey.h"
#include "pickset.h"

class BinnedValueSet;
class DataPointSet;
class RandLocGenPars;
class SurfaceInfo;
class uiImpExpPickSet;
class uiPickSetMan;
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

    bool			storePickSets(int polyopt=0,const char* cat=0);
					//!< 0=all, -1 no poly's, 1 only poly's
    bool			storePickSet(const Pick::Set&);
    bool			storePickSetAs(const Pick::Set&);
    void			mergePickSets(DBKey&);

    void			fetchHors(bool);
    RefMan<Pick::Set>		loadSet(const DBKey&);
    bool			loadSets(DBKeySet&,bool polygons,
					 const char* cat=0,
					 const char* transl=0);
					//!< You have to ref/unref the sets
    RefMan<Pick::Set>		createEmptySet(bool aspolygon);
    RefMan<Pick::Set>		create3DGenSet();
    RefMan<Pick::Set>		createRandom2DSet();
    void			setMisclassSet(const DataPointSet&);
    void			fillZValsFrmHor(Pick::Set*,int);

    static int			evGetHorInfo2D();
    static int			evGetHorInfo3D();
    static int			evGetHorDef3D();
    static int		        evGetHorDef2D();
    static int		        evFillPickSet();
    static int			evDisplayPickSet();

				// Interaction stuff
    BinnedValueSet&		genDef()		{ return gendef_; }
    DBKey			pickSetID() const	{ return picksetid_; }

    ObjectSet<SurfaceInfo>&	horInfos()		{ return hinfos_; }
    const DBKeySet&		selHorIDs() const	{ return selhorids_; }
    TrcKeySampling		selTrcKeySampling() const
				{ return selhs_; }
    DBKey			horID()			{ return horid_; }

    TypeSet<BufferStringSet>&	lineNames()		{ return linenms_; }
    BufferStringSet&		selectLines()		{ return selectlines_; }
    TypeSet<Coord>&		getPos2D()		{ return coords2d_; }
    GeomIDSet&			getGeomIDs2D()		{ return geomids2d_; }
    TypeSet< Interval<float> >& getHor2DZRgs()		{ return hor2dzrgs_; }

protected:

    BinnedValueSet&		gendef_;

    ObjectSet<SurfaceInfo>	hinfos_;
    DBKeySet			selhorids_;
    TrcKeySampling		selhs_;
    Pick::Set*			ps_;
    DBKey			picksetid_;
    DBKey			horid_;

    TypeSet<BufferStringSet>	linenms_;
    BufferStringSet		selectlines_;
    TypeSet<Coord>		coords2d_;
    GeomIDSet			geomids2d_;
    TypeSet< Interval<float> >	hor2dzrgs_;

    uiImpExpPickSet*		imppsdlg_;
    uiImpExpPickSet*		exppsdlg_;
    uiPickSetMan*		manpicksetsdlg_;

    void			survChangedCB(CallBacker*);
    void			importReadyCB(CallBacker*);
    void                        mkRandLocs2D(CallBacker*);
    bool			doLoadSets(DBKeySet&);
    bool			doSaveAs(const DBKey&,const Pick::Set*);

};
