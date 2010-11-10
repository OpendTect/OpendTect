/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltiemgrdlg.cc,v 1.36 2010-11-10 15:26:43 cvsbert Exp $";

#include "uiwelltiemgrdlg.h"

#include "ioman.h"
#include "iostrm.h"
#include "seisioobjinfo.h"
#include "multiid.h"
#include "strmprov.h"
#include "survinfo.h"
#include "wavelet.h"
#include "welldata.h"
#include "wellman.h"
#include "welltransl.h"
#include "welltiesetup.h"
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
#include "uiseparator.h"
#include "uiwaveletextraction.h"
#include "uiwelltietoseismicdlg.h"

static const char* sKeyPlsSel = "Please select";

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
{
    setCtrlStyle( DoAndStay );

    if ( !wtsetup_.wellid_.isEmpty() )
	wllctio_.setObj( wtsetup_.wellid_ );
    wellfld_ = new uiIOObjSel( this, wllctio_ );
    wellfld_->selectionDone.notify( mCB(this,uiTieWinMGRDlg,wellSel) );

    uiSeparator* sep = new uiSeparator( this, "Well2Seismic Sep" );
    sep->attach( stretchedBelow, wellfld_ );

    uiGroup* seisgrp = new uiGroup( this, "Seismic selection group" );
    seisgrp->attach( ensureBelow, sep );
    seisgrp->attach( alignedBelow, wellfld_ );
    
    const bool has2d = SI().has2D(); 
    const bool has3d = SI().has3D(); 
    is2d_ = has2d; 

    if ( has2d && has3d )
    {
	BufferStringSet seistypes; 
	seistypes.add( Seis::nameOf(Seis::Line) );
	seistypes.add( Seis::nameOf(Seis::Vol) );
	typefld_ = new uiGenInput( seisgrp, "Seismic", 
					StringListInpSpec( seistypes ) );
	typefld_->valuechanged.notify( mCB(this,uiTieWinMGRDlg,selChg) );
	typefld_->setValue( 1 );
    }

    if ( has2d )
    {
	seis2dfld_ = new uiSeisSel( seisgrp, seisctio2d_, 
						uiSeisSel::Setup(Seis::Line));
	if ( typefld_ )
	    seis2dfld_->attach( alignedBelow, typefld_ );
	seis2dfld_->selectionDone.notify( mCB(this,uiTieWinMGRDlg,selChg) );
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

    uiLabeledComboBox* llbl1 = new uiLabeledComboBox( logsgrp,
				   "Sonic/Velocity log" );
    vellogfld_ = llbl1->box();
    isvelbox_ = new uiCheckBox( logsgrp, "Velocity" );
    isvelbox_->attach( rightOf, llbl1 );
    
    uiLabeledComboBox* llbl2 = new uiLabeledComboBox( logsgrp, "Density log");
    denlogfld_ = llbl2->box();
    llbl2->attach( alignedBelow, llbl1 );
    logsgrp->setHAlignObj( llbl1 );
    
    used2tmbox_ = new uiCheckBox( logsgrp, "Use existing depth/time model");
    used2tmbox_->attach( alignedBelow, llbl2 );
    
    sep = new uiSeparator( this, "Logs2Wavelt Sep" );
    sep->attach( stretchedBelow, logsgrp );

    wvltfld_ = new uiIOObjSel( this, wvltctio_, "Reference wavelet" );
    wvltfld_->attach( alignedBelow, wellfld_ );
    wvltfld_->attach( ensureBelow, sep );
    uiPushButton* crwvltbut = new uiPushButton( this, "Extract",
	    			mCB(this,uiTieWinMGRDlg,extrWvlt), false );
    crwvltbut->attach( rightOf, wvltfld_ );

    selChg(0);
    finaliseDone.notify( mCB(this,uiTieWinMGRDlg,wellSel) );
}


uiTieWinMGRDlg::~uiTieWinMGRDlg()
{
    delete &wtsetup_;
    delete seisctio3d_.ioobj; delete &seisctio3d_;
    delete seisctio2d_.ioobj; delete &seisctio2d_;
    delWins();
    if ( extractwvltdlg_ )
    {
	extractwvltdlg_->close();
	extractwvltdlg_->extractionDone.remove(
				mCB(this,uiTieWinMGRDlg,extractWvltDone ) );
    }
    delete extractwvltdlg_;

}


void uiTieWinMGRDlg::delWins()
{
    deepErase( welltiedlgsetcpy_ );
}


static void fillLogNms( uiComboBox* fld, const BufferStringSet& nms )
{
    fld->setEmpty();
    fld->addItem( sKeyPlsSel );
    fld->addItems( nms );
}


void uiTieWinMGRDlg::wellSel( CallBacker* )
{
    if ( !wellfld_->commitInput() )
	return;

    const char* nm = Well::IO::getMainFileName( *wllctio_.ioobj );
    if ( !nm || !*nm ) return;

    Well::Data wd; Well::Reader wr( nm, wd );
    BufferStringSet lognms; wr.getLogInfo( lognms );
    lognms.sort();
    fillLogNms( vellogfld_, lognms ); fillLogNms( denlogfld_, lognms );

    wtsetup_.wellid_ = wllctio_.ioobj->key();

    vellogfld_->setCurrentItem( lognms.nearestMatch( "Son" )+1 );
    denlogfld_->setCurrentItem( lognms.nearestMatch( "Den" )+1 );
    used2tmbox_->display( wr.getD2T() && !mIsUnvalidD2TM(wd) );
    used2tmbox_->setChecked( wr.getD2T() && !mIsUnvalidD2TM(wd) );

    getDefaults();
}


void uiTieWinMGRDlg::selChg( CallBacker* )
{
    if ( typefld_ )
	is2d_ = !typefld_->getIntValue();
    if ( is2d_ && seis3dfld_ )
	seis3dfld_->setEmpty();
    else if( !is2d_ && seis2dfld_ ) 
    {
	if ( seis2dfld_ ) seis2dfld_->setEmpty();
	if ( seislinefld_ ) seislinefld_->setInput("");
    }

    if ( seislinefld_ ) seislinefld_->display( is2d_ );
    if ( seis3dfld_ ) seis3dfld_->display( !is2d_ );

    if ( seis2dfld_ )
    {
	seis2dfld_->display( is2d_ );
	seis2dfld_->commitInput();
	if ( seisctio2d_.ioobj )
	    seislinefld_->setLineSet( seisctio2d_.ioobj->key() );
    }
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


bool uiTieWinMGRDlg::getDefaults()
{
PtrMan<IOObj> ioobj = IOM().get( wtsetup_.wellid_ );
mDynamicCastGet(const IOStream*,iostrm,ioobj.ptr())
StreamProvider sp( iostrm->fileName() );
sp.addPathIfNecessary( iostrm->dirName() );
BufferString fname( sp.fileName() );
WellTie::Reader wtr( fname, wtsetup_ );
wtr.getWellTieSetup();

    const bool was2d = wtsetup_.is2d_;
    if ( typefld_ ) typefld_->setValue( !was2d );
    if ( !wtsetup_.seisid_.isEmpty() )
    {
	if ( seis3dfld_ && !was2d ) seis3dfld_->setInput(  wtsetup_.seisid_ );
	if ( seislinefld_ ) seislinefld_->setInput( wtsetup_.linekey_ );
	if ( seis2dfld_ && was2d ) seis2dfld_->setInput(  wtsetup_.seisid_ );
    }

    vellogfld_->setText( wtsetup_.vellognm_ );
    denlogfld_->setText( wtsetup_.denlognm_ );
    isvelbox_->setChecked( !wtsetup_.issonic_ );
    if ( !wtsetup_.wvltid_.isEmpty() )
	wvltfld_->setInput( wtsetup_.wvltid_ );

    selChg(0); 

    return true;	
}


void uiTieWinMGRDlg::saveWellTieSetup( const MultiID& key,
				      const WellTie::Setup& wts )
{
    WellTie::Writer wtr( Well::IO::getMainFileName(key), wts );
    if ( !wtr.putWellTieSetup() )
	uiMSG().error( "Could not write parameters" );
}


#undef mErrRet
#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }
bool uiTieWinMGRDlg::acceptOK( CallBacker* )
{
    mDynamicCastGet( uiSeisSel*, seisfld, is2d_ ? seis2dfld_ : seis3dfld_ );
    if ( !seisfld )
	mErrRet( "Please select a seismic type" );

    if ( !wellfld_->commitInput() )
	 mErrRet("Please select a well")
    if ( !seisfld->commitInput() )
	mErrRet("Please select the input seimic data")
    wtsetup_.seisnm_ = seisfld->getInput();
    wtsetup_.vellognm_ = vellogfld_->text();
    wtsetup_.denlognm_ = denlogfld_->text();
    if ( !strcmp( wtsetup_.denlognm_, wtsetup_.vellognm_ ) )
	mErrRet("velocity and density logs are the same, please select different logs")
    if ( !strcmp ( wtsetup_.vellognm_, sKeyPlsSel ) )
	mErrRet("Please select a log for the velocity")
    wtsetup_.corrvellognm_ += wtsetup_.vellognm_;	    
    wtsetup_.denlognm_ = denlogfld_->text();
    if ( !strcmp( wtsetup_.denlognm_, sKeyPlsSel ) )
	mErrRet("Please select a Density log")
    
    if ( !wvltfld_->commitInput() )
	mErrRet("Please select an initial wavelet")
    if ( !Wavelet::get(wvltfld_->ctxtIOObj().ioobj) )
	mErrRet("Please select a valid wavelet")

    WellTie::UnitFactors units( &wtsetup_ );
    if ( !units.denFactor() || !units.velFactor()  )
	mErrRet( "invalid log units, please check your input logs" );

    if ( is2d_ )
    {
	wtsetup_.linekey_ = LineKey( seislinefld_->getInput() );
	wtsetup_.linekey_.setAttrName( seis2dfld_->attrNm() );
    }
    else
	wtsetup_.linekey_ = 0;
    wtsetup_.is2d_ = is2d_;

    wtsetup_.seisid_ = seisfld->ctxtIOObj().ioobj->key();
    wtsetup_.wellid_ = wellfld_->ctxtIOObj().ioobj->key();
    wtsetup_.wvltid_ = wvltfld_->ctxtIOObj().ioobj->key();
    wtsetup_.issonic_ = !isvelbox_->isChecked();
    wtsetup_.unitfactors_ = units;
    wtsetup_.useexistingd2tm_ = used2tmbox_->isChecked();

    if ( saveButtonChecked() )
    {
	saveWellTieSetup( wtsetup_.wellid_, wtsetup_ );
	wtsetup_.commitDefaults();
    }

    /*
    for ( int idx=0; idx<welltiedlgset_.size(); idx++ )
    {
	if ( welltiedlgset_[idx]->Setup().wellid_ == wtsetup_.wellid_  )
	{
	    BufferString errmsg = "The Window is already opened for the Well\n";
	    errmsg += wellfld_->ctxtIOObj().ioobj->name();
	    mErrRet( errmsg )
	}
    }
    for ( int idx = welltiedlgsetcpy_.size()-1; idx>=0; idx-- )
    {
	if ( welltiedlgsetcpy_[idx]->Setup().wellid_ == wtsetup_.wellid_ )
	    delete welltiedlgsetcpy_.remove( idx );
    }
    */
    WellTie::uiTieWin* wtdlg = new WellTie::uiTieWin( this, wtsetup_ );
    welltiedlgset_ += wtdlg;
    //since the win does not delonclose, windows are stored in a an ObjectSet
    //to be deleted in the destructor
    welltiedlgsetcpy_ += wtdlg;
    welltiedlgset_[welltiedlgset_.size()-1]->windowClosed.notify(
	    			mCB(this,uiTieWinMGRDlg,wellTieDlgClosed) );
    
    return false;
}


void uiTieWinMGRDlg::wellTieDlgClosed( CallBacker* cb )
{
    mDynamicCastGet(WellTie::uiTieWin*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }
    for ( int idx=0; idx<welltiedlgset_.size(); idx++ )
    {
	if ( welltiedlgset_[idx] == dlg )
	     welltiedlgset_.remove(idx); 
    }
}

}; //namespace
