#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiapplserv.h"

#include "attribsel.h"
#include "datapack.h"
#include "integerid.h"
#include "menuhandler.h"
#include "survgeom.h"
#include "timer.h"

namespace Attrib
{
    class Data2DHolder;
    class Desc;
    class DescSet;
    class EngineMan;
    class SelSpec;
}

class BinIDValueSet;
class TrcKeyZSampling;
class DataPointSetDisplayMgr;
class IOObj;
class NLAModel;
class DataPointSet;
class RandomSeisDataPack;
class RegularSeisDataPack;
class SeisTrcBuf;
class TaskRunner;
class uiAttribDescSetEd;
class uiAttribCrossPlot;
class uiAttrSetMan;
class uiAttrVolOut;
class uiImpAttrSet;
namespace ColTab { class MapperSetup; }
namespace Pick { class Set; }
namespace ZDomain { class Def; class Info; }

/*!
\brief Service provider for application level - Attributes
*/

mExpClass(uiAttributes) uiAttribPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiAttribPartServer);
public:
			uiAttribPartServer(uiApplService&);
			~uiAttribPartServer();

    const char*		name() const override		{ return "Attributes"; }

    static int		evDirectShowAttr();
			//!< User requested direct redisplay of curAttrDesc()
    static int		evNewAttrSet();
			//!< FYI
    static int		evAttrSetDlgClosed();
			//!< AttributeSet window closes
    static int		evEvalAttrInit();
			//!< Initialization of evaluation dialog
    static int		evEvalCalcAttr();
			//!< User wants to evaluate current attribute
    static int		evEvalShowSlice();
			//!< Display slice
    static int		evEvalStoreSlices();
			//!< Store slices
    static int		evEvalRestore();
			//!< Update name in tree after evaluation dlg closed
			//!< And restore mapper
    static int		objNLAModel2D();
			//!< Request current 2D NLAModel* via getObject()
    static int		objNLAModel3D();
			//!< Request current 3D NLAModel* via getObject()
    static uiString	getMenuText(bool is2d,bool issteering, bool endmenu);

    void		manageAttribSets(bool is2d=false);
    const Attrib::DescSet* curDescSet(bool is2d) const;
    void		getDirectShowAttrSpec(Attrib::SelSpec&) const;
    bool		setSaved(bool is2d) const;
    void		saveSet(bool is2d);
    bool		editSet(bool is2d);
			//!< returns whether new AttribDescSet has been created
    bool		attrSetEditorActive() const	{ return attrsetdlg_; }
    void		updateSelSpec(Attrib::SelSpec&) const;
    void		setAttrsNeedUpdt()		{ attrsneedupdt_ =true;}

    bool		selectAttrib(Attrib::SelSpec&,const ZDomain::Info*,
				     const Pos::GeomID&,
				     const uiString& seltxt=tr("View Data")) ;
    bool		selectRGBAttribs(TypeSet<Attrib::SelSpec>&,
					 const ZDomain::Info*,
					 const Pos::GeomID&);
    bool		setPickSetDirs(Pick::Set&,const NLAModel*,float vel);
    void		outputVol(const MultiID&,bool is2d,bool multioutput);
    void		updateMultiIdFromNLA(uiAttrVolOut*,const MultiID&,
					bool is2d,const Attrib::DescSet*);
    void		updateNLAInput(const MultiID&,bool is2d);
    bool		replaceSet(const IOPar&,bool is2d);
    mDeprecated("Use with MultiID")
    bool		addToDescSet(const char*,bool is2d);
    bool		addToDescSet(const MultiID&,bool is2d);
    int			getSliceIdx() const		{ return sliceidx_; }
    void		getPossibleOutputs(bool is2d,BufferStringSet&) const;

    void		setTargetSelSpec(const Attrib::SelSpec&);
    void		setTargetSelSpecs(
				const TypeSet<Attrib::SelSpec>& specs )
			{ targetspecs_ = specs; }

    const TypeSet<Attrib::SelSpec>& getTargetSelSpecs() const
			    { return targetspecs_; }

    ConstRefMan<RegularSeisDataPack> createOutput(const TrcKeyZSampling&,
				    const RegularSeisDataPack* cache=nullptr);
    RefMan<RegularSeisDataPack> createOutputRM(const TrcKeyZSampling&,
				    const RegularSeisDataPack* cache=nullptr);
    ConstRefMan<RegularSeisDataPack> create2DOutputRM(const TrcKeyZSampling&,
				    const RegularSeisDataPack* cache=nullptr,
				    TaskRunner* = nullptr);

    bool		createOutput(DataPointSet&,int firstcol=0,
				     bool showprogress=true);
    bool		createOutput(ObjectSet<DataPointSet>&,
				     int firstcol =0);
    bool		createOutput(const BinIDValueSet&,SeisTrcBuf&,
				     const TrcKeySet& trueknotspos,
				     const TrcKeySet& snappedpos);
    bool		createOutput(const BinIDValueSet&,SeisTrcBuf&,
				     const TypeSet<BinID>& trueknotspos,
				     const TypeSet<BinID>& snappedpos);

    RefMan<RandomSeisDataPack>	createRdmTrcsOutputRM(
					    const Interval<float>& zrg,
					    const RandomLineID&);
    RefMan<RandomSeisDataPack>	createRdmTrcsOutputRM(
					    const Interval<float>& zrg,
					    TrcKeySet& path,
					    TrcKeySet& trueknotspos);
    static RefMan<RegularSeisDataPack> createDataPackFor2DRM(
					    const Attrib::Data2DHolder& input,
					    const TrcKeyZSampling& outputsamp,
					    const ZDomain::Info&,
				    const BufferStringSet* compnames=nullptr);

    Attrib::DescID	getStoredID(const MultiID&,bool is2d,
				    int selout=-1) const;
    IOObj*		getIOObj(const Attrib::SelSpec&) const;

    bool		extractData(ObjectSet<DataPointSet>&);
    bool		createAttributeSet(const BufferStringSet&,
					   Attrib::DescSet*);
    void		importAttrSetFromFile();
    void		importAttrSetFromOtherSurvey();

    const NLAModel*	getNLAModel(bool) const;
    void		setNLAName( const char* nm )	{ nlaname_ = nm; }

    void		resetMenuItems();
    MenuItem*		storedAttribMenuItem(const Attrib::SelSpec&,bool is2d,
					     bool);
    MenuItem*		calcAttribMenuItem(const Attrib::SelSpec&,bool is2d,
					   bool);
    MenuItem*		nlaAttribMenuItem(const Attrib::SelSpec&,bool is2d,
					  bool);
    MenuItem*		zDomainAttribMenuItem(const Attrib::SelSpec&,
					      const ZDomain::Info&,
					      bool is2d,bool);
    void		fillInStoredAttribMenuItem(MenuItem*,bool,bool,
						   const Attrib::SelSpec&,bool,
						   bool needext=false);

    bool		handleAttribSubMenu(int mnuid,Attrib::SelSpec&,bool&);
    bool		handleMultiComp(const MultiID&,bool,bool,
					BufferStringSet&,Attrib::DescID&,
					TypeSet<int>&);
    void		info2DAttribSubMenu(int mnuid,BufferString& attbnm,
					    bool& steering,bool& stored);
    bool		prepMultCompSpecs(const TypeSet<int>&,const MultiID&,
					  bool,bool);

    void		setEvaluateInfo(bool ae,bool as)
			{ alloweval_=ae; allowevalstor_=as; }

    void		fillPar(IOPar&,bool,bool) const;
    void		usePar(const IOPar&,bool,bool);
    void		usePar(const IOPar&,bool,bool, uiStringSet&);

    void		setDPSDispMgr( DataPointSetDisplayMgr* dispmgr )
			{ dpsdispmgr_ = dispmgr; }

    void		set2DEvent( bool is2d )		{ is2devsent_ = is2d; }
    bool		is2DEvent()			{ return is2devsent_; }
    int			use3DMode() const;
			//!< If you have services that can work on 2D or 3D
			//!< 0 = 2D, 1 = 3D, -1 = user cancel
    const Attrib::DescSet*	getUserPrefDescSet() const;
			//!< For services that can work on 2D or 3D
    void		showXPlot(CallBacker*);

    void		setEvalBackupColTabMapper(const ColTab::MapperSetup*);
    const ColTab::MapperSetup* getEvalBackupColTabMapper() const;

    void		setSelAttr(const char* attrnm);
    void		loadDefaultAttrSet(const char* attribsetnm);
    void		needSaveNLAps(CallBacker*);

    Notifier<uiAttribPartServer>	needSaveNLA;
    friend class	DPSOutputCreator;

protected:

    static const char*	sKeyUserSettingAttrErrMsg();

    MenuItem		stored2dmnuitem_;
    MenuItem		stored3dmnuitem_;
    MenuItem		calc2dmnuitem_;
    MenuItem		calc3dmnuitem_;
    MenuItem		nla2dmnuitem_;
    MenuItem		nla3dmnuitem_;
    MenuItem		zdomain2dmnuitem_;
    MenuItem		zdomain3dmnuitem_;
    MenuItem		steering2dmnuitem_;
    MenuItem		steering3dmnuitem_;
    MenuItem		multcomp2d_;
    MenuItem		multcomp3d_;

    ObjectSet<uiAttribCrossPlot> attrxplotset_;
    const Attrib::Desc*	dirshwattrdesc_			= nullptr;
    uiAttribDescSetEd*	attrsetdlg_			= nullptr;
    Timer		attrsetclosetimer_;
    bool		is2devsent_			= false;
    bool		attrsneedupdt_			= true;
    uiAttrSetMan*	manattribset2ddlg_		= nullptr;
    uiAttrSetMan*	manattribsetdlg_		= nullptr;
    uiImpAttrSet*	impattrsetdlg_			= nullptr;
    uiAttrVolOut*	volattrdlg_			= nullptr;
    uiAttrVolOut*	multiattrdlg_			= nullptr;
    uiAttrVolOut*	dataattrdlg_			= nullptr;

    Attrib::EngineMan*	createEngMan(const TrcKeyZSampling* cs=nullptr,
				     const Pos::GeomID& =Pos::GeomID::udf());

    void		directShowAttr(CallBacker*);

    void		showEvalDlg(CallBacker*);
    void		showCrossEvalDlg(CallBacker*);
    void		calcEvalAttrs(CallBacker*);
    void		showSliceCB(CallBacker*);
    void		evalDlgClosed(CallBacker*);
    void		xplotClosedCB(CallBacker*);
    void		processEvalDlg(bool iscrossevaluate);
    void		attrsetDlgApply(CallBacker*);

    void		attrsetDlgClosed(CallBacker*);
    void		attrsetDlgCloseTimTick(CallBacker*);
    void		survChangedCB(CallBacker*);

    Attrib::DescID	targetID(bool is2d,int nr=0) const;

    void		handleAutoSet();
    void		useAutoSet(bool);

    void		insertNumerousItems(const BufferStringSet&,
					    const Attrib::SelSpec&,
					    bool,bool,bool);

    static const char*	attridstr();
    BufferString	nlaname_;

    bool			alloweval_		= false;
    bool			allowevalstor_		= false;
    int				sliceidx_		= -1;
    Attrib::DescSet*		evalset			= nullptr;
    TypeSet<Attrib::SelSpec>	targetspecs_;

    DataPointSetDisplayMgr*	dpsdispmgr_		= nullptr;

    ColTab::MapperSetup*	evalmapperbackup_	= nullptr;

private:

    static uiAttribPartServer*	theinst_;

public:

    static uiAttribPartServer*	getInst();

    void		filter2DMenuItems(MenuItem&,const Attrib::SelSpec&,
					  const Pos::GeomID&,
					  bool isstored,int steerpol);
protected:
    void		snapToValidRandomTraces(TrcKeySet& path,
						const Attrib::Desc*);

public:

    mDeprecated("Use createOutputRM")
    DataPackID	createOutput(const TrcKeyZSampling&,const DataPackID&);

    mDeprecated("Use createOutputRM")
    DataPackID		create2DOutput(const TrcKeyZSampling&,
				       const Pos::GeomID&,TaskRunner&);
    mDeprecated("Use version returning RefMan<RegularSeisDataPack>")
    static DataPackID createDataPackFor2D(const Attrib::Data2DHolder& input,
					    const TrcKeyZSampling& outputsamp,
					    const ZDomain::Def& zdef,
				    const BufferStringSet* compnames=nullptr);

    mDeprecated("createRdmTrcsOutputRM")
    DataPackID		createRdmTrcsOutput(const Interval<float>& zrg,
					    const RandomLineID&);
    mDeprecated("createRdmTrcsOutputRM")
    DataPackID		createRdmTrcsOutput(const Interval<float>& zrg,
					    TrcKeySet& path,
					    TrcKeySet& trueknotspos);
    mDeprecated("Use TrcKeySet")
    DataPackID	createRdmTrcsOutput(const Interval<float>& zrg,
				    TypeSet<BinID>& path,
				    TypeSet<BinID>& trueknotspos);
};
