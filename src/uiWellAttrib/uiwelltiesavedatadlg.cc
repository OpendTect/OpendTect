/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Sep 2009
________________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";

#include "uiwelltiesavedatadlg.h"

#include "seiscbvs.h"
#include "seistrctr.h"
#include "wavelet.h"
#include "welltiedata.h"
#include "welltiesetup.h"
#include "welllog.h"
#include "welldata.h"
#include "welllogset.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uitable.h"
#include "uimsg.h"


#define mErrRet(msg) { uiMSG().error(msg); return false; }
namespace WellTie
{

uiSaveDataDlg::uiSaveDataDlg(uiParent* p, const Data& d, const DataWriter& wdr )
    : uiDialog( p, uiDialog::Setup("Save current data",
		"Check the items to be saved","107.4.3") )
    , data_(d)
    , datawriter_(wdr)
{
    setCtrlStyle( DoAndStay );
    BufferStringSet lognms; 	BufferStringSet wvltnms;
    wvltctioset_ += mMkCtxtIOObj(Wavelet);
    wvltctioset_[0]->ctxt.forread = false;
    wvltctioset_ += mMkCtxtIOObj(Wavelet);
    wvltctioset_[1]->ctxt.forread = false;
    wvltnms.add( data_.initwvlt_.name() );
    wvltnms.add( data_.estimatedwvlt_.name() );

    //start at 2, the first 2 are sonic and density.
    for ( int idx=2; idx<data_.logset_.size(); idx++)
    {
	seisctioset_ += mMkCtxtIOObj(SeisTrc);
	seisctioset_[idx-2]->ctxt.deftransl =CBVSSeisTrcTranslator::translKey();
	seisctioset_[idx-2]->ctxt.forread = false;
	lognms.add( data_.logset_.getLog(idx).name() );
    }

    uiSaveDataGroup::Setup su; su.itemnames_=lognms;
    su.wellname( data_.wd_->name() ); su.ctio_ = seisctioset_;
    savelogsfld_ = new uiSaveDataGroup( this, su );

    saveasfld_ = new uiGenInput( this, "Save as", 
	    			BoolInpSpec( true, "Log", "Seismic cube") );
    saveasfld_->attach( centeredBelow, savelogsfld_ );
    saveasfld_->valuechanged.notify( 
			mCB(this,uiSaveDataDlg,changeLogUIOutput) );
    saveasfld_->valuechanged.notify( 
			mCB(savelogsfld_,uiSaveDataGroup,changeLogUIOutput) );
    
    repeatfld_ = new uiLabeledSpinBox( this, 
	    				"Duplicate trace around the track" );
    repeatfld_->attach( centeredBelow, saveasfld_);
    repeatfld_->box()->setInterval( 1, 40, 1 );
    repeatfld_->display( false );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedBelow, repeatfld_ );
    
    su.labelcolnm("Wavelet"); su.itemnames_ = wvltnms; su.saveasioobj_ = true;
    su.ctio_ = wvltctioset_;
    savewvltsfld_ = new uiSaveDataGroup( this, su );
    savewvltsfld_->attach( stretchedBelow, horSepar );
}


uiSaveDataDlg::~uiSaveDataDlg()
{
    for ( int idx=0; idx<wvltctioset_.size(); idx++ )
	delete wvltctioset_[idx]->ioobj;
    for ( int idx=0; idx<seisctioset_.size(); idx++ )
	delete seisctioset_[idx]->ioobj;
    deepErase( seisctioset_ ); deepErase( wvltctioset_ );
}


void uiSaveDataDlg::changeLogUIOutput( CallBacker* )
{
    repeatfld_->display( !saveasfld_->getBoolValue() );
}


#define mCanNotWriteLogs()\
{\
    BufferString msg = datawriter_.errMsg();\
    if ( msg.isEmpty() ) msg = "Cannot write log(s)";\
    mErrRet( msg );\
}
bool uiSaveDataDlg::acceptOK( CallBacker* )
{
    bool success = true;
    if ( !savelogsfld_ || !savewvltsfld_ ) 
	return false;
    BufferStringSet lognms, wvltnms; TypeSet<int> logidces, wvltidces;
    if ( !savelogsfld_->getNamesToBeSaved( lognms, logidces ) )
       return false;	
    if ( !savewvltsfld_->getNamesToBeSaved( wvltnms, wvltidces  ) )
	return false;

    if ( lognms.isEmpty() && wvltnms.isEmpty() )
	mErrRet( "Please check at least one item to be saved" );

    BufferString errmsg( "Can not write " ); 
    for ( int idx=0; idx<wvltnms.size(); idx++ )
    {
	const char* orgwvltnm = savewvltsfld_->itemName( wvltidces[idx] );
	const int wvltidx = savewvltsfld_->indexOf( orgwvltnm );
	if ( wvltidx < 0 || !wvltctioset_[wvltidx]->ioobj ) 
	{ 
	    errmsg += wvltnms.get( idx ); 
	    errmsg += " ";
	    uiMSG().error( errmsg );
	    success = false; 
	    continue;
	}
	Wavelet& wvlt = wvltidx ? data_.estimatedwvlt_ : data_.initwvlt_ ;
	if ( !wvlt.put( wvltctioset_[wvltidx]->ioobj ) )
	{
	    errmsg += wvltnms.get(idx);
	    errmsg += " ";
	    mErrRet( errmsg );
	}
    }

    Well::LogSet logset; ;
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	const char* orglognm = savelogsfld_->itemName( logidces[idx] );
	const Well::Log* l = data_.logset_.getLog( orglognm );
	if ( !l )
	{ 
	    errmsg += lognms.get(idx);
	    uiMSG().error(errmsg); 
	    success = false; 
	    continue;
	}
	Well::Log* newlog = new Well::Log( *l );
	newlog->setName( lognms.get(idx) );
	logset.add( newlog );
    }

    if ( saveasfld_->getBoolValue() )
    {
	if ( !datawriter_.writeLogs( logset ) )
	    mCanNotWriteLogs();
    }
    else 
    {
	DataWriter::LogData lds( logset ); 
	lds.seisctioset_ = seisctioset_;
	lds.nrtraces_ = repeatfld_->box()->getValue(); 
	lds.ctioidxset_ = logidces; 
	if ( !datawriter_.writeLogs2Cube( lds, data_.dahrg_ ) )
	    mCanNotWriteLogs();
    }
    if ( success )
	uiMSG().message( "Successfully saved the selected items" );

    return false;
}



uiSaveDataGroup::uiSaveDataGroup( uiParent* p, const Setup& s )
    : uiGroup( p, "Save objects")
    , itmnames_(s.itemnames_)
    , ctio_(s.ctio_)			  
    , saveasioobj_(s.saveasioobj_)		       
{
    for ( int idx=0; idx<3; idx++ )
    {
	objgrps_ += new uiGroup( this, "Object Group");
	if (idx) objgrps_[idx]->attach( rightOf, objgrps_[idx-1] );
    }
    titlelblflds_ += new uiLabel( this, s.labelcolnm_ );
    titlelblflds_ += new uiLabel( this, "Specify output name : " );
    checkallfld_ = new uiCheckBox( this, 0 );
    checkallfld_->activated.notify( mCB(this,uiSaveDataGroup,checkAll) );

    for ( int idx=0; idx<itmnames_.size(); idx++ )
    {
	objgrps_ += new uiGroup( this, "Object Group");
	BufferString objnm(itmnames_.get(idx)); 
	
	boxflds_ += new uiCheckBox( objgrps_[0], 0 );
	lblflds_ += new uiLabel( objgrps_[1], itmnames_.get(idx) );
	nameflds_ += new uiGenInput( objgrps_[2], "", StringInpSpec() );
	nameflds_[idx]->setText( objnm );
	ioobjselflds_ += new uiIOObjSel( objgrps_[2], *ctio_[idx], "" ); 

	nameflds_[idx]->display( !saveasioobj_ );
	ioobjselflds_[idx]->display( saveasioobj_ );
	
	if ( idx )
	{	    
	    ioobjselflds_[idx]->attach( ensureBelow, ioobjselflds_[idx-1] );
	    nameflds_[idx]->attach( ensureBelow, nameflds_[idx-1] );
	    lblflds_[idx]->attach( ensureBelow, lblflds_[idx-1] );
	    boxflds_[idx]->attach( ensureBelow, boxflds_[idx-1] );
	}
	objnm += "_"; objnm += s.wellname_;
	ioobjselflds_[idx]->setInputText( objnm );
    }
    for ( int idx=0; idx<titlelblflds_.size(); idx++ )
	titlelblflds_[idx]->attach( alignedAbove, objgrps_[idx+1]  );
}


void uiSaveDataGroup::changeLogUIOutput( CallBacker* cb )
{
    mDynamicCastGet( uiGenInput*, cber, cb );
    if ( !cber ) return;

    saveasioobj_ = !cber->getBoolValue();

    for ( int idx=0; idx<itmnames_.size(); idx++ )
    {
	nameflds_[idx]->display( !saveasioobj_ );
	ioobjselflds_[idx]->display( saveasioobj_ );
    }
}


void uiSaveDataGroup::checkAll( CallBacker* )
{
    bool arechecked = checkallfld_->isChecked(); 
    for ( int idx=0; idx<boxflds_.size(); idx++ )
	boxflds_[idx]->setChecked( arechecked );
}


bool uiSaveDataGroup::getNamesToBeSaved( BufferStringSet& nms, 
					 TypeSet<int>& nmidces )
{
    deepErase( nms );
    for ( int idx=0; idx<itmnames_.size(); idx++ )
    {
	if ( !boxflds_[idx]->isChecked() )
	    continue;
	if ( saveasioobj_ && !ioobjselflds_[idx]->commitInput() )
	{
	    BufferString msg = "Please enter a name for the ";
	    msg += itmnames_.get(idx);
	    mErrRet( msg );
	}
	nms.add( ioobjselflds_[idx]->getInput() );
	nmidces += idx;
    }
    return true;
}

}; //namespace Well Tie
