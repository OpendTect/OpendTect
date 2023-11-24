#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"

#include "emioobjinfo.h"
#include "emposid.h"
#include "trckeysampling.h"
#include "multiid.h"

#include "uiapplserv.h"
#include "uistring.h"


class BinIDValueSet;
class BufferStringSet;
class TrcKeyZSampling;
class DataPointSet;
class TrcKeySampling;
class SurfaceInfo;
class ZAxisTransform;
class uiBulkHorizonImport;
class uiBulk2DHorizonImport;
class uiBulkFaultImport;
class uiCreateHorizon;
class uiExportFault;
class uiExportHorizon;
class uiExport2DHorizon;
class uiImportFault3D;
class uiImportFaultStickSet2D;
class uiImportHorizon;
class uiImpHorFromZMap;
class uiSurfaceMan;
class uiVariogramDisplay;

namespace EM { class EMObject; class EMManager; class SurfaceIODataSelection; }
namespace Pick { class Set; }
namespace PosInfo { class Line2DData; }
namespace ZDomain { class Info; }

template <class T> class Array2D;


/*!
\brief Earth Model UI Part Server
*/

mExpClass(uiEarthModel) uiEMPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiEMPartServer);
public:
			uiEMPartServer(uiApplService&);
			~uiEMPartServer();

    const char*		name() const override		{ return "EarthModel"; }

			// Services
    bool		import3DHorGeom(bool bulk=false);
    bool		import3DHorAttr();
    bool		importHorFromZMap();
    bool		export3DHorizon(bool bulk=false);
    bool		export2DHorizon(bool bulk=false);
    bool		importFault();
    bool		importBulkFaults();
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
    void		computeIsochron();
    void		processTime2Depth(EM::IOObjInfo::ObjectType);

    MultiID		getStorageID(const EM::ObjectID&) const;
    EM::ObjectID	getObjectID(const MultiID&) const;

    BufferString	getName(const EM::ObjectID&) const;
    const char*		getType(const EM::ObjectID&) const;
    uiString		getUiName(const EM::ObjectID&) const;
    uiString		getUiType(const EM::ObjectID&) const;

    int			nrAttributes(const EM::ObjectID&) const;
    bool		isGeometryChanged(const EM::ObjectID&) const;
    bool		isChanged(const EM::ObjectID&) const;
    bool		isEmpty(const EM::ObjectID&) const;
    bool		isFullResolution(const EM::ObjectID&) const;
    bool		isFullyLoaded(const EM::ObjectID&) const;

    void		displayEMObject(const MultiID&);
    bool		fillHoles(const EM::ObjectID&,bool);
			/*!<return bool is overwrite old horizon or not. */
    bool		filterSurface(const EM::ObjectID&);
			/*!<return bool is overwrite old horizon or not. */
    void		fillPickSet(Pick::Set&,const MultiID&) const;
    void		deriveHor3DFrom2D(const EM::ObjectID&);
    bool		askUserToSave(const EM::ObjectID&,bool withcancl) const;
			/*!< If object has changed, user is asked whether
			    to save it or not, and if so, the object is saved.
			    Returns false when save option is cancelled. */

    TrcKeySampling	horizon3DDisplayRange() const
				{ return selectedrg_; }
    void		setHorizon3DDisplayRange(const TrcKeySampling&);
			/*!<Users can change the display range, hor 3D only. */

    void		selectHorizons(ObjectSet<EM::EMObject>&,bool is2d,
					uiParent* =nullptr,
					const ZDomain::Info* =nullptr);
			//!<Returned set is reffed and must be unrefed by caller
    void		selectFaults(ObjectSet<EM::EMObject>&,bool is2d,
					uiParent* =nullptr,
					const ZDomain::Info* =nullptr);
			//!<Returned set is reffed and must be unrefed by caller
    void		selectFaultStickSets(ObjectSet<EM::EMObject>&,
					uiParent* =nullptr,
					const ZDomain::Info* =nullptr);
			//!<Returned set is reffed and must be unrefed by caller
    void		selectFaultSets(ObjectSet<EM::EMObject>&,
					uiParent* =nullptr,
					const ZDomain::Info* =nullptr);
			//!<Returned set is reffed and must be unrefed by caller
    void		selectBodies(ObjectSet<EM::EMObject>&,
					uiParent* =nullptr);
			//!<Returned set is reffed and must be unrefed by caller
    bool		showLoadAuxDataDlg(const EM::ObjectID&);
    int			loadAuxData(const EM::ObjectID&,const char*,
				    bool removeold=true);
			/*!<Loads the specified data into memory and returns
			    its auxdatanr. */
    bool		loadAuxData(const EM::ObjectID&,const BufferStringSet&,
				    bool removeold=true);

    bool		showLoadFaultAuxDataDlg(const EM::ObjectID&);
    bool		storeFaultAuxData(const EM::ObjectID& id,
					  BufferString& auxdatanm,
					  const Array2D<float>& data);
    void		manageSurfaces(const char* typ);
    void		manage2DHorizons();
    void		manage3DHorizons();
    void		manageFaultStickSets();
    void		manage3DFaults();
    void		manageFaultSets();
    void		manageBodies();
    bool		loadSurface(const MultiID&,
			    const EM::SurfaceIODataSelection* s=nullptr) const;
    void		getSurfaceInfo(ObjectSet<SurfaceInfo>&) const;
    static void		getAllSurfaceInfo(ObjectSet<SurfaceInfo>&,bool);
    void		getSurfaceDef3D(const TypeSet<EM::ObjectID>&,
				        BinIDValueSet&,
				        const TrcKeySampling&) const;
    void		getSurfaceDef2D(const ObjectSet<MultiID>&,
					const BufferStringSet& sellines,
					TypeSet<Coord>&,
					TypeSet<BinID>&,
					TypeSet< Interval<float> >&) const;
    mDeprecatedDef void getSurfaceDef2D(const ObjectSet<MultiID>&,
					const BufferStringSet& sellines,
					TypeSet<Coord>&,
					TypeSet< Interval<float> >&) const;

    bool		storeObject(const EM::ObjectID&,
				    bool storeas=false) const;
    bool		storeObject(const EM::ObjectID&,bool storeas,
				    MultiID& storagekey,
				    float shift=0) const;
    bool		storeAuxData(const EM::ObjectID&,
				     BufferString& auxdataname,
				     bool storeas=false) const;
    int			setAuxData(const EM::ObjectID&,
				   DataPointSet&,const char* nm,int valnr,
				   float shift);
    bool		getAuxData(const EM::ObjectID&,int auxdatanr,
				   DataPointSet&, float& shift) const;
    bool		getAllAuxData(const EM::ObjectID&,DataPointSet&,
				      TypeSet<float>* shfs=0,
				      const TrcKeyZSampling* cs=0) const;
    bool		interpolateAuxData(const EM::ObjectID&,const char* nm,
					   DataPointSet& res);
    bool		filterAuxData(const EM::ObjectID&,const char* nm,
				      DataPointSet& res);
    bool		computeVariogramAuxData(const EM::ObjectID&,const char*,
						DataPointSet&);
    bool		attr2Geom(const EM::ObjectID&,const char* nm,
				  const DataPointSet&);
    bool		geom2Attr(const EM::ObjectID&);
    ZAxisTransform*	getHorizonZAxisTransform(bool is2d) const;

    MultiID		genRandLine(int opt);
    bool		dispLineOnCreation() const   { return disponcreation_; }

    void		removeUndo();

    static int		evDisplayHorizon();
    static int		evRemoveTreeObject();

			// Interaction stuff
    const EM::ObjectID&	selEMID() const			{ return selemid_; }
    EM::EMObject*	selEMObject();

    void		removeTreeObject(const EM::ObjectID&);

    void		managePreLoad();
    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);

protected:
    void		cleanup();

    void		selectSurfaces(uiParent*,ObjectSet<EM::EMObject>&,
					const char* type,const ZDomain::Info*);
    bool		loadAuxData(const EM::ObjectID&,const TypeSet<int>&,
				    bool removeold=true);
    bool		changeAuxData(const EM::ObjectID&,const char* nm,
				      bool interp,DataPointSet& res);
    void		importReadyCB(CallBacker*);
    void		survChangedCB(CallBacker*);

    EM::ObjectID	selemid_;
    uiImportHorizon*	imphorattrdlg_		= nullptr;
    uiImportHorizon*	imphorgeomdlg_		= nullptr;
    uiBulkHorizonImport* impbulkhordlg_		= nullptr;
    uiImpHorFromZMap*	impzmapdlg_		= nullptr;
    uiBulkFaultImport*	impbulkfltdlg_		= nullptr;
    uiBulkFaultImport*	impfltsetdlg_		= nullptr;
    uiImportFault3D*	impfltdlg_		= nullptr;
    uiImportFault3D*	impfltstickdlg_		= nullptr;
    uiImportFaultStickSet2D*	impfss2ddlg_	= nullptr;
    uiExportHorizon*	exphordlg_		= nullptr;
    uiExport2DHorizon*	exp2dhordlg_		= nullptr;
    uiExportFault*	expfltdlg_		= nullptr;
    uiExportFault*	expfltstickdlg_		= nullptr;
    uiExportFault*	expfltsetdlg_		= nullptr;
    uiCreateHorizon*	crhordlg_		= nullptr;
    uiBulkFaultImport*	impbulkfssdlg_		= nullptr;
    uiBulk2DHorizonImport* impbulk2dhordlg_	= nullptr;

    TrcKeySampling	selectedrg_;
    bool		disponcreation_		= false;

    ObjectSet<uiVariogramDisplay>	variodlgs_;

    static const char*  sKeySectionID() { return "Section ID"; }
    uiSurfaceMan*	man2dhordlg_		= nullptr;
    uiSurfaceMan*	man3dhordlg_		= nullptr;
    uiSurfaceMan*	man3dfaultdlg_		= nullptr;
    uiSurfaceMan*	manfssdlg_		= nullptr;
    uiSurfaceMan*	manfltsetdlg_		= nullptr;
    uiSurfaceMan*	manbodydlg_		= nullptr;

    void		displayOnCreateCB(CallBacker*);
};
