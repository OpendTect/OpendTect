#ifndef uipickpartserv_h
#define uipickpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uipickpartserv.h,v 1.23 2006-05-16 16:28:22 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "ranges.h"
#include "multiid.h"
#include "binidvalset.h"
#include "bufstringset.h"

class Color;
class BinIDRange;
class SurfaceInfo;
class RandLocGenPars;
namespace Pick { class Set; };


/*! \brief Service provider for application level - seismics */

class uiPickPartServer : public uiApplPartServer
{
public:
				uiPickPartServer(uiApplService&);
				~uiPickPartServer();

    const char*			name() const		{ return "Picks"; }

				// Services
    void			impexpSet(bool);
    bool			fetchSets();	//!< Fetch sets by user sel
    bool			storeSets();	//!< Stores all changed sets

    static const int		evGetAvailableSets;
    static const int		evFetchPicks;
    static const int		evGetHorInfo;
    static const int		evGetHorDef;

				// Interaction stuff
    Pick::Set&			set()			{ return ps; }
    				//!< Must be filled on evFetchPicks
    ObjectSet<Pick::Set>&	setsFetched()		{ return pssfetched; }
    				//!< Result of fetchPickSets(). Become yours.
    BufferStringSet&		availableSets()		{ return avsets; }
    const BoolTypeSet& 		selectedSets() const	{ return selsets; }
    MultiID&			psID()			{ return psid; }
    const Color&		getPickColor()		{ return pickcolor; }
    bool			storeSet();	//!< Stores current set
    bool			storeSetAs();	//!< Allows chosing new name
    void			renameSet(const char*,BufferString&);
    void			setMisclassSet(const BinIDValueSet&);

    BinIDValueSet&		genDef() 		{ return gendef; }
    ObjectSet<SurfaceInfo>& 	horInfos()		{ return hinfos; }
    const ObjectSet<MultiID>&	selHorIDs() const	{ return selhorids; }
    const BinIDRange*		selBinIDRange() const	{ return selbr; }

protected:

    Pick::Set&			ps;
    MultiID			psid;
    BufferStringSet		avsets;
    BoolTypeSet			selsets;
    Color&			pickcolor;
    ObjectSet<Pick::Set>	pssfetched;

    BinIDValueSet 		gendef;
    ObjectSet<SurfaceInfo> 	hinfos;
    ObjectSet<MultiID>		selhorids;
    const BinIDRange*		selbr;

    bool			mkRandLocs(Pick::Set&,const RandLocGenPars&);
};


#endif
