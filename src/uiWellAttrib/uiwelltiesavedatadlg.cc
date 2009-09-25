/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bruno
 Date:		Sep 2009
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwelltiesavedatadlg.cc,v 1.3 2009-09-25 16:17:30 cvsbruno Exp $";

#include "uiwelltiesavedatadlg.h"

#include "ioman.h"
#include "iostrm.h"
#include "strmprov.h"
#include "wavelet.h"
#include "welltiedata.h"
#include "welltiesetup.h"
#include "wellwriter.h"
#include "welltransl.h"

#include "uibutton.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uitable.h"
#include "uimsg.h"


#define mErrRet(msg) { uiMSG().error(msg); return false; }
namespace WellTie
{

uiSaveDataDlg::uiSaveDataDlg(uiParent* p, WellTie::DataHolder* dh)
    : uiDialog( p, uiDialog::Setup("Save current data",
		"Check the items to be saved",mTODOHelpID) )
    , wvltctio_(*mMkCtxtIOObj(Wavelet))
    , wellctio_(*mMkCtxtIOObj(Well))
    , dataholder_(dh)				    
    , nrtimessaved_(0)				     
{
    BufferStringSet lognms; 	BufferStringSet wvltnms;

    for ( int idx=0; idx<dh->wvltset().size(); idx++)
	wvltnms.add( dh->wvltset()[idx]->name() );

    for ( int idx=4; idx<dh->logsset()->size()-1; idx++)
	lognms.add( dh->logsset()->getLog(idx).name() );

    uiSaveDataTable::Setup su; su.nrtimes(nrtimessaved_); su.itemnames_=lognms;
    logstablefld_ = new uiSaveDataTable( this, wellctio_, su );

    su.colnm("Wavalet"); su.itemnames_ = wvltnms;
    wvltstablefld_ = new uiSaveDataTable( this, wvltctio_, su );
    wvltstablefld_->attach( alignedBelow, logstablefld_ );
}


bool uiSaveDataDlg::acceptOK( CallBacker* )
{
    if ( !logstablefld_ || !wvltstablefld_ ) 
	return false;
    BufferStringSet lognms, wvltnms;
    if ( !logstablefld_->getNamesToBeSaved( lognms) 
	    	&& !wvltstablefld_->getNamesToBeSaved( wvltnms  ) )
	return false;
    if ( lognms.isEmpty() && wvltnms.isEmpty() )
	mErrRet( "No Data to save" );

    for ( int idx=0; idx<wvltnms.size(); idx++ )
    {
	const int wvltidx = wvltstablefld_->indexOf( wvltnms.get(idx) );
	if ( wvltidx <=0 ) continue;
	if ( !dataholder_->wvltset()[wvltidx]->put( wvltctio_.ioobj ) )
	{
	    BufferString errmsg( "cannot save " ); 
	    errmsg += wvltnms.get(idx);
	    mErrRet( errmsg );
	}
    }

    for ( int idx=0; idx<lognms.size(); idx++)
    {
        const Well::Log* l = dataholder_->logsset()->getLog(lognms.get(idx));
	if ( !l ) continue;
	Well::Log* newlog = new Well::Log( *l );
	Well::LogSet& logsset = 
	    const_cast<Well::LogSet&>(dataholder_->wd()->logs());
	logsset.add( newlog );
    }
    mDynamicCastGet(const IOStream*,iostrm,IOM().get(
		dataholder_->setup().wellid_));
    if ( !iostrm ) return false;

    StreamProvider sp( iostrm->fileName() );
    sp.addPathIfNecessary( iostrm->dirName() );
    BufferString fname = sp.fileName();
    Well::Writer wtr( fname, *dataholder_->wd() );
    wtr.putLogs();

    nrtimessaved_++;
    return true;
}



uiSaveDataTable::uiSaveDataTable( uiParent* p, CtxtIOObj& ctio, const Setup& s )
    : uiGroup(p)
    , ctio_(ctio)
    , names_(s.itemnames_)
    , nrtimessaved_(s.nrtimes_)		   
{
    ctio_.ctxt.forread = false;
    table_ = new uiTable( this, uiTable::Setup()
				    .rowgrow(true),
				    "data table" );
    table_->setNrCols( 3 );
    table_->setColumnLabel( 0, "" );
    table_->setColumnLabel( 1, s.colnm_ );
    table_->setColumnLabel( 2, "Specify output name" );
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setColumnStretchable( 2, true );
    table_->setColumnStretchable( 1, true );
    table_->setNrRows( names_.size() );

    initTable();
}


void uiSaveDataTable::initTable()
{
    deepErase( ioobjselflds_ );
    for ( int idx=0; idx<names_.size(); idx++ )
    {
	BufferString objnm(names_.get(idx)); 
	if (nrtimessaved_) objnm+=(const char*)nrtimessaved_;
	ioobjselflds_ += new uiIOObjSel( 0, ctio_, "" );
	ioobjselflds_[idx]->setInputText( objnm );
	chckboxfld_ += new uiCheckBox( 0, "" );
	labelsfld_ += new uiLabel( 0, names_.get(idx) );
	table_->setCellObject( RowCol(idx,0), chckboxfld_[idx]  );
	table_->setCellObject( RowCol(idx,1), labelsfld_[idx] );
	table_->setCellGroup( RowCol(idx,2), ioobjselflds_[idx] );
    }
    table_->resizeColumnsToContents();
}


bool uiSaveDataTable::getNamesToBeSaved( BufferStringSet& nms )
{
    deepErase( nms );
    for ( int idx=0; idx<names_.size(); idx++ )
    {
	if ( !chckboxfld_[idx]->isChecked() )
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
