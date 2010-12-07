/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Sep 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltiesavedatadlg.cc,v 1.15 2010-12-07 12:47:49 cvsbruno Exp $";

#include "uiwelltiesavedatadlg.h"

#include "wavelet.h"
#include "welltiedata.h"
#include "welltiesetup.h"
#include "welllog.h"
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

uiSaveDataDlg::uiSaveDataDlg(uiParent* p, const WellTie::DataHolder& dh)
    : uiDialog( p, uiDialog::Setup("Save current data",
		"Check the items to be saved","107.4.3") )
    , dataholder_(dh)
    , datawriter_(new WellTie::DataWriter(dh))	     
{
    setCtrlStyle( DoAndStay );

    BufferStringSet lognms; 	BufferStringSet wvltnms;
    for ( int idx=0; idx<dh.wvltset().size(); idx++)
    {
	wvltctioset_ += new CtxtIOObj( *dh.wvltCtxt() );
	wvltnms.add( dh.wvltset()[idx]->name() );
    }
    //TODO change with ref numbers instead of 3 and size()-2!! 
    for ( int idx=3; idx<dh.logset()->size()-2; idx++)
    {
	seisctioset_ += new CtxtIOObj( *dh.seisCtxt() );
	lognms.add( dh.logset()->getLog(idx).name() );
    }

    uiSaveDataGroup::Setup su; su.itemnames_=lognms;
    su.wellname(dataholder_.wd()->name()); su.ctio_ = seisctioset_;
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
    
    su.labelcolnm("Wavelet"); su.itemnames_ = wvltnms;
    su.ctio_ = wvltctioset_;
    savewvltsfld_ = new uiSaveDataGroup( this, su );
    savewvltsfld_->attach( stretchedBelow, horSepar );
}


uiSaveDataDlg::~uiSaveDataDlg()
{
    delete datawriter_;
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


#define mCanNotWriteLogs(msg)\
    mErrRet( "Cannot write log(s)" );
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

    for ( int idx=0; idx<wvltnms.size(); idx++ )
    {
	const char* orgwvltnm = savewvltsfld_->name( wvltidces[idx] );
	const int wvltidx = savewvltsfld_->indexOf( orgwvltnm );
	if ( wvltidx < 0 || !wvltctioset_[wvltidx]->ioobj ) 
	{ 
	    BufferString wmsg( "Can not write " ); 
	    wmsg += wvltnms.get( idx );
	    uiMSG().error( wmsg ); 
	    success = false; 
	    continue;
	}
	if ( !dataholder_.wvltset()[wvltidx]->put(wvltctioset_[wvltidx]->ioobj))
	{
	    BufferString errmsg( "cannot save " ); 
	    errmsg += wvltnms.get(idx);
	    mErrRet( errmsg );
	}
    }

    Well::LogSet logset; ;
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	const char* orglognm = savelogsfld_->name( logidces[idx] );
	const Well::Log* l = dataholder_.logset()->getLog( orglognm );
	if ( !l )
	{ 
	    BufferString logmsg( "Can not write " ); 
	    logmsg += lognms.get(idx);
	    uiMSG().error(logmsg); 
	    success = false; 
	    continue;
	}
	Well::Log* newlog = new Well::Log( *l );
	newlog->setName( lognms.get(idx) );
	logset.add( newlog );
    }

    if ( saveasfld_->getBoolValue() )
    {
	if ( !datawriter_->writeLogs( logset ) )
	    mCanNotWriteLogs();
    }
    else 
    {
	DataWriter::LogData lds( logset ); 
	lds.seisctioset_ = seisctioset_;
	lds.nrtraces_ = repeatfld_->box()->getValue(); 
	lds.ctioidxset_ = logidces; 
	if ( !datawriter_->writeLogs2Cube( lds ) )
	    mCanNotWriteLogs();
    }
    if ( success )
	uiMSG().message( "Successfully saved the selected items" );

    return false;
}



uiSaveDataGroup::uiSaveDataGroup( uiParent* p, const Setup& s )
    : uiGroup( p, "Save objects")
    , names_(s.itemnames_)
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

    for ( int idx=0; idx<names_.size(); idx++ )
    {
	objgrps_ += new uiGroup( this, "Object Group");
	BufferString objnm(names_.get(idx)); 
	
	boxflds_ += new uiCheckBox( objgrps_[0], 0 );
	lblflds_ += new uiLabel( objgrps_[1], names_.get(idx) );
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

    for ( int idx=0; idx<names_.size(); idx++ )
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
    for ( int idx=0; idx<names_.size(); idx++ )
    {
	if ( !boxflds_[idx]->isChecked() )
	    continue;
	if ( saveasioobj_ && !ioobjselflds_[idx]->commitInput() )
	{
	    BufferString msg = "Please enter a name for the ";
	    msg += names_.get(idx);
	    mErrRet( msg );
	}
	nms.add( ioobjselflds_[idx]->getInput() );
	nmidces += idx;
    }
    return true;
}

}; //namespace Well Tie
