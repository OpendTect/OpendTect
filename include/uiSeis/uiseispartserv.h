#ifndef uiseisman_h
#define uiseisman_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uiseispartserv.h,v 1.3 2002-06-19 15:42:02 bert Exp $
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

    enum ExternalType	{ SegY, SeisWorks, GeoFrame, CBVS };
    bool		isAvailable(ExternalType) const;
    bool		importSeis(ExternalType);
    bool		exportSeis(ExternalType);

    bool		mergeSeis();
    void		manageSeismics();

protected:

    MultiID		segyid;

    bool		ioSeis(ExternalType,bool);

};


#endif
