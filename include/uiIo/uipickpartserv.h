#ifndef uiseisman_h
#define uiseisman_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uipickpartserv.h,v 1.3 2002-03-22 15:08:43 bert Exp $
________________________________________________________________________

-*/

#include <uiapplserv.h>
#include <multiid.h>
#include <uidset.h>

class PickSetGroup;


/*! \brief Service provider for application level - seismics */

class uiPickPartServ : public uiApplPartServer
{
public:
			uiPickPartServ(uiApplService&);
			~uiPickPartServ();

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

protected:

    PickSetGroup&	psg;
    MultiID		psgid;
    UserIDSet		avsets;
    BoolTypeSet		selsets;

};


#endif
