#ifndef uipickpartserv_h
#define uipickpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uipickpartserv.h,v 1.25 2006-06-28 13:32:49 cvsbert Exp $
________________________________________________________________________

-*/

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

class uiPickPartServer : public uiApplPartServer
{
public:
				uiPickPartServer(uiApplService&);
				~uiPickPartServer();

    const char*			name() const		{ return "Picks"; }

				// Services
    void			impexpSet(bool import);
    bool			fetchSets();	//!< Fetch set(s) by user sel
    bool			storeSets();	//!< Stores all changed sets
    bool			storeSet(const Pick::Set&);
    bool			storeSetAs(const Pick::Set&);
    void			setMisclassSet(const BinIDValueSet&);
    bool			pickSetsStored() const;

    static const int		evGetHorInfo;
    static const int		evGetHorDef;


				// Interaction stuff
    BinIDValueSet&		genDef() 		{ return gendef; }
    ObjectSet<SurfaceInfo>& 	horInfos()		{ return hinfos; }
    const ObjectSet<MultiID>&	selHorIDs() const	{ return selhorids; }
    const BinIDRange*		selBinIDRange() const	{ return selbr; }

protected:

    BinIDValueSet 		gendef;
    ObjectSet<SurfaceInfo> 	hinfos;
    ObjectSet<MultiID>		selhorids;
    const BinIDRange*		selbr;

    Pick::SetMgr&		setmgr;

    bool			storeNewSet(Pick::Set*&) const;
    bool			mkRandLocs(Pick::Set&,const RandLocGenPars&);
    IOObj*			getSetIOObj(const Pick::Set&) const;
    bool			doStore(const Pick::Set&,const IOObj&) const;
};


#endif
