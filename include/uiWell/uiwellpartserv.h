#ifndef uiwellpartserv_h
#define uiwellpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellpartserv.h,v 1.11 2004-05-06 11:16:47 bert Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"

class MultiID;
template <class T> class Interval;

/*! \brief Part Server for Wells */

class uiWellPartServer : public uiApplPartServer
{
public:
				uiWellPartServer(uiApplService&);

    const char*			name() const		{ return "Wells"; }

    				// Services
    bool			importWell();

    void			manageWells();
    bool			selectWells(ObjectSet<MultiID>&);

    void			selectLogs(const MultiID&,int&,int&);

protected:

};

/*!\mainpage Well User Interface

  Apart from nice visualisation import and management of well data must be
  done. The uiWellPartServer delivers the services needed.

*/


#endif
