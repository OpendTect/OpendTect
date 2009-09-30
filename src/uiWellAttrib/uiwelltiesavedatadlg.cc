/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Sep 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltiesavedatadlg.cc,v 1.7 2009-09-30 11:00:13 cvsbruno Exp $";

#include "uiwelltiesavedatadlg.h"

#include "wavelet.h"
#include "welltiedata.h"
#include "welltiesetup.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uiseparator.h"
#include "uitable.h"
#include "uimsg.h"


#define mErrRet(msg) { uiMSG().error(msg); return false; }
namespace WellTie
{

uiSaveDataDlg::uiSaveDataDlg(uiParent* p, WellTie::DataHolder* dh)
    : uiDialog( p, uiDialog::Setup("Save current data",
		"Check the items to be saved",mTODOHelpID) )
    , dataholder_(dh)
    , datawriter_(new WellTie::DataWriter(dh))	     
    , nrtimessaved_(0)
{
    setCtrlStyle( DoAndStay );

    BufferStringSet lognms; 	BufferStringSet wvltnms;

    for ( int idx=0; idx<dh->wvltset().size(); idx++)
    {
	wvltctio_ += new CtxtIOObj( dh->wvltCtxt() );
	wvltnms.add( dh->wvltset()[idx]->name() );
    }

    for ( int idx=3; idx<dh->logsset()->size()-2; idx++)
    {
	seisctio_ += new CtxtIOObj( dh->seisCtxt() );
	lognms.add( dh->logsset()->getLog(idx).name() );
    }

    uiSaveDataGroup::Setup su; su.nrtimes(nrtimessaved_); su.itemnames_=lognms;
    su.wellname(dataholder_->wd()->name()); su.ctio_ = seisctio_;
    savelogsfld_ = new uiSaveDataGroup( this, su );

    saveasfld_ = new uiGenInput( this, "Save as", 
	    			BoolInpSpec( true, "Log", "Seismic cube") );
    saveasfld_->attach( centeredBelow, savelogsfld_ );
    saveasfld_->valuechanged.notify( 
			mCB(savelogsfld_,uiSaveDataGroup,changeLogUIOutput) );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( stretchedBelow, saveasfld_ );

    su.labelcolnm("Wavalet"); su.itemnames_ = wvltnms; su.uselabelsel_ = false;
    su.ctio_ = wvltctio_;
    savewvltsfld_ = new uiSaveDataGroup( this, su );
    savewvltsfld_->attach( stretchedBelow, horSepar );
}


uiSaveDataDlg::~uiSaveDataDlg()
{
    delete datawriter_;
    for ( int idx=0; idx<wvltctio_.size(); idx++ )
	delete wvltctio_[idx]->ioobj;
    for ( int idx=0; idx<seisctio_.size(); idx++ )
	delete seisctio_[idx]->ioobj;
    deepErase( seisctio_ ); deepErase( wvltctio_ );
}


#define mCanNotWriteLogs(msg)\
    mErrRet( "Cannot write logs" );
bool uiSaveDataDlg::acceptOK( CallBacker* )
{
    if ( !savelogsfld_ || !savewvltsfld_ ) 
	return false;
    BufferStringSet lognms, wvltnms;
    if ( !savelogsfld_->getNamesToBeSaved( lognms) )
       return false;	
    if ( !savewvltsfld_->getNamesToBeSaved( wvltnms  ) )
	return false;

    if ( lognms.isEmpty() && wvltnms.isEmpty() )
	mErrRet( "Please check at least one item to be saved" );

    for ( int idx=0; idx<wvltnms.size(); idx++ )
    {
	const int wvltidx = savewvltsfld_->indexOf( wvltnms.get(idx) );
	if ( wvltidx <=0 ) continue;
	if ( !dataholder_->wvltset()[wvltidx]->put( wvltctio_[idx]->ioobj ) )
	{
	    BufferString errmsg( "cannot save " ); 
	    errmsg += wvltnms.get(idx);
	    mErrRet( errmsg );
	}
    }

    Well::LogSet logset;
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	const Well::Log* l = dataholder_->logsset()->getLog(lognms.get(idx));
	if ( !l ) continue;
	Well::Log* newlog = new Well::Log( *l );
	logset.add( newlog );
    }

    if ( saveasfld_->getBoolValue() )
    {
	if ( !datawriter_->writeLogs( logset ) )
	    mCanNotWriteLogs();
    }
    else 
    {
	DataWriter::LogData lds( logset ); lds.seisctioset_ = seisctio_;
	if ( !datawriter_->writeLogs2Cube( lds ) )
	    mCanNotWriteLogs();
    }

    nrtimessaved_++;
    uiMSG().message( "Successfully saved the selected items" );

    return false;
}


uiSaveDataGroup::uiSaveDataGroup( uiParent* p, const Setup& s )
    : uiGroup( p, "Save objects")
    , names_(s.itemnames_)
    , ctio_(s.ctio_)			  
    , nrtimessaved_(s.nrtimes_)
    , uselabelsel_(s.uselabelsel_)		       
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
	if ( nrtimessaved_ ) objnm += (const char*)nrtimessaved_;
	
	boxflds_ += new uiCheckBox( objgrps_[0], 0 );
	lblflds_ += new uiLabel( objgrps_[1], names_.get(idx) );
	nameflds_ += new uiGenInput( objgrps_[2], "", StringInpSpec() );
	nameflds_[idx]->setText( objnm );
	ioobjselflds_ += new uiIOObjSel( objgrps_[2], *ctio_[idx], "" ); 

	nameflds_[idx]->display( uselabelsel_ );
	ioobjselflds_[idx]->display( !uselabelsel_ );
	
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

    uselabelsel_ = cber->getBoolValue();

    for ( int idx=0; idx<names_.size(); idx++ )
    {
	nameflds_[idx]->display( uselabelsel_ );
	ioobjselflds_[idx]->display( !uselabelsel_ );
    }
}


void uiSaveDataGroup::checkAll( CallBacker* )
{
    bool arechecked = checkallfld_->isChecked(); 
    for ( int idx=0; idx<boxflds_.size(); idx++ )
	boxflds_[idx]->setChecked( arechecked );
}


bool uiSaveDataGroup::getNamesToBeSaved( BufferStringSet& nms )
{
    deepErase( nms );
    for ( int idx=0; idx<names_.size(); idx++ )
    {
	if ( !boxflds_[idx]->isChecked() )
	    continue;
	if ( !uselabelsel_ && !ioobjselflds_[idx]->commitInput() )
	{
	    BufferString msg = "Please enter a name for the ";
	    msg += names_.get(idx);
	    mErrRet( msg );
	}
	nms.add( names_.get(idx) );
    }
    return true;
}

}; //namespace Well Tie
