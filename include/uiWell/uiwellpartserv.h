#ifndef uiwellpartserv_h
#define uiwellpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2003
 RCS:           $Id: uiwellpartserv.h,v 1.15 2005-10-24 15:17:25 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"

class MultiID;
class Coord3;
template <class T> class Interval;

/*! \brief Part Server for Wells */

class uiWellPartServer : public uiApplPartServer
{
public:
				uiWellPartServer(uiApplService&);

    const char*			name() const		{ return "Wells"; }

    				// Services
    bool			importTrack();
    bool			importLogs();
    bool			importMarkers();

    void			manageWells();
    bool			selectWells(ObjectSet<MultiID>&);

    void			selectLogs(const MultiID&,int&,int&);
    bool			hasLogs(const MultiID&) const;

    void			selectWellCoordsForRdmLine();
    
    void			createWellFromPicks();
    const char*			askWellName();

    bool			storeWell(const TypeSet<Coord3>&, const char*);

    static const int            evPreviewRdmLine;

protected:

};

/*!\mainpage Well User Interface

  Apart from nice visualisation import and management of well data must be
  done. The uiWellPartServer delivers the services needed.

*/


#endif
