#ifndef uiwellpartserv_h
#define uiwellpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellpartserv.h,v 1.1 2003-08-29 07:24:49 nanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"

class MultiID;

/*! \brief Service provider for application level - Wells */

class uiWellPartServer : public uiApplPartServer
{
public:
				uiWellPartServer(uiApplService&);

    const char*			name() const		{ return "Wells"; }

    enum ExternalType		{ Ascii, SeisWorks, GeoFrame };

    				// Services
    bool			importWell(ExternalType);
    bool			exportWell(ExternalType);

    void			manageWells();
    bool			selectWells(ObjectSet<MultiID>&);

protected:

    bool			ioWell(ExternalType,bool);

};

#endif
