#ifndef uiwellpartserv_h
#define uiwellpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellpartserv.h,v 1.4 2003-10-27 23:10:02 bert Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"

class MultiID;
template <class T> class Interval;

/*! \brief Service provider for application level - Wells */

class uiWellPartServer : public uiApplPartServer
{
public:
				uiWellPartServer(uiApplService&);

    const char*			name() const		{ return "Wells"; }

    				// Services
    bool			importWell();

    void			manageWells();
    bool			selectWells(ObjectSet<MultiID>&);

    void			selectLogs(const MultiID&,int&,int&,
	    				   Interval<float>&);

};

#endif
