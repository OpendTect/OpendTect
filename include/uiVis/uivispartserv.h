#ifndef uiattribpartserv_h
#define uiattribpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Mar 2002
 RCS:           $Id: uivispartserv.h,v 1.1 2002-03-25 16:04:26 bert Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "multiid.h"
class UserIDSet;
class PickSet;
class PickSetGroup;


/*! \brief Service provider for application level - Visutes */

class uiVisPartServer : public uiApplPartServer
{
public:
			uiVisPartServer(uiApplService&);
			~uiVisPartServer();
    const char*		name() const		{ return "Visualisation"; }

    bool		setPicks(const PickSetGroup&);
    void		getPickSets(UserIDSet&);
    void		getPickSetData(const char*,PickSet&);

};

#endif
