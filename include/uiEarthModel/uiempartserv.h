#ifndef uiempartserv_h
#define uiempartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Sep 2002
 RCS:           $Id: uiempartserv.h,v 1.87 2009-05-22 01:04:15 cvskris Exp $
________________________________________________________________________

-*/

#include "emposid.h"
#include "multiid.h"
#include "uiapplserv.h"
#include "position.h"


class BinID;
class HorSampling;
class BinIDValueSet;
class DataPointSet;
class BufferStringSet;
class MultiID;
class SurfaceInfo;
class uiPopupMenu;

namespace Pick { class Set; }
namespace PosInfo { class Line2DData; }

template <class T> class Interval;

namespace EM { class EMObject; class EMManager; class SurfaceIODataSelection; };

/*! \brief Earth Model UI Part Server */

mClass uiEMPartServer : public uiApplPartServer
{
public:
			uiEMPartServer(uiApplService&);
			~uiEMPartServer();

    const char*		name() const			{ return "EarthModel"; }

			// Services
    bool		import3DHorizon(bool isgeom);
    bool		export3DHorizon();
    bool		export2DHorizon();
    bool		importFault(const char* type);
    bool		exportFault(const char* type);

    MultiID		getStorageID(const EM::ObjectID&) const;
    EM::ObjectID	getObjectID(const MultiID&) const;

    BufferString	getName(const EM::ObjectID&) const;
    const char*		getType(const EM::ObjectID&) const;

    int			nrAttributes(const EM::ObjectID&) const;
    bool		isGeometryChanged(const EM::ObjectID&) const;
    bool		isChanged(const EM::ObjectID&) const;
    bool		isEmpty(const EM::ObjectID&) const;
    bool		isFullResolution(const EM::ObjectID&) const;
    bool		isFullyLoaded(const EM::ObjectID&) const;
    bool		isShifted(const EM::ObjectID&) const;

    void		fillHoles(const EM::ObjectID&);
    void		filterSurface(const EM::ObjectID&);
    void		fillPickSet(Pick::Set&,MultiID);
    void		deriveHor3DFrom2D(const EM::ObjectID&);
    bool		askUserToSave(const EM::ObjectID&) const;
    			/*!< If object has changed, user is asked whether
			    to save it or not, and if so, the object is saved.
			    Returns false when save option is cancelled. */

    void		selectHorizons(ObjectSet<EM::EMObject>&,bool is2d);
    			//!<Returned set is reffed and must be unrefed by caller
    void		selectFaults(ObjectSet<EM::EMObject>&,bool is2d);
    			//!<Returned set is reffed and must be unrefed by caller
    void		selectFaultStickSets(ObjectSet<EM::EMObject>&);
    			//!<Returned set is reffed and must be unrefed by caller
    void		selectBodies(ObjectSet<EM::EMObject>&);
    			//!<Returned set is reffed and must be unrefed by caller
    bool		showLoadAuxDataDlg(const EM::ObjectID&);
    int			loadAuxData(const EM::ObjectID&,const char*);
    			/*!<Loads the specified data into memory and returns
			    its auxdatanr. */

    void		manageSurfaces(const char* typ);
    bool		loadSurface(const MultiID&,
	    			    const EM::SurfaceIODataSelection* s=0);
    void		getSurfaceInfo(ObjectSet<SurfaceInfo>&);
    static void         getAllSurfaceInfo(ObjectSet<SurfaceInfo>&,bool);
    void		getSurfaceDef3D(const TypeSet<EM::ObjectID>&,
	    			        BinIDValueSet&,
				        const HorSampling&) const;
    void		getSurfaceDef2D(const ObjectSet<MultiID>&,
	    				const ObjectSet<PosInfo::Line2DData>&,
	    				BufferStringSet&,TypeSet<Coord>&,
					TypeSet< Interval<float> >&);

    bool		storeObject(const EM::ObjectID&,
	    			    bool storeas=false) const;
    bool		storeObject(const EM::ObjectID&,bool storeas,
				    MultiID& storagekey) const;
    bool		storeAuxData(const EM::ObjectID&,
	    			     BufferString& auxdataname,
	    			     bool storeas=false) const;
    int			setAuxData(const EM::ObjectID&,
	    			   DataPointSet&,const char* nm,int valnr,
				   float shift);
    bool		getAuxData(const EM::ObjectID&,int auxdatanr,
	    			   DataPointSet&) const;
    bool		getAllAuxData(const EM::ObjectID&,DataPointSet&,
	    			      TypeSet<float>* shfs=0) const;
    BinIDValueSet*	interpolateAuxData(const EM::ObjectID&,const char* nm);
    BinIDValueSet*	filterAuxData(const EM::ObjectID&,const char* nm);

    const char*		genRandLine(int opt);
    bool 		dispLineOnCreation()	{ return disponcreation_; }

    void		removeUndo();

    static const int	evDisplayHorizon();
    static const int	evRemoveTreeObject();
    static const int	evSyncGeometry();

			// Interaction stuff
    const EM::ObjectID&	selEMID() const			{ return selemid_; }
    EM::EMObject*	selEMObject();

    const EM::ObjectID	saveUnsavedEMObject();
    void		removeUnsavedEMObjectFromTree();
    void		removeTreeObject(const EM::ObjectID&);  

protected:

    void		selectSurfaces(ObjectSet<EM::EMObject>&,
	    			       const char* type);
    bool		loadAuxData(const EM::ObjectID&,const TypeSet<int>&);
    void		syncGeometry(CallBacker*);
    BinIDValueSet*	changeAuxData(const EM::ObjectID&,const char* nm,
	    			      bool interp);

    EM::ObjectID	selemid_;
    EM::EMManager&	em_;
    
    bool		disponcreation_;
};


/*!\mainpage Earth Model User Interface

 The earth model objects are visualised by the visXX classes. The I/O,
 parameters and other data-related issues also require standard user interface
 elements. The classes in this module provide these services via the
 uiEMPartServer interface.

*/

#endif
