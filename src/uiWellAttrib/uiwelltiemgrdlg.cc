/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltiemgrdlg.cc,v 1.16 2009-09-03 14:04:30 cvsbruno Exp $";

#include "uiwelltiemgrdlg.h"

#include "attribdesc.h"
#include "ioman.h"
#include "multiid.h"
#include "strmprov.h"
#include "iostrm.h"
#include "attribdescset.h"
#include "attribsel.h"
#include "wavelet.h"
#include "welldata.h"
#include "wellman.h"
#include "welltransl.h"
#include "welltiesetup.h"
#include "wellreader.h"

#include "uiattrsel.h"
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

uiTieWinMGRDlg::uiTieWinMGRDlg( uiParent* p, WellTie::Setup& wtsetup,
       			    	  const Attrib::DescSet& attrset )
	: uiDialog(p,uiDialog::Setup("Tie Well To Seismics",
		"Select Data to tie Well to Seismic","107.4.0")
		.savetext("Save as default").savebutton(true).savechecked(false))
	, wtsetup_(wtsetup)
	, attrset_(attrset)
        , wllctio_(*mMkCtxtIOObj(Well))
        , wvltctio_(*mMkCtxtIOObj(Wavelet))
{
    setCtrlStyle( DoAndStay );

    if ( !wtsetup_.wellid_.isEmpty() )
	wllctio_.setObj( wtsetup_.wellid_ );
    wellfld_ = new uiIOObjSel( this, wllctio_ );
    wellfld_->selectiondone.notify( mCB(this,uiTieWinMGRDlg,wellSel) );

    attrfld_ = new uiAttrSel( this, "Seismic data", uiAttrSelData(attrset_) );
    attrfld_->attach( alignedBelow, wellfld_ );

    uiLabeledComboBox* llbl1 = new uiLabeledComboBox( this,
				   "Sonic/Velocity log" );
    vellogfld_ = llbl1->box();
    llbl1->attach( alignedBelow, attrfld_ );
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

    const Attrib::Desc* ad = attrset_.getDesc( 
	    Attrib::DescID( wtsetup_.attrid_.asInt(), false ) );
    if ( !ad ) return false;
    
    attrfld_  -> setInputText( ad->userRef() );
    vellogfld_-> setText( wtsetup_.vellognm_ );
    denlogfld_-> setText( wtsetup_.denlognm_ );
    isvelbox_ -> setChecked( !wtsetup_.issonic_ );

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
    wtsetup_.attrid_ = attrfld_->attribID();
    if ( !wtsetup_.attrid_.isValid() )
	mErrRet("Please select the input seimic data")
    wtsetup_.vellognm_ = vellogfld_->text();
    if ( !strcmp ( wtsetup_.vellognm_, sKeyPlsSel ) )
	mErrRet("Please select a log for the velocity")
    wtsetup_.corrvellognm_ += wtsetup_.vellognm_;	    
    wtsetup_.denlognm_ = denlogfld_->text();
    if ( !strcmp( wtsetup_.denlognm_, sKeyPlsSel ) )
	mErrRet("Please select a Density log")
    
    if ( !wvltfld_->commitInput() || !wvltfld_->ctxtIOObj().ioobj )
	mErrRet("Please select a valid wavelet")

    WellTie::UnitFactors units( &wtsetup_ );
    if ( !units.denFactor() || !units.velFactor()  )
	mErrRet( "invalid log units, please check your input logs" );

    wtsetup_.wellid_ = wellfld_->ctxtIOObj().ioobj->key();
    wtsetup_.wvltid_ = wvltfld_->ctxtIOObj().ioobj->key();
    wtsetup_.issonic_ = !isvelbox_->isChecked();

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

    WellTie::uiTieWin* wtdlg = new WellTie::uiTieWin( this, wtsetup_, attrset_);
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
