/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Sep 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltiesavedatadlg.cc,v 1.5 2009-09-29 12:13:11 cvsbruno Exp $";

#include "uiwelltiesavedatadlg.h"

#include "ioman.h"
#include "iostrm.h"
#include "strmprov.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "seiswrite.h"
#include "survinfo.h"
#include "wavelet.h"
#include "welltiedata.h"
#include "welltiesetup.h"
#include "welltieextractdata.h"
#include "wellwriter.h"

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
    , wvltctio_(*mMkCtxtIOObj(Wavelet))
    , seisctio_(*mMkCtxtIOObj(SeisTrc))
    , dataholder_(dh)				    
    , nrtimessaved_(0)				     
{
    seisctio_.ctxt.forread = false;
    BufferStringSet lognms; 	BufferStringSet wvltnms;

    for ( int idx=0; idx<dh->wvltset().size(); idx++)
	wvltnms.add( dh->wvltset()[idx]->name() );

    for ( int idx=3; idx<dh->logsset()->size()-2; idx++)
	lognms.add( dh->logsset()->getLog(idx).name() );

    uiSaveDataGroup::Setup su; su.nrtimes(nrtimessaved_); su.itemnames_=lognms;
    savelogsfld_ = new uiSaveDataGroup( this, seisctio_, su );

    saveasfld_ = new uiGenInput( this, "Save as", 
	    			BoolInpSpec( true, "Log", "Seismic cube") );
    saveasfld_->attach( centeredBelow, savelogsfld_ );
    saveasfld_->valuechanged.notify( 
			mCB(savelogsfld_,uiSaveDataGroup,changeLogUIOutput) );

    uiSeparator* horSepar = new uiSeparator( this );
    horSepar->attach( ensureBelow, saveasfld_ );

    su.colnm("Wavalet"); su.itemnames_ = wvltnms; su.saveaslog_ = false;
    savewvltsfld_ = new uiSaveDataGroup( this, wvltctio_, su );
    savewvltsfld_->attach( stretchedBelow, horSepar );
}


bool uiSaveDataDlg::acceptOK( CallBacker* )
{
    if ( !savelogsfld_ || !savewvltsfld_ ) 
	return false;
    BufferStringSet lognms, wvltnms;
    if ( !savelogsfld_->getNamesToBeSaved( lognms) 
	    	&& !savewvltsfld_->getNamesToBeSaved( wvltnms  ) )
	return false;

    if ( lognms.isEmpty() && wvltnms.isEmpty() )
	mErrRet( "No Data to save" );

    for ( int idx=0; idx<wvltnms.size(); idx++ )
    {
	const int wvltidx = savewvltsfld_->indexOf( wvltnms.get(idx) );
	if ( wvltidx <=0 ) continue;
	if ( !dataholder_->wvltset()[wvltidx]->put( wvltctio_.ioobj ) )
	{
	    BufferString errmsg( "cannot save " ); 
	    errmsg += wvltnms.get(idx);
	    mErrRet( errmsg );
	}
    }

    //TODO Create Well tie writer class instead
    Well::LogSet& logsset =const_cast<Well::LogSet&>(dataholder_->wd()->logs());
    for ( int idx=0; idx<lognms.size(); idx++ )
    {
	const Well::Log* l =dataholder_->logsset()->getLog(lognms.get(idx));
	if ( !l ) continue;
	Well::Log* newlog = new Well::Log( *l );
	logsset.add( newlog );
    }

    if ( saveasfld_->getBoolValue() )
    {
	mDynamicCastGet(const IOStream*,iostrm,IOM().get(
					    dataholder_->setup().wellid_) );
	if ( !iostrm ) return false;

	StreamProvider sp( iostrm->fileName() );
	sp.addPathIfNecessary( iostrm->dirName() );
	BufferString fname = sp.fileName();
	Well::Writer wtr( fname, *dataholder_->wd() );
	wtr.putLogs();
    }
    else
	writeLogs2Cube( logsset );

    nrtimessaved_++;
    return true;
}


bool uiSaveDataDlg::writeLogs2Cube( const Well::LogSet& logset )
{
    for ( int idx=0; idx<logset.size(); idx++ )
    {
	WellTie::TrackExtractor wtextr( 0, dataholder_->wd() );
	wtextr.timeintv_ = dataholder_->dpms()->timeintvs_[1];
	if ( !wtextr.execute() )
	    mErrRet( "unable to  track extract position" );

	const Well::Log& log = logset.getLog(idx);
	const int datasz = log.size();
	
	TypeSet<BinID> bids;
	for ( int idx=0; idx<datasz; idx++ )
	    bids += wtextr.getBIDValues()[idx];

	writeLog2Cube( log, bids );
    }
    return true;
}


bool uiSaveDataDlg::writeLog2Cube( const Well::Log& log, 
				   const TypeSet<BinID>& bids )
{
    SeisTrcWriter writer( seisctio_.ioobj );

    ObjectSet<SeisTrc> trcset;
    SeisTrc* curtrc = 0;
    const int datasz = log.size();
    BinID prevbid( bids[0] );
    for ( int idx=0; idx<datasz; idx++ )
    {
	const BinID bid( bids[idx] );
	if ( idx && bid == prevbid )
	    curtrc->set( idx, log.value(idx), 0 );
	else
	{
	    SeisTrc* newtrc = new SeisTrc( datasz );
	    trcset += newtrc;
	    for ( int sidx=0; sidx<datasz; sidx++ )
		newtrc->set( sidx, idx ? 0 : log.value(idx), 0 );
	    newtrc->info().sampling.step = SI().zStep();
	    newtrc->info().binid = bid;
	    curtrc = newtrc;
	}
	prevbid = bid;
    }
    for ( int idx=0; idx<trcset.size(); idx++ )
    {
	if ( !writer.put(*trcset[idx]) )
	    mErrRet( "cannot write new trace" );
    }
    deepErase(trcset);
    return true;
}



uiSaveDataGroup::uiSaveDataGroup( uiParent* p, CtxtIOObj& ctio, const Setup& s )
    : uiGroup( p, "Save objects")
    , ctio_(ctio)
    , names_(s.itemnames_)
    , nrtimessaved_(s.nrtimes_)
    , saveaslog_(s.saveaslog_)		       
{
    for ( int idx=0; idx<3; idx++ )
    {
	objgrps_ += new uiGroup( this, "Object Group");
	if (idx) objgrps_[idx]->attach( rightOf, objgrps_[idx-1] );
    }
    titlelblflds_ += new uiLabel( this, s.colnm_ );
    titlelblflds_ += new uiLabel( this, "Specify output name : " );

    for ( int idx=0; idx<names_.size(); idx++ )
    {
	objgrps_ += new uiGroup( this, "Object Group");
	BufferString objnm(names_.get(idx)); 
	if ( nrtimessaved_ ) objnm += (const char*)nrtimessaved_;
	
	boxflds_ += new uiCheckBox( objgrps_[0], "" );
	lblflds_ += new uiLabel( objgrps_[1], names_.get(idx) );
	ioobjselflds_ += new uiIOObjSel( objgrps_[2], ctio_, "" ); 
	ioobjselflds_[idx]->setInputText( objnm );
	nameflds_ += new uiGenInput( objgrps_[2], "", StringInpSpec() );
	nameflds_[idx]->setText( objnm );

	nameflds_[idx]->display( saveaslog_ );
	ioobjselflds_[idx]->display( !saveaslog_ );
	
	if ( idx )
	{	    
	    ioobjselflds_[idx]->attach( ensureBelow, ioobjselflds_[idx-1] );
	    nameflds_[idx]->attach( ensureBelow, nameflds_[idx-1] );
	    lblflds_[idx]->attach( ensureBelow, lblflds_[idx-1] );
	    boxflds_[idx]->attach( ensureBelow, boxflds_[idx-1] );
	}
    }
    for ( int idx=0; idx<titlelblflds_.size(); idx++ )
	titlelblflds_[idx]->attach( alignedAbove, objgrps_[idx+1]  );
}


void uiSaveDataGroup::changeLogUIOutput( CallBacker* cb )
{
    mDynamicCastGet( uiGenInput*, cber, cb );
    if ( !cber ) return;

    saveaslog_ = cber->getBoolValue();

    for ( int idx=0; idx<names_.size(); idx++ )
    {
	nameflds_[idx]->display( saveaslog_ );
	ioobjselflds_[idx]->display( !saveaslog_ );
    }
}


bool uiSaveDataGroup::getNamesToBeSaved( BufferStringSet& nms )
{
    deepErase( nms );
    for ( int idx=0; idx<names_.size(); idx++ )
    {
	if ( !boxflds_[idx]->isChecked() )
	    continue;
	if ( !ioobjselflds_[idx]->commitInput() )
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
