/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiseispartserv.h"

#include "ctxtioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "od_helpids.h"
#include "ptrman.h"
#include "seis2ddata.h"
#include "seis2dlineio.h"
#include "seisbuf.h"
#include "seisioobjinfo.h"
#include "seispreload.h"
#include "seisselection.h"
#include "seistrctr.h"
#include "survinfo.h"

#include "ui2dgeomman.h"
#include "uibatchtime2depthsetup.h"
#include "uigeninput.h"
#include "uigisexp.h"
#include "uigisexp2dlines.h"
#include "uimsg.h"
#include "uiseiscbvsimpfromothersurv.h"
#include "uiseisexpcubepositions.h"
#include "uiseisfileman.h"
#include "uiseisimportcbvs.h"
#include "uiseisiosimple.h"
#include "uiseislinesel.h"
#include "uiseismulticubeps.h"
#include "uiseispreloadmgr.h"
#include "uiseispsman.h"
#include "uiseisrandto2dline.h"
#include "uiseissel.h"
#include "uiseiswvltimpexp.h"
#include "uiseiswvltman.h"
#include "uisurvey.h"
#include "uitaskrunner.h"
#include "uitoolbutton.h"
#include "uivelocityvolumeconversion.h"


static int seis2dloadaction = 0;
static const char* sKeyPreLoad()	{ return "PreLoad"; }

uiSeisPartServer::uiSeisPartServer( uiApplService& a )
    : uiApplPartServer(a)
{
    SeisIOObjInfo::initDefault( sKey::Steering() );
    mAttachCB( IOM().surveyChanged, uiSeisPartServer::survChangedCB );
    mAttachCB( ui2DGeomManageDlg::instanceCreated(),
	       uiSeisPartServer::create2DLineGISExportBut );
}


uiSeisPartServer::~uiSeisPartServer()
{
    detachAllNotifiers();
    delete man2dseisdlg_;
    delete man3dseisdlg_;
    delete man2dprestkdlg_;
    delete man3dprestkdlg_;
    delete manwvltdlg_;
    delete impwvltdlg_;
    delete expwvltdlg_;
    delete impcbvsdlg_;
    delete impcbvsothsurvdlg_;
    delete imp3dseisdlg_;
    delete exp3dseisdlg_;
    delete imp2dseisdlg_;
    delete exp2dseisdlg_;
    delete impps3dseisdlg_;
    delete expps3dseisdlg_;
    delete impps2dseisdlg_;
    delete expps2dseisdlg_;
    delete expcubeposdlg_;
    delete gisexp2dlinesdlg_;
    delete t2ddlgs2d_;
    delete t2ddlgs3d_;
}


void uiSeisPartServer::survChangedCB( CallBacker* )
{
    closeAndNullPtr( man2dseisdlg_ );
    closeAndNullPtr( man3dseisdlg_ );
    closeAndNullPtr( man2dprestkdlg_ );
    closeAndNullPtr( man3dprestkdlg_ );
    closeAndNullPtr( manwvltdlg_ );
    closeAndNullPtr( impwvltdlg_ );
    closeAndNullPtr( expwvltdlg_ );

    closeAndNullPtr( impcbvsdlg_ );
    closeAndNullPtr( impcbvsothsurvdlg_ );
    closeAndNullPtr( imp3dseisdlg_ );
    closeAndNullPtr( exp3dseisdlg_ );
    closeAndNullPtr( imp2dseisdlg_ );
    closeAndNullPtr( exp2dseisdlg_ );
    closeAndNullPtr( impps3dseisdlg_ );
    closeAndNullPtr( expps3dseisdlg_ );
    closeAndNullPtr( impps2dseisdlg_ );
    closeAndNullPtr( expps2dseisdlg_ );
    closeAndNullPtr( expcubeposdlg_ );
    closeAndNullPtr( gisexp2dlinesdlg_ );

    closeAndNullPtr( t2ddlgs2d_ );
    closeAndNullPtr( t2ddlgs3d_ );
}


#define mPopupSimpIODlg(dlgobj,is2d,isps) { \
    if ( !dlgobj ) \
    { \
	const Seis::GeomType gt( Seis::geomTypeOf( is2d, isps ) ); \
	if ( !uiSurvey::survTypeOKForUser(Seis::is2D(gt)) ) return true; \
	dlgobj = new uiSeisIOSimple( parent(), gt, forread ); \
	dlgobj->setCaption( mkDlgCaption(forread,is2d,isps) ); \
    }\
    dlgobj->show(); }


uiString uiSeisPartServer::mkDlgCaption( bool forread, bool is2d, bool isps )
{
    const uiString tp = uiStrings::sSeismics( is2d, isps, 1 );
    return tr( "%1 %2 simple flat file")
	.arg( forread
	    ? uiStrings::phrImport( tp )
	    : uiStrings::phrExport( tp ) )
	.arg( forread ? tr("from") : tr("to" ) );
}


bool uiSeisPartServer::ioSeis( int opt, bool forread )
{
    switch ( opt )
    {
	case 0:
	    {
		if ( !impcbvsdlg_ )
		    impcbvsdlg_ = new uiSeisImportCBVS( parent() );

		impcbvsdlg_->show();
		break;
	    }
	case 9:
	    {
		if ( forread )
		{
		    if ( !impcbvsothsurvdlg_ )
			impcbvsothsurvdlg_ =
				new uiSeisImpCBVSFromOtherSurveyDlg( parent() );

		    impcbvsothsurvdlg_->show();
		}
		else
		    exportCubePos();
		break;
	    }
	case 5:
	    {
		if ( forread )
		    mPopupSimpIODlg(imp3dseisdlg_, false, false )
		else
		    mPopupSimpIODlg(exp3dseisdlg_, false, false )
		break;
	    }
	case 6:
	    {
		if ( forread )
		    mPopupSimpIODlg(imp2dseisdlg_, true, false )
		else
		    mPopupSimpIODlg(exp2dseisdlg_, true, false )
		break;
	    }
	case 7:
	    {
		if ( forread )
		    mPopupSimpIODlg(impps3dseisdlg_, false, true )
		else
		    mPopupSimpIODlg(expps3dseisdlg_, false, true )
		break;
	    }
	case 8:
	    {
		if ( forread )
		    mPopupSimpIODlg( impps2dseisdlg_, true, true )
		else
		    mPopupSimpIODlg( expps2dseisdlg_, true, true )
		break;
	    }
    }

    return true;
}


bool uiSeisPartServer::importSeis( int opt )
{ return ioSeis( opt, true ); }
bool uiSeisPartServer::exportSeis( int opt )
{ return ioSeis( opt, false ); }


MultiID uiSeisPartServer::getDefault2DDataID() const
{
    const BufferString key( IOPar::compKey(sKey::Default(),
		      SeisTrc2DTranslatorGroup::sKeyDefault()) );
    MultiID mid;
    SI().pars().get( key, mid );
    if ( !mid.isUdf() )
	return mid;

    const IOObjContext ctxt( SeisTrc2DTranslatorGroup::ioContext() );
    const IODir iodir ( ctxt.getSelKey() );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
    int nrod2d = 0;
    int def2didx = 0;
    int seisidx = -1;
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	SeisIOObjInfo seisinfo( ioobjs[idx] );
	if ( seisinfo.isOK() )
	{
	    nrod2d++;
	    def2didx = idx;
	    if ( ioobjs[idx]->name() == "Seis" )
		seisidx = idx;
	}
    }

    if ( nrod2d == 1 )
	return ioobjs[def2didx]->key();
    else if ( seisidx >= 0 )
	return ioobjs[seisidx]->key();

    uiString msg = uiStrings::phrCannotFind(tr("valid default 2D data.\n"
					       "Do you want to set it now?") );
    const bool tomanage = uiMSG().askGoOn( msg );
    if ( !tomanage )
	return MultiID::udf();

    uiSeisSel::Setup su( Seis::Line );
    su.seltxt( tr("Set default 2D seismic data") );
    su.enabotherdomain(false).allowsetsurvdefault( false );
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc2D);
    ctio->ctxt_.toselect_.requireType( sKey::Attribute(), true );
    uiSeisSelDlg dlg( parent(), *ctio, su );
    if ( !dlg.go() )
	return MultiID::udf();

    PtrMan<IOObj> seisobj = dlg.ioObj()->clone();
    seisobj->setSurveyDefault();
    return seisobj->key();
}


MultiID uiSeisPartServer::getDefaultDataID( bool is2d ) const
{
    if ( is2d )
	return getDefault2DDataID();

    const BufferString key( IOPar::compKey(sKey::Default(),
		      SeisTrcTranslatorGroup::sKeyDefault3D()) );
    MultiID mid;
    SI().pars().get( key, mid );
    if ( !mid.isUdf() )
	return mid;

    const IOObjContext ctxt( SeisTrcTranslatorGroup::ioContext() );
    const IODir iodir ( ctxt.getSelKey() );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();
    int nrod3d = 0;
    int def3didx = 0;
    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	SeisIOObjInfo seisinfo( ioobjs[idx] );
	if ( seisinfo.isOK() && !seisinfo.is2D() )
	{
	    nrod3d++;
	    def3didx = idx;
	}
    }

    if ( nrod3d == 1 )
	return ioobjs[def3didx]->key();

    uiString msg = uiStrings::phrCannotFind(
	tr("valid default volume.\nDo you want to set it now?") );
    const bool tomanage = uiMSG().askGoOn( msg );
    if ( !tomanage )
	return MultiID::udf();

    uiSeisSel::Setup su( Seis::Vol );
    su.seltxt( tr("Set default seismic data") );
    su.enabotherdomain(false).allowsetsurvdefault( false );
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc);
    ctio->ctxt_.toselect_.requireType( sKey::Attribute(), true );
    uiSeisSelDlg dlg( parent(), *ctio, su );
    if ( !dlg.go() )
	return MultiID::udf();

    PtrMan<IOObj> seisobj = dlg.ioObj()->clone();
    seisobj->setSurveyDefault();
    return seisobj->key();
}


#define mManageSeisDlg( dlgobj, dlgclss ) \
    delete dlgobj; \
    dlgobj = new dlgclss( parent(), is2d ); \
    dlgobj->setModal( modal ); \
    dlgobj->show();

void uiSeisPartServer::manageSeismics( int opt, bool modal )
{
    const bool is2d = opt == 1 || opt == 3;
    switch( opt )
    {
	case 0: mManageSeisDlg(man3dseisdlg_,uiSeisFileMan)
		break;
	case 1: mManageSeisDlg(man2dseisdlg_,uiSeisFileMan)
		break;
	case 2: mManageSeisDlg(man3dprestkdlg_,uiSeisPreStackMan)
		break;
	case 3: mManageSeisDlg(man2dprestkdlg_,uiSeisPreStackMan)
		break;
    }
}


void uiSeisPartServer::managePreLoad()
{
    uiSeisPreLoadMgr dlg( parent() );
    dlg.go();
}


void uiSeisPartServer::importWavelets()
{
    if ( !impwvltdlg_ )
    {
	impwvltdlg_ = new uiSeisWvltImp( parent() );
	impwvltdlg_->setModal( false );
    }

    impwvltdlg_->show();
}


void uiSeisPartServer::exportWavelets()
{
    if ( !expwvltdlg_ )
    {
	expwvltdlg_ = new uiSeisWvltExp( parent() );
	expwvltdlg_->setModal( false );
    }

    expwvltdlg_->show();
}


void uiSeisPartServer::manageWavelets()
{
    delete manwvltdlg_;
    manwvltdlg_ = new uiSeisWvltMan( parent() );
    manwvltdlg_->go();
}


void uiSeisPartServer::exportCubePos( const MultiID* key )
{
    if ( !expcubeposdlg_ )
	expcubeposdlg_ = new uiSeisExpCubePositionsDlg( parent() );

    if ( key )
	expcubeposdlg_->setInput( *key );

    expcubeposdlg_->show();
}


void uiSeisPartServer::create2DLineGISExportBut( CallBacker* cb )
{
    mDynamicCastGet(ui2DGeomManageDlg*,twodgm,cb)
	if ( !twodgm )
	    return;

    twodgm->addExtraButton( uiGISExpStdFld::strIcon(),
			    uiGISExpStdFld::sToolTipTxt(),
			    mCB(this,uiSeisPartServer,exportToGISCB) );
}


void uiSeisPartServer::exportToGISCB( CallBacker* )
{
    exportLinesToGIS( parent(), nullptr);
}


bool uiSeisPartServer::exportLinesToGIS( uiParent* p,
					 const TypeSet<Pos::GeomID>* geomids )
{
    if ( !uiGISExpStdFld::canDoExport(p) )
	return false;

    TypeSet<Pos::GeomID> usegeomids;
    if ( geomids )
	usegeomids = *geomids;

    if ( gisexp2dlinesdlg_ && gisexp2dlinesdlg_->parent() != p )
        closeAndNullPtr( gisexp2dlinesdlg_ );

    if ( gisexp2dlinesdlg_ )
	gisexp2dlinesdlg_->setSelected( usegeomids );
    else
    {
	gisexp2dlinesdlg_ = new uiGISExport2DLines( p, &usegeomids );
	gisexp2dlinesdlg_->setModal( false );
    }

    gisexp2dlinesdlg_->raise();
    gisexp2dlinesdlg_->show();
    return true;
}


bool uiSeisPartServer::select2DSeis( MultiID& mid )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc2D);
    uiSeisSel::Setup setup(Seis::Line);
    uiSeisSelDlg dlg( parent(), *ctio, setup );
    if ( !dlg.go() || !dlg.ioObj() ) return false;

    mid = dlg.ioObj()->key();
    return true;
}


#define mGet2DDataSet(retval) \
    PtrMan<IOObj> ioobj = IOM().get( mid ); \
    if ( !ioobj ) return retval; \
    Seis2DDataSet dataset( *ioobj );


bool uiSeisPartServer::select2DLines( TypeSet<Pos::GeomID>& selids,
				      int& action )
{
    selids.erase();

    uiDialog::Setup dsu(tr("Select 2D Lines"),
			mODHelpKey(mSeisPartServerselect2DLinesHelpID));
    uiDialog dlg( parent(), dsu );
    MouseCursorChanger cursorchgr( MouseCursor::Wait );
    auto* lchfld = new uiSeis2DLineChoose( &dlg, OD::ChooseAtLeastOne );
    BufferStringSet options;
    options.add( "Display projection lines only" )
	   .add( "Load default data" )
	   .add( "Select attribute" )
	   .add( "Color blended" );
    auto* optfld =
	new uiGenInput( &dlg, tr("On OK"), StringListInpSpec(options) );
    optfld->setValue( seis2dloadaction );
    optfld->attach( alignedBelow, lchfld );
    cursorchgr.restore();
    if ( !dlg.go() )
	return false;

    seis2dloadaction = action = optfld->getIntValue();
    lchfld->getChosen( selids );
    return selids.size();
}


void uiSeisPartServer::get2DStoredAttribs( const Pos::GeomID& geomid,
					   BufferStringSet& datasets,
					   int steerpol )
{
    SeisIOObjInfo::Opts2D o2d; o2d.steerpol_ = steerpol;
    SeisIOObjInfo::getDataSetNamesForLine( geomid, datasets, o2d );
}


bool uiSeisPartServer::create2DOutput( const MultiID& mid,
				       const Pos::GeomID& geomid,
				       TrcKeyZSampling& cs, SeisTrcBuf& buf )
{
    mGet2DDataSet(false)

    StepInterval<int> trcrg;
    dataset.getRanges( geomid, trcrg, cs.zsamp_ );
    cs.hsamp_.setCrlRange( trcrg );
    PtrMan<Executor> exec = dataset.lineFetcher( geomid, buf );
    uiTaskRunner dlg( parent() );
    return TaskRunner::execute( &dlg, *exec );
}


void uiSeisPartServer::getStoredGathersList( bool for3d,
					     BufferStringSet& nms ) const
{
    const IODir iodir( IOObjContext::getStdDirData(IOObjContext::Seis)->id_ );
    const ObjectSet<IOObj>& ioobjs = iodir.getObjs();

    for ( int idx=0; idx<ioobjs.size(); idx++ )
    {
	const IOObj& ioobj = *ioobjs[idx];
	if ( SeisTrcTranslator::isPS(ioobj)
	  && SeisTrcTranslator::is2D(ioobj) != for3d )
	    nms.add( (const char*)ioobj.name() );
    }

    nms.sort();
}


void uiSeisPartServer::storeRlnAs2DLine( const Geometry::RandomLine& rln ) const
{
    uiSeisRandTo2DLineDlg dlg( parent(), &rln );
    dlg.go();
}


void uiSeisPartServer::processTime2Depth( bool is2d ) const
{
    uiBatchTime2DepthSetup* dlg = is2d ? t2ddlgs2d_
				       : t2ddlgs3d_;
    if ( !dlg )
    {
	dlg = new uiBatchTime2DepthSetup( parent(), is2d );
	dlg->setModal( false );

	uiSeisPartServer* myself = const_cast<uiSeisPartServer*>(this);
	if ( is2d )
        myself->t2ddlgs2d_ = dlg;
	else
        myself->t2ddlgs3d_ = dlg;
    }

    dlg->show();
}


void uiSeisPartServer::processVelConv() const
{
    Vel::uiBatchVolumeConversion dlg( parent() );
    dlg.go();
}


void uiSeisPartServer::createMultiCubeDataStore() const
{
    uiSeisMultiCubePS dlg( parent(), MultiID::udf() );
    dlg.go();
}


void uiSeisPartServer::get2DZdomainAttribs( const Pos::GeomID& geomid,
			const char* zdomainstr, BufferStringSet& attribs )
{
    SeisIOObjInfo::Opts2D o2d;
    o2d.zdomky_ = zdomainstr;
    SeisIOObjInfo::getDataSetNamesForLine( geomid, attribs, o2d );
}


void uiSeisPartServer::fillPar( IOPar& par ) const
{
    const ObjectSet<Seis::PreLoadDataEntry>& pls = Seis::PLDM().getEntries();
    for ( int idx=0; idx<pls.size(); idx++ )
    {
	IOPar iop;
	Seis::PreLoader spl( pls[idx]->mid_, pls[idx]->geomid_ );
	spl.fillPar( iop );
	const BufferString parkey = IOPar::compKey( sKeyPreLoad(), idx );
	par.mergeComp( iop, parkey );
    }
}


bool uiSeisPartServer::usePar( const IOPar& par )
{
    PtrMan<IOPar> plpar = par.subselect( sKeyPreLoad() );
    if ( !plpar ) return true;

    IOPar newpar;
    newpar.mergeComp( *plpar, "Seis" );

    uiTaskRunner uitr( parent() );
    Seis::PreLoader::load( newpar, &uitr );
    return true;
}
