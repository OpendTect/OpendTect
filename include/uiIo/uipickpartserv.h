#ifndef uipickpartserv_h
#define uipickpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uipickpartserv.h,v 1.9 2002-08-05 14:36:38 bert Exp $
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


/*! \brief Service provider for application level - seismics */

class uiPickPartServer : public uiApplPartServer
{
public:
			uiPickPartServer(uiApplService&);
			~uiPickPartServer();

    const char*		name() const			{ return "Picks"; }

			// Services
    bool		fetchPickSets();
    bool		storePickSets();

    static const int	evGetAvailableSets;
    static const int	evFetchPicks;
    static const int	evGetHorNames;
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

    ObjectSet<BufferString> horNames()			{ return hornms; }
    TypeSet<BinIDValue>& horDef()			{ return hordef; }
    const char*		selHor() const			{ return selhor; }
    const BinIDRange*	selBinIDRange() const		{ return selbr; }

protected:

    PickSetGroup&	psg;
    MultiID		psgid;
    UserIDSet		avsets;
    BoolTypeSet		selsets;
    Color&		pickcolor;

    ObjectSet<BufferString> hornms;
    TypeSet<BinIDValue> hordef;
    const char*		selhor;
    const BinIDRange*	selbr;

    bool		mkRandLocs(PickSet&,const RandLocGenPars&);
};


#endif
