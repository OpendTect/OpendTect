#ifndef uiempartserv_h
#define uiempartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Sep 2002
 RCS:           $Id: uiempartserv.h,v 1.3 2002-09-20 09:19:01 nanne Exp $
________________________________________________________________________

-*/

#include <uiapplserv.h>
class SurfaceInfo;
class MultiID;
class BinIDZValue;


/*! \brief Service provider for application level - seismics */

class uiEMPartServer : public uiApplPartServer
{
public:
			uiEMPartServer(uiApplService&);
			~uiEMPartServer();

    const char*		name() const			{ return "EarthModel"; }

			// Services
    bool		importHorizon();
    bool		selectHorizon(MultiID&);
    bool		exportHorizon(const ObjectSet<SurfaceInfo>&);
    bool		importWellTrack();
    bool		selectWellTracks(ObjectSet<MultiID>&);
    bool		selectFault(MultiID&);

    static const int	evGetHorData;

			// Interaction stuff
    int			selHorID() const		{ return selhorid_; }
    ObjectSet< TypeSet<BinIDZValue> >& horData()	{ return horbidzvs_; }


protected:

    ObjectSet< TypeSet<BinIDZValue> >	horbidzvs_;
    int					selhorid_;
    

};


#endif
