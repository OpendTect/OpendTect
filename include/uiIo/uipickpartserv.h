#ifndef uipickpartserv_h
#define uipickpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uipickpartserv.h,v 1.6 2002-04-12 10:10:16 nanne Exp $
________________________________________________________________________

-*/

#include <uiapplserv.h>
#include <multiid.h>
#include <uidset.h>

class PickSetGroup;
class Color;


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

			// Interaction stuff
    PickSetGroup&	group()				{ return psg; }
    			//!< 1) Result of fetchPickSets()
    			//!< 2) Must be filled on evFetchPicks
    UserIDSet&		availableSets()			{ return avsets; }
    const BoolTypeSet& selectedSets() const		{ return selsets; }
    MultiID&		psgID()				{ return psgid; }
    const Color&	getPickColor()			{ return pickcolor; }

protected:

    PickSetGroup&	psg;
    MultiID		psgid;
    UserIDSet		avsets;
    BoolTypeSet		selsets;
    Color&		pickcolor;

};


#endif
