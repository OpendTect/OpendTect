#ifndef uiempartserv_h
#define uiempartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Sep 2002
 RCS:           $Id: uiempartserv.h,v 1.8 2003-07-16 09:58:11 nanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
class SurfaceInfo;
class BinIDValue;
class BinIDRange;
class BinIDZValue;
class MultiID;
class BufferString;


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
    bool		loadSurface(const MultiID&);
    void		getSurfaceInfo(ObjectSet<SurfaceInfo>&);
    void		getSurfaceDef(const MultiID&, 
	    			      ObjectSet< TypeSet<BinIDValue> >&, 
				      const BinIDRange*) const;

    bool		storeSurface(const MultiID&);
    void		setDataVal(const MultiID&,
	    			   ObjectSet< TypeSet<BinIDZValue> >&,
				   const char*);
    bool		getDataVal(const MultiID&,
	    			   ObjectSet< TypeSet<BinIDZValue> >&,
				   BufferString&);

    static const int	evGetHorData;

			// Interaction stuff
    int			selVisID() const		{ return selvisid_; }
    ObjectSet< TypeSet<BinIDZValue> >& horData()	{ return horbidzvs_; }


protected:

    ObjectSet< TypeSet<BinIDZValue> >	horbidzvs_;
    int			selvisid_;
    
};


#endif
