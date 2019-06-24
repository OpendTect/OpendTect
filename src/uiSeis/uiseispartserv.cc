/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2001
________________________________________________________________________

-*/

#include "uiseispartserv.h"

#include "arrayndimpl.h"
#include "ctxtioobj.h"
#include "dbdir.h"
#include "dbman.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "posinfo2d.h"
#include "posinfo2dsurv.h"
#include "ptrman.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seis2ddata.h"
#include "seis2dlineio.h"
#include "seisbuf.h"
#include "seisbufadapters.h"
#include "seisioobjinfo.h"
#include "seispreload.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "seisioobjinfo.h"
#include "survinfo.h"

#include "uibatchtime2depthsetup.h"
#include "uichecklist.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewmainwin.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimenu.h"
#include "uimergeseis.h"
#include "uimsg.h"
#include "uiseisimportodcube.h"
#include "uiseisimpcubefromothersurv.h"
#include "uiseisexpcubepositions.h"
#include "uiseisfileman.h"
#include "uiseisioobjinfo.h"
#include "uiseisiosimple.h"
#include "uiseismulticubeps.h"
#include "uiseispsman.h"
#include "uiseisrandto2dline.h"
#include "uiseissel.h"
#include "uiseislinesel.h"
#include "uiseiswvltimpexp.h"
#include "uiseiswvltman.h"
#include "uiseispreloadmgr.h"
#include "uiselsimple.h"
#include "uitaskrunner.h"
#include "uivelocityvolumeconversion.h"
#include "od_helpids.h"


static int seis2dloadaction = 0;
static const char* sKeyPreLoad()	{ return "PreLoad"; }

uiSeisPartServer::uiSeisPartServer( uiApplService& a )
    : uiApplPartServer(a)
    , man2dseisdlg_(0)
    , man3dseisdlg_(0)
    , man2dprestkdlg_(0)
    , man3dprestkdlg_(0)
    , manwvltdlg_(0)
	, impwvltdlg_(0)
	, expwvltdlg_(0)
    , impodcubedlg_(0)
    , impcubeothsurvdlg_(0)
    , imp3dseisdlg_(0)
    , exp3dseisdlg_(0)
    , imp2dseisdlg_(0)
    , exp2dseisdlg_(0)
    , impps3dseisdlg_(0)
    , expps3dseisdlg_(0)
    , impps2dseisdlg_(0)
    , expps2dseisdlg_(0)
    , expcubeposdlg_(0)
    , t2ddlg2d_(0)
    , t2ddlg3d_(0)
{
    SeisIOObjInfo::initDefault( sKey::Steering() );
    mAttachCB( DBM().surveyChanged, uiSeisPartServer::survChangedCB );
}


uiSeisPartServer::~uiSeisPartServer()
{
    detachAllNotifiers();
    delete man2dseisdlg_;
    delete man3dseisdlg_;
    delete man2dprestkdlg_;
    delete man3dprestkdlg_;
    delete manwvltdlg_;
    delete impodcubedlg_;
    delete impcubeothsurvdlg_;
    delete imp3dseisdlg_;
    delete exp3dseisdlg_;
    delete imp2dseisdlg_;
    delete exp2dseisdlg_;
    delete impps3dseisdlg_;
    delete expps3dseisdlg_;
    delete impps2dseisdlg_;
    delete expps2dseisdlg_;
    delete expcubeposdlg_;
	delete impwvltdlg_;
	delete expwvltdlg_;
    delete t2ddlg2d_;
    delete t2ddlg3d_;
}


void uiSeisPartServer::survChangedCB( CallBacker* )
{
    deleteAndZeroPtr( man2dseisdlg_ );
    deleteAndZeroPtr( man3dseisdlg_ );
    deleteAndZeroPtr( man2dprestkdlg_ );
    deleteAndZeroPtr( man3dprestkdlg_ );
    deleteAndZeroPtr( manwvltdlg_ );
    Seis::PLDM().removeAll();

    deleteAndZeroPtr( impodcubedlg_ );
    deleteAndZeroPtr( impcubeothsurvdlg_ );
    deleteAndZeroPtr( imp3dseisdlg_ );
    deleteAndZeroPtr( exp3dseisdlg_ );
    deleteAndZeroPtr( imp2dseisdlg_ );
    deleteAndZeroPtr( exp2dseisdlg_ );
    deleteAndZeroPtr( impps3dseisdlg_ );
    deleteAndZeroPtr( expps3dseisdlg_ );
    deleteAndZeroPtr( impps2dseisdlg_ );
    deleteAndZeroPtr( expps2dseisdlg_ );
    deleteAndZeroPtr( expcubeposdlg_ );
	deleteAndZeroPtr( impwvltdlg_ );
	deleteAndZeroPtr( expwvltdlg_ );
    deleteAndZeroPtr( t2ddlg2d_ );
    deleteAndZeroPtr( t2ddlg3d_ );
}

#define mPopupSimpIODlg(dlgobj,is2d,isps) { \
    if ( !dlgobj ) \
    { \
	const Seis::GeomType gt( Seis::geomTypeOf( is2d, isps ) ); \
	if ( !uiSurvey::userIsOKWithPossibleTypeChange(Seis::is2D(gt)) ) \
	    return true; \
	dlgobj = new uiSeisIOSimple( parent(), gt, forread ); \
	dlgobj->setCaption( mkSimpIODlgCaption(forread,is2d,isps) ); \
    }\
    dlgobj->show(); }


uiString uiSeisPartServer::mkSimpIODlgCaption( bool forread, bool is2d,
						bool isps )
{
    const uiString tp = uiStrings::sSeisObjName( is2d, !is2d, isps );

    return (forread ? tr("Import %1 from simple flat file")
		    : tr("Export %1 to simple flat file"))
	    .arg( uiStrings::sSeisObjName( is2d, !is2d, isps ) );
}


bool uiSeisPartServer::ioSeis( int opt, bool forread )
{
    switch ( opt )
    {
	case 0:
	    {
		if ( !impodcubedlg_ )
		    impodcubedlg_ = new uiSeisImportODCube( parent() );

		impodcubedlg_->show();
		break;
	    }
	case 9:
	    {
		if ( forread )
		{
		    if ( !impcubeothsurvdlg_ )
			impcubeothsurvdlg_ =
				new uiSeisImpCubeFromOtherSurveyDlg( parent() );

		    impcubeothsurvdlg_->show();
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


DBKey uiSeisPartServer::getDefault2DDataID() const
{
    BufferString key( IOPar::compKey(sKey::Default(),
		      SeisTrc2DTranslatorGroup::sKeyDefault()) );
    BufferString midstr( SI().getDefaultPars().find(key) );
    if ( !midstr.isEmpty() )
	return DBKey( midstr.buf() );

    const IOObjContext ctxt( mIOObjContext(SeisTrc2D) );
    int nrod2d = 0;
    DBKey def2dky; DBKey seisky;
    DBDirEntryList entrylist( ctxt );
    for ( int idx=0; idx<entrylist.size(); idx++ )
    {
	SeisIOObjInfo seisinfo( entrylist.ioobj(idx) );
	if ( seisinfo.isOK() )
	{
	    nrod2d++;
	    def2dky = entrylist.key(idx);
	    if ( entrylist.name(idx) == "Seis" )
		seisky = def2dky;
	}
    }

    if ( nrod2d == 1 )
	return def2dky;
    else if ( seisky.isValid() )
	return seisky;

    uiString msg = uiStrings::phrCannotFind(tr("valid default 2D data.\n"
					       "Do you want to set it now?") );
    const bool tomanage = uimsg().askGoOn( msg );
    if ( !tomanage )
	return DBKey::getInvalid();

    uiIOObjSelDlg::Setup su( tr("Set default 2D seismic data") );
    su.allowsetsurvdefault( false );
    CtxtIOObj ctio( ctxt );
    uiIOObjSelDlg dlg( parent(), su, ctio );
    if ( !dlg.go() )
	return DBKey::getInvalid();

    PtrMan<IOObj> seisobj = dlg.ioObj()->clone();
    seisobj->setSurveyDefault();
    return seisobj->key();
}


DBKey uiSeisPartServer::getDefaultDataID( bool is2d ) const
{
    if ( is2d )
	return getDefault2DDataID();

    BufferString key( IOPar::compKey(sKey::Default(),
		      SeisTrcTranslatorGroup::sKeyDefault3D()) );
    BufferString midstr( SI().getDefaultPars().find(key) );
    if ( !midstr.isEmpty() )
	return DBKey( midstr.buf() );

    const IOObjContext ctxt( mIOObjContext(SeisTrc) );
    int nrod3d = 0; DBKey def3dky;
    DBDirEntryList entrylist( ctxt );
    for ( int idx=0; idx<entrylist.size(); idx++ )
    {
	SeisIOObjInfo seisinfo( entrylist.ioobj(idx) );
	if ( seisinfo.isOK() && !seisinfo.is2D() )
	{
	    nrod3d++;
	    def3dky = entrylist.key( idx );
	}
    }

    if ( nrod3d == 1 )
	return def3dky;

    uiString msg = uiStrings::phrCannotFind( tr("valid default volume."
						"Do you want to set it now?") );
    const bool tomanage = uimsg().askGoOn( msg );
    if ( !tomanage )
	return DBKey::getInvalid();

    uiIOObjSelDlg::Setup su( tr("Set default seismic data") );
    su.allowsetsurvdefault( false );
    CtxtIOObj ctio( ctxt );
    uiIOObjSelDlg dlg( parent(), su, ctio );
    if ( !dlg.go() )
	return DBKey::getInvalid();

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
	case 0: mManageSeisDlg(man3dseisdlg_,uiSeisFileMan);
		break;
	case 1: mManageSeisDlg(man2dseisdlg_,uiSeisFileMan);
		break;
	case 2: mManageSeisDlg(man3dprestkdlg_,uiSeisPreStackMan);
		break;
	case 3: mManageSeisDlg(man2dprestkdlg_,uiSeisPreStackMan);
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
	delete impwvltdlg_;
	impwvltdlg_ = new uiSeisWvltImp(parent());
	impwvltdlg_->go();
}


void uiSeisPartServer::exportWavelets()
{
	delete expwvltdlg_;
	expwvltdlg_ = new uiSeisWvltExp(parent());
	expwvltdlg_->go();
}


void uiSeisPartServer::manageWavelets()
{
    delete manwvltdlg_;
    manwvltdlg_ = new uiSeisWvltMan( parent() );
    manwvltdlg_->go();
}


void uiSeisPartServer::exportCubePos( const DBKey* key )
{
    if ( !expcubeposdlg_ )
	expcubeposdlg_ = new uiSeisExpCubePositionsDlg( parent() );

    if ( key )
	expcubeposdlg_->setInput( *key );

    expcubeposdlg_->show();
}


bool uiSeisPartServer::select2DSeis( DBKey& mid )
{
    PtrMan<CtxtIOObj> ctio = mMkCtxtIOObj(SeisTrc2D);
    uiSeisSel::Setup setup(Seis::Line);
    uiSeisSelDlg dlg( parent(), *ctio, setup );
    if ( !dlg.go() || !dlg.ioObj() ) return false;

    mid = dlg.ioObj()->key();
    return true;
}


#define mGet2DDataSet(retval) \
    PtrMan<IOObj> ioobj = mid.getIOObj(); \
    if ( !ioobj ) return retval; \
    Seis2DDataSet dataset( *ioobj );


bool uiSeisPartServer::select2DLines( GeomIDSet& selids, int& action )
{
    selids.erase();

    uiDialog::Setup dsu( tr("Select 2D Lines"), mNoDlgTitle,
			 mODHelpKey(mSeisPartServerselect2DLinesHelpID) );
    uiDialog dlg( parent(), dsu );
    uiUserShowWait usw( parent(), uiStrings::sCollectingData() );
    uiSeis2DLineChoose* lchfld =
		new uiSeis2DLineChoose( &dlg, OD::ChooseAtLeastOne );
    uiStringSet options;
    options.add( tr("Display projection lines only") )
	   .add( tr("Load default data") )
	   .add( tr("Select attribute") )
	   .add( tr("Color blended") );
    uiGenInput* optfld =
	new uiGenInput( &dlg, tr("On OK"), StringListInpSpec(options) );
    optfld->setValue( seis2dloadaction );
    optfld->attach( alignedBelow, lchfld );
    usw.readyNow();
    if ( !dlg.go() )
	return false;

    seis2dloadaction = action = optfld->getIntValue();
    lchfld->getChosen( selids );
    return selids.size();
}


void uiSeisPartServer::get2DStoredAttribs( const char* linenm,
					   BufferStringSet& datasets,
					   int steerpol )
{
    SeisIOObjInfo::Opts2D o2d; o2d.steerpol_ = steerpol;
    SeisIOObjInfo::getDataSetNamesForLine( linenm, datasets, o2d );
}


void uiSeisPartServer::getStoredGathersList( bool for3d,
					     BufferStringSet& nms ) const
{
    ConstRefMan<DBDir> dbdir = DBM().fetchDir( IOObjContext::Seis );
    if ( !dbdir )
	return;

    DBDirIter iter( *dbdir );
    while ( iter.next() )
    {
	const IOObj& ioobj = iter.ioObj();
	const SeisIOObjInfo info( ioobj );
	if ( info.isPS() && info.is2D()!=for3d )
	    nms.add( ioobj.name() );
    }

    nms.sort();
}


void uiSeisPartServer::storeRlnAs2DLine( const Geometry::RandomLine& rln ) const
{
    uiSeisRandTo2DLineDlg dlg( parent(), &rln );
    dlg.go();
}


void uiSeisPartServer::processTime2Depth( bool is2d )
{
    uiBatchTime2DepthSetup*& dlg = is2d ? t2ddlg2d_ : t2ddlg3d_;
    if ( !dlg )
    {
	dlg = new uiBatchTime2DepthSetup( parent(), is2d );
	dlg->setModal( false );
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
    uiSeisMultiCubePS dlg( parent() );
    dlg.go();
}


void uiSeisPartServer::get2DZdomainAttribs( const char* linenm,
			const char* zdomainstr, BufferStringSet& attribs )
{
    SeisIOObjInfo::Opts2D o2d;
    o2d.zdomky_ = zdomainstr;
    SeisIOObjInfo::getDataSetNamesForLine( linenm, attribs, o2d );
}


void uiSeisPartServer::fillPar( IOPar& par ) const
{
    const ObjectSet<Seis::PreLoadDataEntry>& pls = Seis::PLDM().getEntries();
    for ( int idx=0; idx<pls.size(); idx++ )
    {
	IOPar iop;
	Seis::PreLoader spl( pls[idx]->dbkey_ );
	spl.setDefGeomID( pls[idx]->geomid_ );
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

    uiTaskRunnerProvider uitrp( parent() );
    Seis::PreLoader::load( newpar, uitrp );
    return true;
}
