#ifndef uiseisman_h
#define uiseisman_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiseispartserv.h,v 1.1 2002-02-08 22:00:56 bert Exp $
________________________________________________________________________

-*/

#include <uiapplserv.h>
#include <multiid.h>


/*! \brief Service provider for application level - seismics */

class uiSeisPartServer : public uiApplPartServer
{
public:
			uiSeisPartServer( uiApplService& a )
			: uiApplPartServer(a)		{}

    const char*		name() const			{ return "Seismics"; }

    enum ExternalType	{ SegY, SeisWorks, GeoFrame };
    bool		isAvailable(ExternalType) const;
    bool		importSeis(ExternalType);
    bool		exportSeis(ExternalType);

    bool		mergeSeis();

protected:

    MultiID		segyid;

    bool		ioSeis(ExternalType,bool);

};


#endif
