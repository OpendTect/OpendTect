#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2002
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "trckeysampling.h"
#include "dbkey.h"
#include "position.h"
#include "uiapplserv.h"
#include "uistring.h"

class BinnedValueSet;
class BufferStringSet;
class TrcKeyZSampling;
class DataPointSet;
class TrcKeySampling;
class SurfaceInfo;
class ZAxisTransform;
class uiBulkFaultImport;
class uiBulkHorizonImport;
class uiBulk2DHorizonImport;
class uiCreateHorizon;
class uiExportFault;
class uiExportHorizon;
class uiImportFault3D;
class uiImportFaultStickSet2D;
class uiImportHorizon;
class uiSurfaceMan;
class uiVariogramDisplay;

namespace EM { class Object; class ObjectManager; class SurfaceIODataSelection;}
namespace Pick { class Set; }
namespace PosInfo { class Line2DData; }

template <class T> class Array2D;


/*!\brief Earth Model UI Part Server */

mExpClass(uiEarthModel) uiEMPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiEMPartServer);
public:
			uiEMPartServer(uiApplService&);
			~uiEMPartServer();

    const char*		name() const			{ return "EarthModel"; }

			// Services
    bool		import3DHorGeom(bool bulk=false);
    bool		import3DHorAttr();
    bool		export3DHorizon(bool bulk=false);
    bool		export2DHorizon(bool bulk=false);
    bool		importFault(bool bulk);
    bool		importFaultStickSet();
    bool		importFaultSet();
    bool		importBulkFaultStickSet(bool is2d=false);
    void		import2DFaultStickset();
    void		importBulk2DFaultStickset();
    bool		importBulk2DHorizon();
    bool		exportFault(bool single=false);
    bool		exportFaultStickSet(bool single=false);
    bool		exportFaultSet();
    void		createHorWithConstZ(bool is2d);

    int			nrAttributes(const DBKey&) const;
    bool		isGeometryChanged(const DBKey&) const;
    bool		isChanged(const DBKey&) const;
    bool		isEmpty(const DBKey&) const;
    bool		isFullResolution(const DBKey&) const;
    bool		isFullyLoaded(const DBKey&) const;

    void		displayEMObject(const DBKey&);
    bool		fillHoles(const DBKey&,bool);
			/*!<return bool is overwrite old horizon or not. */
    bool		filterSurface(const DBKey&);
			/*!<return bool is overwrite old horizon or not. */
    void		fillPickSet(Pick::Set&,DBKey);
    void		deriveHor3DFrom2D(const DBKey&);
    bool		askUserToSave(const DBKey&,bool withcancl) const;
			/*!< If object has changed, user is asked whether
			    to save it or not, and if so, the object is saved.
			    Returns false when save option is cancelled. */

    TrcKeySampling	horizon3DDisplayRange() const
				{ return selectedrg_; }
    void		setHorizon3DDisplayRange(const TrcKeySampling&);
			/*!<Users can change the display range, hor 3D only. */

    void		selectHorizons(ObjectSet<EM::Object>&,bool is2d,
					uiParent* p=0);
			//!<Returned set is reffed and must be unrefed by caller
    void		selectFaults(ObjectSet<EM::Object>&,uiParent* p=0);
			//!<Returned set is reffed and must be unrefed by caller
    void		selectFaultStickSets(ObjectSet<EM::Object>&,
					uiParent* p=0);
			//!<Returned set is reffed and must be unrefed by caller
    void		selectBodies(ObjectSet<EM::Object>&,uiParent* p=0);
			//!<Returned set is reffed and must be unrefed by caller
    bool		showLoadAuxDataDlg(const DBKey&,uiParent* p=0);
    int			loadAuxData(const DBKey&,const char*,
				    bool removeold=true);
			/*!<Loads the specified data into memory and returns
			    its auxdatanr. */
    bool		loadAuxData(const DBKey&,const BufferStringSet&,
				    bool removeold=true);

    bool		showLoadFaultAuxDataDlg(const DBKey&);
    bool		storeFaultAuxData(const DBKey& id,
					  BufferString& auxdatanm,
					  const Array2D<float>& data);
    void		manageSurfaces(const char* typ);
    void		manage2DHorizons();
    void		manage3DHorizons();
    void		manageFaultStickSets();
    void		manage3DFaults();
    void		manageFaultSets();
    void		manageBodies();
    bool		loadSurface(const DBKey&,
				    const EM::SurfaceIODataSelection* s=0);
    void		getSurfaceInfo(ObjectSet<SurfaceInfo>&);
    static void         getAllSurfaceInfo(ObjectSet<SurfaceInfo>&,bool);
    void		getSurfaceDef3D(const DBKeySet&,BinnedValueSet&,
				        const TrcKeySampling&) const;
    void		getSurfaceDef2D(const DBKeySet&,
					const BufferStringSet& sellines,
					TypeSet<Coord>&,GeomIDSet&,
					TypeSet<Interval<float> >&);

    bool		storeObject(const DBKey&,
				    bool storeas=false) const;
    bool		storeObject(const DBKey&,bool storeas,
				    DBKey& storagekey,
				    float shift=0) const;
    bool		storeAuxData(const DBKey&,
				     BufferString& auxdataname,
				     bool storeas=false) const;
    int			setAuxData(const DBKey&,
				   DataPointSet&,const char* nm,int valnr,
				   float shift);
    bool		getAuxData(const DBKey&,int auxdatanr,
				   DataPointSet&, float& shift) const;
    bool		getAllAuxData(const DBKey&,DataPointSet&,
				      TypeSet<float>* shfs=0,
				      const TrcKeyZSampling* cs=0) const;
    bool		interpolateAuxData(const DBKey&,const char* nm,
					   DataPointSet& res);
    bool		filterAuxData(const DBKey&,const char* nm,
				      DataPointSet& res);
    bool		computeVariogramAuxData(const DBKey&,const char*,
						DataPointSet&);
    bool		attr2Geom(const DBKey&,const char* nm,
				  const DataPointSet&);
    bool		geom2Attr(const DBKey&);
    ZAxisTransform*	getHorizonZAxisTransform(bool is2d);

    DBKey		genRandLine(int opt);
    bool		dispLineOnCreation()	{ return disponcreation_; }

    void		removeUndo();

    static int		evDisplayHorizon();
    static int		evRemoveTreeObject();

			// Interaction stuff
    const DBKey&	selEMID() const			{ return selemid_; }
    EM::Object*		selEMObject();

    void		removeTreeObject(const DBKey&);

    void		managePreLoad();
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:

    bool		loadAuxData(const DBKey&,const TypeSet<int>&,
				    bool removeold=true);
    bool		changeAuxData(const DBKey&,const char* nm,
				      bool interp,DataPointSet& res);
    void		importReadyCB(CallBacker*);
    void		survChangedCB(CallBacker*);
    void		displayOnCreateCB(CallBacker*);

    DBKey		selemid_;
    EM::ObjectManager&	emmgr_;
    uiImportHorizon*	imphorattrdlg_;
    uiImportHorizon*	imphorgeomdlg_;
    uiBulkHorizonImport* impbulkhordlg_;
    uiBulk2DHorizonImport* impbulkhor2ddlg_;
    uiImportFault3D*	impfltdlg_;
    uiBulkFaultImport*	impbulkfltdlg_;
    uiImportFault3D*	impfltstickdlg_;
    uiImportFaultStickSet2D*	impfss2ddlg_;
    uiExportHorizon*	exphordlg_;
    uiExportHorizon*	expbulkhordlg_;
    uiExportFault*	expfltdlg_;
    uiExportFault*	expfltstickdlg_;
    uiExportFault*	expfaultsetdlg_;
    uiCreateHorizon*	crhordlg_;
    uiBulkFaultImport*	impbulkfssdlg_;

    TrcKeySampling	selectedrg_;
    bool		disponcreation_;

    ObjectSet<uiVariogramDisplay> variodlgs_;

    static const char*  sKeySectionID() { return "Section ID"; }
    uiSurfaceMan*	man2dhordlg_;
    uiSurfaceMan*	man3dhordlg_;
    uiSurfaceMan*	ma3dfaultdlg_;
    uiSurfaceMan*	manfssdlg_;
    uiSurfaceMan*	manfaultsetdlg_;
    uiSurfaceMan*	manbodydlg_;
};
