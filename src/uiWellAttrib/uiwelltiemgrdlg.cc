/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltiemgrdlg.cc,v 1.19 2009-11-09 14:52:01 cvsbruno Exp $";

#include "uiwelltiemgrdlg.h"

#include "ioman.h"
#include "multiid.h"
#include "strmprov.h"
#include "survinfo.h"
#include "iostrm.h"
#include "wavelet.h"
#include "welldata.h"
#include "wellman.h"
#include "welltransl.h"
#include "welltiesetup.h"
#include "wellreader.h"

#include "uiseissel.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiwaveletextraction.h"
#include "uiwelltietoseismicdlg.h"

static const char* sKeyPlsSel = "Please select";

namespace WellTie
{

uiTieWinMGRDlg::uiTieWinMGRDlg( uiParent* p, WellTie::Setup& wtsetup )
	: uiDialog(p,uiDialog::Setup("Tie Well To Seismics",
		"Select Data to tie Well to Seismic","107.4.0")
		.savetext("Save as default").savebutton(true).savechecked(false))
	, wtsetup_(wtsetup)
        , wllctio_(*mMkCtxtIOObj(Well))
        , wvltctio_(*mMkCtxtIOObj(Wavelet))
    	, seisctio2d_(*uiSeisSel::mkCtxtIOObj(Seis::Line,true))
    	, seisctio3d_(*uiSeisSel::mkCtxtIOObj(Seis::Vol,true))
{
    setCtrlStyle( DoAndStay );

    if ( !wtsetup_.wellid_.isEmpty() )
	wllctio_.setObj( wtsetup_.wellid_ );
    wellfld_ = new uiIOObjSel( this, wllctio_ );
    wellfld_->selectiondone.notify( mCB(this,uiTieWinMGRDlg,wellSel) );

    const bool has2d = SI().has2D(); 
    const bool has3d = SI().has3D(); 
    const bool is2d = ( !SI().has3D() && SI().has2D() );

    seisflds_ += new uiSeisSel( this, seisctio2d_, 
	    					uiSeisSel::Setup(Seis::Line) );
    seisflds_ += new uiSeisSel( this, seisctio3d_, 
	    					uiSeisSel::Setup(Seis::Vol) );
    for ( int idx=0; idx<seisflds_.size(); idx++ )
    {
	seisflds_[idx]->setLabelText( "Seismic data" );
	seisflds_[idx]->attach( alignedBelow, wellfld_ );
    }
    seisflds_[0]->display( false );
    is2dfld_ = new uiCheckBox( this, "2D" );
    is2dfld_->display( has3d && has2d );
    is2dfld_->attach( rightOf, seisflds_[0] );

    uiLabeledComboBox* llbl1 = new uiLabeledComboBox( this,
				   "Sonic/Velocity log" );
    vellogfld_ = llbl1->box();
    llbl1->attach( alignedBelow, seisflds_[1] );
    isvelbox_ = new uiCheckBox( this, "Velocity" );
    isvelbox_->attach( rightOf, llbl1 );

    uiLabeledComboBox* llbl2 = new uiLabeledComboBox( this, "Density log");
    denlogfld_ = llbl2->box();
    llbl2->attach( alignedBelow, llbl1 );
    
    wvltfld_ = new uiIOObjSel( this, wvltctio_, "Reference wavelet" );
    wvltfld_->attach(alignedBelow, llbl2);
    uiPushButton* crwvltbut = new uiPushButton( this, "Extract",
	    			mCB(this,uiTieWinMGRDlg,extrWvlt), false );
    crwvltbut->attach( rightOf, wvltfld_ );

    finaliseDone.notify( mCB(this,uiTieWinMGRDlg,wellSel) );
}


uiTieWinMGRDlg::~uiTieWinMGRDlg()
{
    deepErase( welltiedlgsetcpy_ );
}


static void fillLogNms( uiComboBox* fld, const BufferStringSet& nms )
{
    fld->empty();
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

    getDefaults();
}


void uiTieWinMGRDlg::extrWvlt( CallBacker* )
{
    uiWaveletExtraction dlg( this );
    if ( dlg.go() )
	wvltfld_->setInput( dlg.storeKey() );
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

    for ( int idx=0; idx<seisflds_.size(); idx++ )
	seisflds_[idx]->setInput(  wtsetup_.seisid_ );
    vellogfld_->setText( wtsetup_.vellognm_ );
    denlogfld_->setText( wtsetup_.denlognm_ );
    isvelbox_ ->setChecked( !wtsetup_.issonic_ );
    //is2dfld_ -> setChecked( !wtsetup_.issonic_ );

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
    if ( !wellfld_->commitInput() )
	 mErrRet("Please select a well")
    if ( !seisflds_[1]->commitInput() )
	mErrRet("Please select the input seimic data")
    wtsetup_.seisnm_ = seisflds_[1]->getInput();
    wtsetup_.vellognm_ = vellogfld_->text();
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

    wtsetup_.seisid_ = seisflds_[1]->ctxtIOObj().ioobj->key();
    wtsetup_.wellid_ = wellfld_->ctxtIOObj().ioobj->key();
    wtsetup_.wvltid_ = wvltfld_->ctxtIOObj().ioobj->key();
    wtsetup_.issonic_ = !isvelbox_->isChecked();
    wtsetup_.unitfactors_ = units;

    if ( saveButtonChecked() )
    {
	saveWellTieSetup( wtsetup_.wellid_, wtsetup_ );
	wtsetup_.commitDefaults();
    }

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

    WellTie::uiTieWin* wtdlg = new WellTie::uiTieWin( this, wtsetup_ );
    welltiedlgset_ += wtdlg;
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
