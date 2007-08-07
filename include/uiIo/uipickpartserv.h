#ifndef uipickpartserv_h
#define uipickpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uipickpartserv.h,v 1.29 2007-08-07 04:45:31 cvsraman Exp $
________________________________________________________________________

-*/

#include "uipicksetmgr.h"
#include "uiapplserv.h"
#include "ranges.h"
#include "multiid.h"
#include "binidvalset.h"
#include "bufstringset.h"

class Color;
class IOObj;
class BinIDRange;
class SurfaceInfo;
class RandLocGenPars;
namespace Pick { class Set; class SetMgr; };


/*! \brief Service provider for application level - seismics */

class uiPickPartServer  : public uiApplPartServer
			, public uiPickSetMgr
{
public:
				uiPickPartServer(uiApplService&);
				~uiPickPartServer();

    const char*			name() const		{ return "Picks"; }

				// Services
    void			managePickSets();
    Pick::Set*			pickSet()		{ return ps_; }
    void			impexpSet(bool import);
    void			fetchAllHors();
    bool			fetchSets();	//!< Fetch set(s) by user sel
    void			setMisclassSet(const BinIDValueSet&);
    void			fillZValsFrmHor(Pick::Set*,int);

    static const int		evGetHorInfo;
    static const int            evGetAllHorInfo;
    static const int		evGetHorDef;
    static const int            evFillPickSet;


				// Interaction stuff
    BinIDValueSet&		genDef() 		{ return gendef_; }
    ObjectSet<SurfaceInfo>& 	horInfos()		{ return hinfos_; }
    ObjectSet<SurfaceInfo>&     allhorInfos()		{ return allhinfos_; }
    const ObjectSet<MultiID>&	selHorIDs() const	{ return selhorids_; }
    const BinIDRange*		selBinIDRange() const	{ return selbr_; }
    MultiID			horID()			{ return horid_; }

    uiParent*			parent()
    				{ return appserv().parent(); }

protected:

    BinIDValueSet 		gendef_;
    ObjectSet<SurfaceInfo> 	hinfos_;
    ObjectSet<SurfaceInfo>      allhinfos_;
    ObjectSet<MultiID>		selhorids_;
    const BinIDRange*		selbr_;
    Pick::Set*			ps_;
    MultiID			horid_;

    bool			mkRandLocs(Pick::Set&,const RandLocGenPars&);
};


#endif
