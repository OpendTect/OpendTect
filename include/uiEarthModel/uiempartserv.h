#ifndef uiempartserv_h
#define uiempartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Sep 2002
 RCS:           $Id: uiempartserv.h,v 1.10 2003-08-04 13:39:09 nanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
class BinIDRange;
class BinIDValue;
class BinIDZValue;
class BufferString;
class MultiID;
class uiPopupMenu;
class SurfaceInfo;

namespace EM { class SurfaceIODataSelection; };

/*! \brief Service provider for application level - seismics */

class uiEMPartServer : public uiApplPartServer
{
public:
			uiEMPartServer(uiApplService&);
			~uiEMPartServer();

    const char*		name() const			{ return "EarthModel"; }

    enum ExternalType   { Ascii, SeisWorks, GeoFrame };

			// Services
    bool		importHorizon(ExternalType);
    bool		exportHorizon(ExternalType);

    bool		selectHorizon(MultiID&);
    bool		loadAuxData(const MultiID&,int);
    int			createAuxDataSubMenu(uiPopupMenu&,int,const MultiID&);
    bool		importWellTrack();
    bool		selectWellTracks(ObjectSet<MultiID>&);
    bool		importLMKFault();
    bool		selectFault(MultiID&);
    bool		loadSurface(const MultiID&,
	    			    const EM::SurfaceIODataSelection* s=0);
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

    bool		ioHorizon(uiEMPartServer::ExternalType,bool);
    
};


#endif
