#ifndef uiempartserv_h
#define uiempartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Sep 2002
 RCS:           $Id: uiempartserv.h,v 1.6 2003-02-11 09:56:30 nanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
class SurfaceInfo;
class BinIDValue;
class BinIDRange;
class BinIDZValue;
class MultiID;


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
    int			selVisID() const		{ return selvisid_; }
    ObjectSet< TypeSet<BinIDZValue> >& horData()	{ return horbidzvs_; }


protected:

    ObjectSet< TypeSet<BinIDZValue> >	horbidzvs_;
    int			selvisid_;
    
};


#endif
