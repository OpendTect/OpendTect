#ifndef uiempartserv_h
#define uiempartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          Sep 2002
 RCS:           $Id: uiempartserv.h,v 1.14 2003-08-22 11:27:27 nanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "ranges.h"
class BinIDRange;
class BinIDValue;
class BinIDZValue;
class BufferString;
class MultiID;
class uiPopupMenu;
class SurfaceInfo;
class BinID;

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

    void		manageSurfaces();
    bool		selectHorizon(MultiID&);
    bool		loadAuxData(const MultiID&,int);
    int			createAuxDataSubMenu(uiPopupMenu&,int,const MultiID&,
	    				     bool);
    bool		importWellTrack();
    bool		importLMKFault();
    bool		selectFault(MultiID&);
    bool		loadSurface(const MultiID&,
	    			    const EM::SurfaceIODataSelection* s=0);
    void		getSurfaceInfo(ObjectSet<SurfaceInfo>&);
    void		getSurfaceDef(const ObjectSet<MultiID>&,
	    			      TypeSet<BinID>&,
				      TypeSet<Interval<float> >&,
				      const BinIDRange* br=0) const;

    bool		storeSurface(const MultiID&);
    void		setDataVal(const MultiID&,
	    			   ObjectSet< TypeSet<BinIDZValue> >&,
				   const char*);
    bool		getDataVal(const MultiID&,
	    			   ObjectSet< TypeSet<BinIDZValue> >&,
				   BufferString&);

    static const int	evDisplayHorizon;

			// Interaction stuff
    const MultiID&	selEMID() const			{ return selemid_; }

protected:

    MultiID&		selemid_;

    bool		ioHorizon(uiEMPartServer::ExternalType,bool);
    
};


#endif
