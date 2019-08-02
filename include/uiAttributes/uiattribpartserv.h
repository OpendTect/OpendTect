#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2002
________________________________________________________________________

-*/

#include "uiattributesmod.h"
#include "uiapplserv.h"

#include "attribsel.h"
#include "datapack.h"
#include "menuhandler.h"
#include "survgeom.h"
#include "timer.h"

namespace Attrib
{
    class Data2DHolder;
    class Desc;
    class DescSet;
    class SelSpec;
    class SelSpecList;
    class EngineMan;
}

class BinnedValueSet;
class TrcKeyZSampling;
class DataPointSetDisplayMgr;
class IOObj;
class NLAModel;
class DataPointSet;
class RegularSeisDataPack;
class SeisTrcBuf;
class TaskRunner;
class uiAttribDescSetEd;
class uiAttribCrossPlot;
class uiAttrSetMan;
class uiAttrVolOut;
class uiImpAttrSet;
namespace ColTab { class Mapper; }
namespace Pick { class Set; }
namespace ZDomain { class Info; class Def; }

/*!\brief Service provider for application level - Attributes */

mExpClass(uiAttributes) uiAttribPartServer : public uiApplPartServer
{ mODTextTranslationClass(uiAttribPartServer);
public:

    typedef Attrib::Desc	Desc;
    typedef Attrib::DescSet	DescSet;
    typedef Attrib::DescID	DescID;
    typedef Attrib::SelSpec	SelSpec;
    typedef Attrib::SelSpecList	SelSpecList;

			uiAttribPartServer(uiApplService&);
			~uiAttribPartServer();

    const char*		name() const			{ return "Attributes"; }

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

    void		manageAttribSets(bool is2d);
    const DescSet&	curDescSet(bool is2d) const;
    DescSet&		curDescSet4Edit(bool is2d) const;
    void		getDirectShowAttrSpec(SelSpec&) const;
    bool		setSaved(bool is2d) const;
    void		saveSet(bool is2d);
    bool		editSet(bool is2d);
			    //!< returns whether new DescSet has been created
    bool		attrSetEditorActive() const	{ return attrsetdlg_; }
    void		updateSelSpec(SelSpec&) const;
    void		setAttrsNeedUpdt()		{ attrsneedupdt_ =true;}

    bool		selectAttrib(SelSpec&,const ZDomain::Info*,
				     Pos::GeomID geomid,
				     const uiString& seltxt=tr("View Data")) ;
    bool		selectRGBAttribs(SelSpecList&,
					 const ZDomain::Info*,Pos::GeomID);
    bool		setPickSetDirs(Pick::Set&,const NLAModel*,float vel);
    void		outputVol(const DBKey&,bool is2d,bool multioutput);
    bool		replaceSet(const IOPar&,bool is2d);
    bool		addToDescSet(const char*,bool is2d);
    int			getSliceIdx() const		{ return sliceidx_; }
    void		getPossibleOutputs(bool is2d,BufferStringSet&) const;

    void		setTargetSelSpec(const SelSpec&);
    void		setTargetSelSpecs(const SelSpecList& specs)
			{ targetspecs_ = specs; }
    const SelSpecList&	getTargetSelSpecs() const
			{ return targetspecs_; }
    const Desc*		getTargetDesc() const;

    DataPack::ID	createOutput(const TrcKeyZSampling&,DataPack::ID);
    RefMan<RegularSeisDataPack> createOutput(const TrcKeyZSampling&,
					 const RegularSeisDataPack* prevslcs=0);
    bool		createOutput(DataPointSet&,int firstcol =0);
    bool		createOutput(ObjectSet<DataPointSet>&,
				     int firstcol =0);
    bool		createOutput(const BinnedValueSet&,SeisTrcBuf&,
				     const TypeSet<BinID>&,
				     const TypeSet<BinID>&);
    DataPack::ID	createRdmTrcsOutput(const Interval<float>& zrg,
					    int rdlidx,
					    const TypeSet<BinID>* subpath=0 );
    static DataPack::ID createDataPackFor2D(const Attrib::Data2DHolder& input,
					    const ZDomain::Def& zdef,
					    const BufferStringSet* compnames=0);

    DescID		getDefaultAttribID(bool is2d) const;
    DescID		getStoredID(const DBKey&,bool is2d,
				    int selout=-1) const;
    IOObj*		getIOObj(const SelSpec&) const;

    bool		extractData(ObjectSet<DataPointSet>&);
    bool		createAttributeSet(const BufferStringSet&,DescSet*);
    void		importAttrSetFromFile();
    void		importAttrSetFromOtherSurvey();

    const NLAModel*	getNLAModel(bool) const;
    void		setNLAName( const char* nm )	{ nlaname_ = nm; }

    void		resetMenuItems();
    MenuItem*		storedAttribMenuItem(const SelSpec&,bool is2d,bool);
    MenuItem*		calcAttribMenuItem(const SelSpec&,bool is2d,bool);
    MenuItem*		nlaAttribMenuItem(const SelSpec&,bool is2d,bool);
    MenuItem*		zDomainAttribMenuItem(const SelSpec&,
					      const ZDomain::Info&,
					      bool is2d,bool);
    void		fillInStoredAttribMenuItem(MenuItem*,bool,bool,
						   const SelSpec&,bool,
						   bool needext=false);
    void		filter2DMenuItems(MenuItem&,const SelSpec&,
					  Pos::GeomID geomid, bool isstored,
					  int steerpol );

    bool		handleAttribSubMenu(int mnuid,SelSpec&,bool&);
    bool		handleMultiComp(const DBKey&,bool,bool,
					BufferStringSet&,DescID&,TypeSet<int>&);
    void		info2DAttribSubMenu(int mnuid,BufferString& attbnm,
					    bool& steering,bool& stored);
    bool		prepMultCompSpecs(TypeSet<int>,const DBKey&,
					  bool,bool);

    void		setEvaluateInfo(bool ae,bool as)
			{ alloweval_=ae; allowevalstor_=as; }

    void		fillPar(IOPar&,bool) const;
    void		usePar(const IOPar&,bool);

    void		setDPSDispMgr( DataPointSetDisplayMgr* dispmgr )
			{ dpsdispmgr_ = dispmgr; }

    void		set2DEvent( bool is2d )		{ is2devsent_ = is2d; }
    bool		is2DEvent()			{ return is2devsent_; }
    static const DescSet* getUserPrefDescSet(uiParent*);
				//!< For services that can work on 2D or 3D
				//!< Null return means user wants to cancel
    void		showXPlot(CallBacker*);

    void		setEvalBackupColTabMapper(const ColTab::Mapper*);
    const ColTab::Mapper* getEvalBackupColTabMapper() const;

    void		setSelAttr(const char* attrnm,bool isnewset=true);
    void		loadDefaultAttrSet(const char* attrsetnm);

protected:

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
    const Desc*		dirshwattrdesc_;
    uiAttribDescSetEd*	attrsetdlg_;
    Timer		attrsetclosetim_;
    bool                is2devsent_;
    bool		attrsneedupdt_;
    uiAttrSetMan*	manattribset2ddlg_;
    uiAttrSetMan*	manattribset3ddlg_;
    uiImpAttrSet*	impattrsetdlg_;
    uiAttrVolOut*	volattrdlg_;
    uiAttrVolOut*	multiattrdlg_;
    uiAttrVolOut*	dataattrdlg_;

    Attrib::EngineMan*	createEngMan(const TrcKeyZSampling* cs=0,
				     Pos::GeomID geomid=mUdfGeomID);

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

    DescID		targetID(bool is2d,int nr=0) const;

    void		insertNumerousItems(const BufferStringSet&,
					    const SelSpec&,bool,bool,bool);

    void		snapToValidRandomTraces(TypeSet<BinID>& path,
						const Desc*);

    static const char*	attridstr();
    BufferString	nlaname_;

    bool		alloweval_;
    bool		allowevalstor_;
    int			sliceidx_;
    SelSpecList&	targetspecs_;

    DataPointSetDisplayMgr*	dpsdispmgr_;

    const ColTab::Mapper*	evalmapperbackup_;

private:

    DataPack::ID		create2DOutput(const TrcKeyZSampling&,
					       TaskRunner&);
				//!< Use createOutput() instead.
};
