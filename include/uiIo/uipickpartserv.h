#ifndef uipickpartserv_h
#define uipickpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uipickpartserv.h,v 1.13 2002-10-25 08:58:35 bert Exp $
________________________________________________________________________

-*/

#include <uiapplserv.h>
#include <multiid.h>
#include <uidset.h>
#include <position.h>

class Color;
class PickSet;
class BinIDRange;
class PickSetGroup;
class RandLocGenPars;
class SurfaceInfo;


/*! \brief Service provider for application level - seismics */

class uiPickPartServer : public uiApplPartServer
{
public:
			uiPickPartServer(uiApplService&);
			~uiPickPartServer();

    const char*		name() const			{ return "Picks"; }

			// Services
    void		importPickSet();
    bool		fetchPickSets();
    bool		storePickSets();

    static const int	evGetAvailableSets;
    static const int	evFetchPicks;
    static const int	evGetHorInfo;
    static const int	evGetHorDef;

			// Interaction stuff
    PickSetGroup&	group()				{ return psg; }
    			//!< 1) Result of fetchPickSets()
    			//!< 2) Must be filled on evFetchPicks
    UserIDSet&		availableSets()			{ return avsets; }
    const BoolTypeSet& selectedSets() const		{ return selsets; }
    MultiID&		psgID()				{ return psgid; }
    const Color&	getPickColor()			{ return pickcolor; }
    bool		storeSinglePickSet(PickSet*);
    void		renamePickset(const char*,BufferString&);

    ObjectSet<SurfaceInfo>& horInfos()		{ return hinfos; }
    int			selHorID() const	{ return selhorid; }
    const BinIDRange*	selBinIDRange() const	{ return selbr; }
    TypeSet<BinIDZValue>& horDef()
    			{ return usehd1 ? hordef : hordef2; }

protected:

    PickSetGroup&	psg;
    MultiID		psgid;
    UserIDSet		avsets;
    BoolTypeSet		selsets;
    Color&		pickcolor;

    ObjectSet<SurfaceInfo> hinfos;
    TypeSet<BinIDZValue> hordef;
    TypeSet<BinIDZValue> hordef2;
    bool		usehd1;
    int			selhorid;
    const BinIDRange*	selbr;

    bool		mkRandLocs(PickSet&,const RandLocGenPars&);
};


#endif
