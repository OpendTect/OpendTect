#ifndef uipickpartserv_h
#define uipickpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uipickpartserv.h,v 1.15 2003-02-03 14:07:09 nanne Exp $
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
    void		setMisclassSet(const TypeSet<BinIDZValue>&);

    ObjectSet<SurfaceInfo>& horInfos()		{ return hinfos; }
    const MultiID&	selHorID() const	{ return selhorid; }
    const BinIDRange*	selBinIDRange() const	{ return selbr; }
    TypeSet<BinIDValue>& horDef()
    			{ return usehd1 ? hordef : hordef2; }

protected:

    PickSetGroup&	psg;
    MultiID		psgid;
    UserIDSet		avsets;
    BoolTypeSet		selsets;
    Color&		pickcolor;

    ObjectSet<SurfaceInfo> hinfos;
    TypeSet<BinIDValue> hordef;
    TypeSet<BinIDValue> hordef2;
    bool		usehd1;
    MultiID		selhorid;
    const BinIDRange*	selbr;

    bool		mkRandLocs(PickSet&,const RandLocGenPars&);
};


#endif
