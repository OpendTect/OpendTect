#ifndef uiempartserv_h
#define uiempartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Sep 2002
 RCS:           $Id: uiempartserv.h,v 1.35 2005-10-10 15:00:55 cvskris Exp $
________________________________________________________________________

-*/

#include "emposid.h"
#include "multiid.h"
#include "uiapplserv.h"


class BinID;
class BinIDRange;
class BinIDValueSet;
class BufferString;
class BufferStringSet;
class MultiID;
class SurfaceInfo;
class uiPopupMenu;
template <class T> class Interval;

namespace EM { class SurfaceIODataSelection; };

/*! \brief Earth Model UI Part Server */

class uiEMPartServer : public uiApplPartServer
{
public:
			uiEMPartServer(uiApplService&);
			~uiEMPartServer();

    const char*		name() const			{ return "EarthModel"; }

			// Services
    bool		importHorizon();
    bool		exportHorizon();

    MultiID		getStorageID( const EM::ObjectID& ) const;
    EM::ObjectID	getObjectID( const MultiID& ) const;

    BufferString	getName(const EM::ObjectID&) const;
    const char*		getType(const EM::ObjectID&) const;

    bool		isChanged(const EM::ObjectID&) const;
    bool		isFullResolution(const EM::ObjectID&) const;

    bool		selectHorizon(EM::ObjectID& id);
    bool		selectFault(EM::ObjectID& id);
    bool		showLoadAuxDataDlg(const EM::ObjectID&);
    bool		loadAuxData(const EM::ObjectID&,const char*);
    			/*!<Loads the specified data into mem. */

    bool		importLMKFault();

    void		manageSurfaces(bool hor=true);
    bool		loadSurface(const MultiID&,
	    			    const EM::SurfaceIODataSelection* s=0);
    void		getSurfaceInfo(ObjectSet<SurfaceInfo>&);
    void		getSurfaceDef(const TypeSet<EM::ObjectID>&,
	    			      BinIDValueSet&,
				      const BinIDRange* br=0) const;

    bool		storeObject(const EM::ObjectID&,bool storeas=false);
    bool		storeAuxData(const EM::ObjectID&,bool storeas=false);
    void		setAuxData(const EM::ObjectID&,
	    			   ObjectSet<BinIDValueSet>&,const char*);
    void		setAuxData(const EM::ObjectID&,
	    			   ObjectSet<BinIDValueSet>&,
				   const BufferStringSet&);

    void		removeHistory();

    static const int	evDisplayHorizon;

			// Interaction stuff
    const EM::ObjectID&	selEMID() const			{ return selemid_; }

protected:
    bool		selectSurface(EM::ObjectID&,bool);
    bool		loadAuxData(const EM::ObjectID&, const TypeSet<int>& );

    EM::ObjectID	selemid_;

    bool		ioHorizon(bool);
    
};


/*!\mainpage Earth Model User Interface

 The earth model objects are visualised by the visXX classes. The I/O,
 parameters and other data-related issues also require standard user interface
 elements. The classes in this module provide these services via the
 uiEMPartServer interface.

*/

#endif
