#ifndef uiwellpartserv_h
#define uiwellpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellpartserv.h,v 1.10 2004-04-29 16:41:46 nanne Exp $
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

    static const char*		unitstr;

protected:

};

/*!\mainpage Well User Interface

  Apart from nice visualisation import and management of well data must be
  done. The uiWellPartServer delivers the services needed.

*/


#endif
