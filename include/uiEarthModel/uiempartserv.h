#ifndef uiempartserv_h
#define uiempartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Sep 2002
 RCS:           $Id: uiempartserv.h,v 1.1 2002-09-17 13:26:44 bert Exp $
________________________________________________________________________

-*/

#include <uiapplserv.h>
class HorizonInfo;
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
    bool		selectHorizon(MultiID&);
    bool		selectWellTracks(ObjectSet<MultiID>&);
    bool		importHorizon();
    bool		exportHorizon(const ObjectSet<HorizonInfo>&);
    bool		storePickSets();
    bool		importWellTrack();

    static const int	evGetHorData;

			// Interaction stuff
    int			selHorID() const		{ return selhorid_; }
    ObjectSet< TypeSet<BinIDZValue> >& horData()	{ return horbidzvs_; }


protected:

    ObjectSet< TypeSet<BinIDZValue> >	horbidzvs_;
    int					selhorid_;
    

};


#endif
