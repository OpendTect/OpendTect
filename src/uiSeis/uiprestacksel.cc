/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2011
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


#include "uiprestacksel.h"

#include "ctxtioobj.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uilistbox.h"


uiPreStackDataPackSelDlg::uiPreStackDataPackSelDlg( uiParent* p,
       			const TypeSet<DataPack::FullID>& dpfids,
			const MultiID& selid )
    : uiDialog(p,uiDialog::Setup("Select Data",mNoDlgTitle,mNoHelpID))
    , dpfids_(dpfids)
    , selid_(selid)  
{
    datapackinpfld_ = new uiListBox( this );

    for ( int idx=0; idx<dpfids_.size(); idx++ )
    {
	datapackinpfld_->addItem( DataPackMgr::nameOf( dpfids_[idx] ) );
	if ( dpfids_[idx] == selid )
	    datapackinpfld_->setCurrentItem( idx );
    }
    if ( selid.isUdf() && !dpfids_.isEmpty() )
	datapackinpfld_->setCurrentItem( 0 );
};


bool uiPreStackDataPackSelDlg::acceptOK( CallBacker* )
{
    const int selidx = datapackinpfld_->currentItem();
    selid_ = dpfids_.validIdx( selidx ) ? dpfids_[selidx] : MultiID::udf();
    return true;
}



uiPreStackSel::uiPreStackSel( uiParent* p, bool is2d )
    : uiGroup(p, "Pre-Stack data selector")
    , ctio_(*uiSeisSel::mkCtxtIOObj(is2d?Seis::LinePS:Seis::VolPS,true))
    , selid_(MultiID::udf())
{
    BufferString seltxt( "Select Pre-Stack data" );
    seisinpfld_ = new uiSeisSel( this, ctio_, uiSeisSel::Setup(is2d,true) );
    datapackinpfld_ = new uiIOSelect( this, uiIOSelect::Setup(seltxt), 
				mCB(this,uiPreStackSel,doSelDataPack));

    datapackinpfld_->display( false );
    setHAlignObj( seisinpfld_ );
}


uiPreStackSel::~uiPreStackSel()
{
    delete ctio_.ioobj; delete &ctio_;
}


bool uiPreStackSel::fillPar( IOPar& par ) const
{
    return seisinpfld_->fillPar( par );
}


void uiPreStackSel::usePar( const IOPar& par ) 
{
    return seisinpfld_->usePar( par );
}


void uiPreStackSel::setInput( const MultiID& mid )
{
    if ( dpfids_.isEmpty() )
	seisinpfld_->setInput( mid );
    else
	datapackinpfld_->setInput( DataPackMgr::nameOf( mid ) );
    selid_ = mid;
}


const MultiID uiPreStackSel::getMultiID() const
{
    if ( dpfids_.isEmpty() )
	return selid_;

    BufferString mid = "#"; 
    mid += selid_;
    return mid.buf();
}


void uiPreStackSel::doSelDataPack( CallBacker* )
{
    uiPreStackDataPackSelDlg dlg( this, dpfids_, selid_ );
    if ( dlg.go() )
	setInput( dlg.getMultiID() );
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiPreStackSel::commitInput()
{
    if ( dpfids_.isEmpty() )
    {
	seisinpfld_->commitInput();
	if ( !ctio_.ioobj )
	    mErrRet( "Please select the input data store" )

	selid_ = ctio_.ioobj->key();
    }
    if ( selid_.isUdf() )
	mErrRet( "Please select a valid input" )

    return true;
}


void uiPreStackSel::setDataPackInp( const TypeSet<DataPack::FullID>& ids ) 
{
    dpfids_ = ids;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	BufferString mid; mid += ids[0];
	if ( *mid.buf() == '#' )
	{
	    const char* newmid = mid.buf() + 1;
	    dpfids_[idx] = newmid;
	}
    }

    if ( !dpfids_.isEmpty() )
	setInput( dpfids_[0] );

    datapackinpfld_->display( true );
    seisinpfld_->display( false );
}

