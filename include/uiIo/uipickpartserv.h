#ifndef uiseisman_h
#define uiseisman_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uipickpartserv.h,v 1.1 2002-02-08 22:00:56 bert Exp $
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

    bool		fetchPickSets();
    bool		storePickSets();

    static const int	evGetAvailableSets;
    static const int	evFetchPicks;

    PickSetGroup&	group()				{ return psg; }
    UserIDSet&		availableSets()			{ return avsets; }
    const TypeSet<bool>& selectedSets() const		{ return selsets; }
    MultiID&		psgID()				{ return psgid; }

protected:

    PickSetGroup&	psg;
    MultiID		psgid;
    UserIDSet		avsets;
    TypeSet<bool>	selsets;

};


#endif
