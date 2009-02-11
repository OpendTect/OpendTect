#ifndef uiempartserv_h
#define uiempartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Sep 2002
 RCS:           $Id: uiempartserv.h,v 1.82 2009-02-11 10:39:03 cvsranojay Exp $
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
class uiHorizonShiftDialog;

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
    bool		importFault();
    bool		exportFault(const char* type);

    void		showHorShiftDlg(uiParent*,const EM::ObjectID&,
	    				const BufferStringSet&,
					const TypeSet<int>&);
    MultiID		getStorageID(const EM::ObjectID&) const;
    EM::ObjectID	getObjectID(const MultiID&) const;

    BufferString	getName(const EM::ObjectID&) const;
    const char*		getType(const EM::ObjectID&) const;
    float		getShift() const;

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
    void		askUserToSave(const EM::ObjectID&) const;
    			/*!< If object has changed, user is asked whether
			    to save it or not, and if so, the object is saved */

    void		selectHorizons(TypeSet<EM::ObjectID>&,bool is2d);
    void		selectFaults(TypeSet<EM::ObjectID>&,bool is2d);
    void		selectFaultStickSets(TypeSet<EM::ObjectID>&);
    void		selectBodies(TypeSet<EM::ObjectID>&);
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
    void		getDataPointSet(const EM::ObjectID&,
	    				const EM::SectionID&, DataPointSet&,
					float shift); 
    void		fillHorShiftDPS(ObjectSet<DataPointSet>&);
    BinIDValueSet*	interpolateAuxData(const EM::ObjectID&,const char* nm);
    BinIDValueSet*	filterAuxData(const EM::ObjectID&,const char* nm);

    const char*		genRandLine(int opt);
    bool 		dispLineOnCreation()	{ return disponcreation_; }

    void		removeUndo();

    static const int	evDisplayHorizon();
    static const int	evRemoveTreeObject();
    static const int	evSyncGeometry();
    static const int	evCalcShiftAttribute();
    static const int	evHorizonShift();
    static const int	evStoreShiftHorizons();
    static const int	evShiftDlgOpened();
    static const int	evShiftDlgClosed();

			// Interaction stuff
    const EM::ObjectID&	selEMID() const			{ return selemid_; }
    EM::EMObject*	selEMObject();
    void		setAttribIdx( int idx )		{ attribidx_ = idx; }
    int			attribIdx() const		{ return attribidx_; }
    					    //Works only in case of Shift Dlg
    int			textureIdx() const; //Works only in case of Shift Dlg
    const StepInterval<float>& shiftRange() const	{ return shiftrg_; }
    void		setShiftRange( const StepInterval<float>& rg )
    							{ shiftrg_ = rg; }
    const char*		getAttribBaseNm() const		
    			{ return shiftattrbasename_.buf(); }

    void		removeTreeObject(const EM::ObjectID&);  

protected:

    void		selectSurfaces(TypeSet<EM::ObjectID>&,const char* type);
    bool		loadAuxData(const EM::ObjectID&,const TypeSet<int>&);
    void		syncGeometry(CallBacker*);
    BinIDValueSet*	changeAuxData(const EM::ObjectID&,const char* nm,
	    			      bool interp);

    void		calcDPS(CallBacker*);
    void		horShifted(CallBacker*);
    void		shiftDlgClosed(CallBacker*);
    EM::ObjectID	selemid_;
    EM::EMManager&	em_;
    uiHorizonShiftDialog* horshiftdlg_;
    
    bool		disponcreation_;
    StepInterval<float> shiftrg_;
    int			shiftidx_;
    int			attribidx_;
    BufferString	shiftattrbasename_;
};


/*!\mainpage Earth Model User Interface

 The earth model objects are visualised by the visXX classes. The I/O,
 parameters and other data-related issues also require standard user interface
 elements. The classes in this module provide these services via the
 uiEMPartServer interface.

*/

#endif
