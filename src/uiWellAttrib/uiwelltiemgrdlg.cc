/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiwelltiemgrdlg.h"

#include "ioman.h"
#include "ioobj.h"
#include "multiid.h"
#include "seisioobjinfo.h"
#include "seisread.h"
#include "seistrctr.h"
#include "strmprov.h"
#include "survinfo.h"
#include "unitofmeasure.h"
#include "wavelet.h"
#include "welldata.h"
#include "welllog.h"
#include "welllogset.h"
#include "wellman.h"
#include "welltransl.h"
#include "welltiedata.h"
#include "welltiesetup.h"
#include "welltieunitfactors.h"
#include "welltiegeocalculator.h"
#include "wellreader.h"
#include "welld2tmodel.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseislinesel.h"
#include "uiseissel.h"
#include "uiseiswvltsel.h"
#include "uiseparator.h"
#include "uiwaveletextraction.h"
#include "uiwellpropertyrefsel.h"
#include "uiwelltietoseismicdlg.h"
#include "uiwelltiecheckshotedit.h"


namespace WellTie
{

uiTieWinMGRDlg::uiTieWinMGRDlg( uiParent* p, WellTie::Setup& wtsetup )
	: uiDialog(p,uiDialog::Setup("Tie Well To Seismics",
		"Select Data to tie Well to Seismic","107.4.0")
		.savetext("Save as default")
		.savebutton(true)
		.savechecked(false)
		.modal(false))
	, wtsetup_(wtsetup)
	, wllctio_(*mMkCtxtIOObj(Well))
	, wvltctio_(*mMkCtxtIOObj(Wavelet))
	, seisctio2d_(*uiSeisSel::mkCtxtIOObj(Seis::Line,true))
	, seisctio3d_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,true))
	, seis2dfld_(0)
	, seis3dfld_(0)
	, seislinefld_(0)
	, seisextractfld_(0)
	, typefld_(0)
	, extractwvltdlg_(0)
	, wd_(0)
{
    setCtrlStyle( DoAndStay );

    if ( !wtsetup_.wellid_.isEmpty() )
	wllctio_.setObj( wtsetup_.wellid_ );
    wellfld_ = new uiIOObjSel( this, wllctio_ );
    wellfld_->selectionDone.notify( mCB(this,uiTieWinMGRDlg,wellSelChg) );

    uiSeparator* sep = new uiSeparator( this, "Well2Seismic Sep" );
    sep->attach( stretchedBelow, wellfld_ );

    uiGroup* seisgrp = new uiGroup( this, "Seismic selection group" );
    seisgrp->attach( ensureBelow, sep );
    seisgrp->attach( alignedBelow, wellfld_ );
    
    const bool has2d = SI().has2D();
    const bool has3d = SI().has3D();
    setupwasused_ = false;

    if ( has2d && has3d )
    {
	BufferStringSet seistypes; 
	seistypes.add( Seis::nameOf(Seis::Line) );
	seistypes.add( Seis::nameOf(Seis::Vol) );
	typefld_ = new uiGenInput( seisgrp, "Seismic", 
					StringListInpSpec( seistypes ) );
	typefld_->valuechanged.notify( mCB(this,uiTieWinMGRDlg,seisSelChg) );
    }

    if ( has2d )
    {
	seis2dfld_ = new uiSeisSel( seisgrp, seisctio2d_, 
						uiSeisSel::Setup(Seis::Line));
	if ( typefld_ )
	    seis2dfld_->attach( alignedBelow, typefld_ );
	seis2dfld_->selectionDone.notify( mCB(this,uiTieWinMGRDlg,seisSelChg) );
	seislinefld_ = new uiSeis2DLineNameSel( seisgrp, true );
	seislinefld_->attach( alignedBelow, seis2dfld_ );
    }
    if ( has3d )
    {
	seis3dfld_ = new uiSeisSel( seisgrp, seisctio3d_, 
						uiSeisSel::Setup(Seis::Vol));
	if ( typefld_ )
	    seis3dfld_->attach( alignedBelow, typefld_ );
    }
    seisgrp->setHAlignObj( typefld_ ? (uiGroup*)typefld_ 
				    : (uiGroup*)seis2dfld_ ? 
				    seis2dfld_ : seis3dfld_ );

    sep = new uiSeparator( this, "Seismic2Log Sep" );
    sep->attach( stretchedBelow, seisgrp );
    
    uiGroup* logsgrp = new uiGroup( this, "Log selection group" );
    logsgrp->attach( alignedBelow, wellfld_ );
    logsgrp->attach( ensureBelow, sep );

    logsfld_ = new uiWellElasticPropSel( logsgrp ); 

    used2tmbox_ = new uiCheckBox( logsgrp, "Use existing depth/time model");
    used2tmbox_->activated.notify( mCB(this, uiTieWinMGRDlg, d2TSelChg ) );
    used2tmbox_->attach( alignedBelow, logsfld_ );

    const char** corrs = WellTie::Setup::CorrTypeNames();
    cscorrfld_ = new uiLabeledComboBox( logsgrp, corrs, 
	    				WellTie::Setup::sKeyCSCorrType());
    cscorrfld_->attach( alignedBelow, used2tmbox_ );
    logsgrp->setHAlignObj( cscorrfld_ );

    sep = new uiSeparator( this, "Logs2Wavelt Sep" );
    sep->attach( stretchedBelow, logsgrp );

    wvltfld_ = new uiSeisWaveletSel( this, "Reference wavelet" );
    wvltfld_->attach( alignedBelow, wellfld_ );
    wvltfld_->attach( ensureBelow, sep );
    uiPushButton* crwvltbut = new uiPushButton( this, "Extract",
	    			mCB(this,uiTieWinMGRDlg,extrWvlt), false );
    crwvltbut->attach( rightOf, wvltfld_ );

    postFinalise().notify( mCB(this,uiTieWinMGRDlg,wellSelChg) );
}


uiTieWinMGRDlg::~uiTieWinMGRDlg()
{
    delete &wtsetup_;
    if ( wd_ )
	delete wd_;
    delete seisctio3d_.ioobj; delete &seisctio3d_;
    delete seisctio2d_.ioobj; delete &seisctio2d_;
    if ( extractwvltdlg_ )
	delete extractwvltdlg_;
    delWins();

}


void uiTieWinMGRDlg::delWins()
{
    deepErase( welltiedlgsetcpy_ );
}


void uiTieWinMGRDlg::wellSelChg( CallBacker* )
{
    if ( !wellfld_->commitInput() )
	return;

    const char* nm = Well::IO::getMainFileName( *wllctio_.ioobj );
    if ( !nm || !*nm ) return;

    wd_ = new Well::Data; 
    Well::Reader wr( nm, *wd_ );
    wr.get();
    wtsetup_.wellid_ = wllctio_.ioobj->key();

    logsfld_->wellid_ = wtsetup_.wellid_;
    logsfld_->setLogs( wd_->logs() );

    used2tmbox_->display( wr.getD2T() && !mIsUnvalidD2TM((*wd_)) );
    used2tmbox_->setChecked( wr.getD2T() && !mIsUnvalidD2TM((*wd_)) );

    getDefaults();
}


void uiTieWinMGRDlg::seisSelChg( CallBacker* )
{
    setTypeFld();

    if ( seis3dfld_ )
	seis3dfld_->display( !is2d_ );
    if ( seis2dfld_ )
	seis2dfld_->display( is2d_ );
    if ( seislinefld_ )
	seislinefld_->display( is2d_ );

    if ( !is2d_ && seis3dfld_ )
	set3DSeis();
    else if ( is2d_ )
    {
	if ( seis2dfld_ )
	{
	    set2DSeis();
	    seis2dfld_->commitInput();
	}
	if ( seislinefld_ )
	{
	    setLine();
	    if ( seisctio2d_.ioobj )
		seislinefld_->setLineSet( seisctio2d_.ioobj->key() );
	}
    }
}

void uiTieWinMGRDlg::d2TSelChg( CallBacker* )
{
    const bool useexistingmdl = used2tmbox_->isChecked();
    const bool havecs = wd_->haveCheckShotModel();
    cscorrfld_->display( !useexistingmdl && havecs );
}


void uiTieWinMGRDlg::extrWvlt( CallBacker* )
{
    if ( !extractwvltdlg_ )
	extractwvltdlg_ = new uiWaveletExtraction( 0, is2d_ );
    extractwvltdlg_->extractionDone.notify(
				mCB(this,uiTieWinMGRDlg,extractWvltDone) );
    extractwvltdlg_->show();
}


void uiTieWinMGRDlg::extractWvltDone( CallBacker* )
{
    wvltfld_->setInput( extractwvltdlg_->storeKey() );
}


#undef mErrRet
#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }
bool uiTieWinMGRDlg::getDefaults()
{
    PtrMan<IOObj> ioobj = IOM().get( wtsetup_.wellid_ );
    if ( !ioobj ) return false;
    const BufferString fname( ioobj->fullUserExpr(true) );
    wd_ = Well::MGR().get( wtsetup_.wellid_, false );
    if ( !wd_ ) return false;
    WellTie::Reader wtr( fname );
    wtr.getWellTieSetup( wtsetup_ );
    logsfld_->wellid_ = wtsetup_.wellid_;

    WellTie::UnitFactors units;

    if ( !wtsetup_.denlognm_.isEmpty() )
    {
	Well::Log* den = wd_->logs().getLog( wtsetup_.denlognm_ );
	const PropertyRef::StdType tp = PropertyRef::Den;
	bool dummy = false;
	if ( !den ) mErrRet( "No valid density log selected" );
	if ( !units.getDenFactor( *den ) )
	    logsfld_->setLog( tp, wtsetup_.denlognm_, dummy, 0, 0 );
	else
	{
	    BufferString denuom = den->unitMeasLabel();
	    logsfld_->setLog( tp, wtsetup_.denlognm_, dummy,
		    	      UnitOfMeasure::getGuessed(denuom), 0 );
	}
    }

    if ( !wtsetup_.vellognm_.isEmpty() )
    {
	Well::Log* vp = wd_->logs().getLog( wtsetup_.vellognm_ );
	const PropertyRef::StdType tp = PropertyRef::Vel;
	if ( !vp ) mErrRet( "No valid velocity log selected" );
	if ( !units.getVelFactor( *vp, wtsetup_.issonic_ ) )
	    logsfld_->setLog( tp, wtsetup_.vellognm_, wtsetup_.issonic_, 0, 1 );
	else
	{
	    BufferString veluom = vp->unitMeasLabel();
	    logsfld_->setLog( tp, wtsetup_.vellognm_, wtsetup_.issonic_,
		    	      UnitOfMeasure::getGuessed(veluom), 1 );
	}
    }

    if ( !wtsetup_.wvltid_.isEmpty() )
	wvltfld_->setInput( wtsetup_.wvltid_ );

    seisSelChg(0);
    d2TSelChg(0);

    return true;
}


void uiTieWinMGRDlg::saveWellTieSetup( const MultiID& key,
				      const WellTie::Setup& wts ) const
{
    WellTie::Writer wtr( Well::IO::getMainFileName(key) );
    if ( !wtr.putWellTieSetup( wts ) )
	uiMSG().error( "Could not write parameters" );
}



#undef mErrRet
#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }
bool uiTieWinMGRDlg::initSetup()
{
    mDynamicCastGet( uiSeisSel*, seisfld, is2d_ ? seis2dfld_ : seis3dfld_ );
    if ( !seisfld )
	mErrRet( "Please select a seismic type" )

    if ( !seisfld->commitInput() )
	mErrRet("Please select the input seimic data")
    if ( !wellfld_->commitInput() || !wd_ )
	mErrRet("Please select a valid well")
    if ( !wvltfld_->getWavelet() )
	mErrRet("Please select a valid wavelet")

    wtsetup_.seisnm_ = seisfld->getInput();

    for ( int idx=0; idx<welltiedlgset_.size(); idx++ )
    {
	uiTieWin* win = welltiedlgset_[idx];
	if ( win->Setup().wellid_ == wellfld_->ctxtIOObj().ioobj->key() )
	    mErrRet( "A window with this well is already opened" )
    }

    if ( !logsfld_->isOK() )
	return false;

    wd_ = Well::MGR().get( wtsetup_.wellid_, false );
    if ( !wd_ )
	return false;

    Well::Data* loadedwd =  Well::MGR().get( wtsetup_.wellid_, false );
    if ( !loadedwd ) loadedwd = wd_;

    uiPropSelFromList* psflden = logsfld_->getPropSelFromListByName("Density");
    if ( !psflden ) return false;
    wtsetup_.denlognm_ = psflden->text();
    Well::Log* den = loadedwd->logs().getLog( wtsetup_.denlognm_ );
    if ( !den )
	mErrRet( "Could not extract this density log" )
    else if ( !psflden->uom() )
	mErrRet( "Please select a unit for the density log" )
    else
	den->setUnitMeasLabel( psflden->uom()->symbol() );

    uiPropSelFromList* psflvp = logsfld_->getPropSelFromListByName("Velocity");
    if ( !psflvp ) return false;
    wtsetup_.vellognm_ = psflvp->text();
    wtsetup_.issonic_  = psflvp->isUseAlternate();
    Well::Log* vp = loadedwd->logs().getLog( wtsetup_.vellognm_ );
    if ( !vp )
	mErrRet( "Could not extract this velocity log" )
    else if ( !psflvp->uom() )
	mErrRet( "Please select a unit for the velocity log" )
    else
	vp->setUnitMeasLabel( psflvp->uom()->symbol() );

    if ( is2d_ )
    {
	wtsetup_.linekey_.setAttrName( seis2dfld_->attrNm() );
	wtsetup_.linekey_ = LineKey( seislinefld_->getInput() );
    }
    else
	wtsetup_.linekey_ = 0;

    wtsetup_.seisid_ = seisfld->ctxtIOObj().ioobj->key();
    wtsetup_.wellid_ = wellfld_->ctxtIOObj().ioobj->key();
    wtsetup_.wvltid_ = wvltfld_->getID();

    wtsetup_.useexistingd2tm_ = used2tmbox_->isChecked();
    WellTie::Setup::parseEnumCorrType( cscorrfld_->box()->text(), 
	    				wtsetup_.corrtype_); 

    if ( saveButtonChecked() )
    {
	saveWellTieSetup( wtsetup_.wellid_, wtsetup_ );
	wtsetup_.commitDefaults();
    }

    return true;
}


bool uiTieWinMGRDlg::acceptOK( CallBacker* )
{
    if ( !initSetup() )
	return false;

    Server* server = new Server( wtsetup_ );
    if ( server->errMSG() ) 
	{ mErrRet( server->errMSG() ); delete server; return false; }

    if ( wtsetup_.corrtype_ == WellTie::Setup::UserDefined )
    {
	uiCheckShotEdit dlg( this, *server );
	if ( !dlg.go() )
	    { delete server; return false; }
    }

    WellTie::uiTieWin* wtdlg = new WellTie::uiTieWin( this, *server );
    welltiedlgset_ += wtdlg;
    welltiedlgsetcpy_ += wtdlg;
    //windows are stored in a an ObjectSet to be deleted in the destructor
    welltiedlgset_[welltiedlgset_.size()-1]->windowClosed.notify(
				mCB(this,uiTieWinMGRDlg,wellTieDlgClosed) );

    PtrMan<IOObj> ioobj = IOM().get( wtsetup_.wellid_ );
    if ( !ioobj ) return false;

    const BufferString fname( ioobj->fullUserExpr(true) );
    WellTie::Reader wtr( fname );
    IOPar* par= wtr.getIOPar( uiTieWin::sKeyWinPar() );
    if ( par ) wtdlg->usePar( *par );
    delete par;

    return false;
}


void uiTieWinMGRDlg::wellTieDlgClosed( CallBacker* cb )
{
    mDynamicCastGet(WellTie::uiTieWin*,win,cb)
    if ( !win ) { pErrMsg("Huh"); return; }
    for ( int idx=0; idx<welltiedlgset_.size(); idx++ )
    {
	if ( welltiedlgset_[idx] == win )
	{
	    welltiedlgset_.removeSingle(idx);
	    WellTie::Writer wtr( 
		    	Well::IO::getMainFileName( win->Setup().wellid_) );
	    IOPar par; win->fillPar( par );
	    wtr.putIOPar( par, uiTieWin::sKeyWinPar() ); 
	}
    }
}

#define mGetPar(key) \
    defpars->find(SeisTrcTranslatorGroup::key())

void uiTieWinMGRDlg::set3DSeis() const
{
    const LineKey curlk( seis3dfld_->getKey() );
    const MultiID curid = curlk.lineName().buf();
    PtrMan<IOPar> defpars = SI().pars().subselect( sKey::Default() );
    const FixedString seisidstr = defpars ? mGetPar( sKeyDefault3D ) : 0;
    const MultiID defaultid = seisidstr;
    const MultiID setupid = wtsetup_.seisid_.isEmpty() ? 0 : wtsetup_.seisid_;

    if ( !seis3dfld_->isEmpty() && ( curid != defaultid && curid != setupid ) )
	return;
    if ( !wtsetup_.seisid_.isEmpty() )
    {
	if ( seisIDIs3D( wtsetup_.seisid_ ) )
	{
	    seis3dfld_->setInput( wtsetup_.seisid_ );
	}
	return;
    }
    else
    {
	seis3dfld_->setInput( defaultid );
	return;
    }
}


void uiTieWinMGRDlg::set2DSeis() const
{
    const LineKey curlk( seis2dfld_->getKey() );
    const MultiID curid = curlk.lineName().buf();
    PtrMan<IOPar> defpars = SI().pars().subselect( sKey::Default() );
    const FixedString lsid = defpars ? mGetPar( sKeyDefault2D ) : 0;
    const MultiID defaultid = lsid;
    const MultiID setupid = wtsetup_.seisid_.isEmpty() ? 0 : wtsetup_.seisid_;

    if ( !seis2dfld_->isEmpty() && ( curid != defaultid && curid != setupid ) )
	return;
    if ( !wtsetup_.seisid_.isEmpty() )
    {
	if ( !seisIDIs3D( wtsetup_.seisid_ ) )
	{
	    seis2dfld_->setInput( wtsetup_.seisid_ );
	}
	return;
    }
    else
    {
	BufferString lineidstr;
	PtrMan<IOObj> lsobj = IOM().get( MultiID(lsid) );
	BufferString attrnm = mGetPar( sKeyDefaultAttrib );
	if ( lsobj && attrnm.isEmpty() )
	{
	    SeisIOObjInfo seisinfo( lsobj );
	    BufferStringSet attrnms;
	    SeisIOObjInfo::Opts2D o2d; o2d.steerpol_ = 0;
	    seisinfo.getAttribNames( attrnms, o2d );
	    if ( !attrnms.isEmpty() )
		attrnm = attrnms.get(0);
	}
	lineidstr = LineKey( lsid, attrnm );
	const MultiID seisid = lineidstr.buf();
	seis2dfld_->setInput( seisid );
	return;
    }
}


void uiTieWinMGRDlg::setLine() const
{
    BufferString linekey = "";

    if ( wtsetup_.linekey_ != 0 )
    {
	linekey = wtsetup_.linekey_;
    }
    else if ( seislinefld_->getInput() )
    {
	return;
    }

    seislinefld_->setInput( linekey );
}


void uiTieWinMGRDlg::setTypeFld()
{
    if ( !setupwasused_ )
    {
	is2d_ = SI().has2D();
    }
    else if ( typefld_ )
    {
	is2d_ = !typefld_->getIntValue();
    }

    if ( !wtsetup_.seisid_.isEmpty() && !setupwasused_ )
    {
	is2d_ = !seisIDIs3D( wtsetup_.seisid_ );
	setupwasused_ = true;
    }

    if ( typefld_ )
	typefld_->setValue( !is2d_ );
}


bool uiTieWinMGRDlg::seisIDIs3D( MultiID seisid ) const
{
    PtrMan<IOObj> ioobj = IOM().get( seisid );
    SeisTrcReader rdr( ioobj );

    return !rdr.is2D();
}

}; //namespace
