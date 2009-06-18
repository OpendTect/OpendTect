/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bruno
 Date:          Jan 2009
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiwelltiemgrdlg.cc,v 1.6 2009-06-18 07:41:52 cvsbruno Exp $";

#include "uiwelltiemgrdlg.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "ioman.h"
#include "ioobj.h"
#include "multiid.h"
#include "survinfo.h"
#include "strmdata.h"
#include "strmprov.h"
#include "iostrm.h"
#include "datacoldef.h"
#include "attribdescset.h"
#include "attribsel.h"
#include "wavelet.h"
#include "ctxtioobj.h"

#include "welldata.h"
#include "wellman.h"
#include "welltransl.h"
#include "welltiesetup.h"
#include "wellreader.h"

#include "uiattrsel.h"
#include "uiwelltietoseismicdlg.h"
#include "uiwaveletextraction.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uicombobox.h"
#include "uibutton.h"
#include "uimsg.h"
#include "uiioobjsel.h"

static const char* sKeyPlsSel = "Please select";


uiWellTieMGRDlg::uiWellTieMGRDlg( uiParent* p, WellTieSetup& wtsetup,
       			    	  const Attrib::DescSet& attrset )
	: uiDialog(p,uiDialog::Setup("Tie Well To Seismics",
		"Select Data to tie Well to Seismic",mTODOHelpID)
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
    wellfld_->selectiondone.notify( mCB(this,uiWellTieMGRDlg,wellSel) );

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
	    			mCB(this,uiWellTieMGRDlg,extrWvlt), false );
    crwvltbut->attach( rightOf, wvltfld_ );

    finaliseDone.notify( mCB(this,uiWellTieMGRDlg,wellSel) );
}


uiWellTieMGRDlg::~uiWellTieMGRDlg()
{
    deepErase( welltiedlgsetcpy_ );
    delete wllctio_.ioobj; delete &wllctio_;
    delete wvltctio_.ioobj; delete &wvltctio_;
}


static void fillLogNms( uiComboBox* fld, const BufferStringSet& nms )
{
    fld->empty();
    fld->addItem( sKeyPlsSel );
    fld->addItems( nms );
}


void uiWellTieMGRDlg::wellSel( CallBacker* )
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
    getDefaults();
}


void uiWellTieMGRDlg::extrWvlt( CallBacker* )
{
    uiWaveletExtraction dlg( this );
    if ( dlg.go() )
	wvltfld_->setInput( dlg.storeKey() );
}


bool uiWellTieMGRDlg::getDefaults()
{
    PtrMan<IOObj> ioobj = IOM().get( wtsetup_.wellid_ );
    mDynamicCastGet(const IOStream*,iostrm,ioobj.ptr())
    StreamProvider sp( iostrm->fileName() );
    sp.addPathIfNecessary( iostrm->dirName() );
    BufferString fname( sp.fileName() );
    WellTieReader wtr( fname, wtsetup_ );
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


void uiWellTieMGRDlg::saveWellTieSetup( const MultiID& key,
					const WellTieSetup& wts )
{
    WellTieWriter wtr( Well::IO::getMainFileName(key), wts );
    if ( !wtr.putWellTieSetup() )
	uiMSG().error( "Could not write parameters" );
}


#undef mErrRet
#define mErrRet(s) { if ( s ) uiMSG().error(s); return false; }

bool uiWellTieMGRDlg::acceptOK( CallBacker* )
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
    
    if ( !wvltfld_->commitInput() )
	 mErrRet("Please select an initial wavelet")

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

    welltiedlgset_ += new uiWellTieToSeismicDlg(this, wtsetup_, attrset_ );
    welltiedlgsetcpy_ += welltiedlgset_[welltiedlgset_.size()-1];
    welltiedlgset_[welltiedlgset_.size()-1]->windowClosed.notify(
	    			mCB(this,uiWellTieMGRDlg,wellTieDlgClosed));
    welltiedlgset_[welltiedlgset_.size()-1]->show();
    
    return false;
}


void uiWellTieMGRDlg::wellTieDlgClosed( CallBacker* cb )
{
    mDynamicCastGet(uiWellTieToSeismicDlg*,dlg,cb)
    if ( !dlg ) { pErrMsg("Huh"); return; }
    for ( int idx=0; idx<welltiedlgset_.size(); idx++ )
    {
	if ( welltiedlgset_[idx] == dlg )
	     welltiedlgset_.remove(idx); 
    }
}
