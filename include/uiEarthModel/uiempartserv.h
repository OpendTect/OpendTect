#ifndef uiempartserv_h
#define uiempartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Sep 2002
 RCS:           $Id: uiempartserv.h,v 1.5 2003-02-03 14:10:17 nanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "multiid.h"
class SurfaceInfo;
class BinIDValue;
class BinIDRange;
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
    bool		importLMKFault();
    bool		selectFault(MultiID&);
    void		getSurfaceInfo(ObjectSet<SurfaceInfo>&);
    void		getSurfaceDef(const MultiID&, 
	    			      ObjectSet< TypeSet<BinIDValue> >&, 
				      const BinIDRange*) const;

    static const int	evGetHorData;

			// Interaction stuff
    const MultiID&	selHorID() const		{ return selhorid_; }
    ObjectSet< TypeSet<BinIDZValue> >& horData()	{ return horbidzvs_; }


protected:

    ObjectSet< TypeSet<BinIDZValue> >	horbidzvs_;
    MultiID		selhorid_;
    
};


#endif
