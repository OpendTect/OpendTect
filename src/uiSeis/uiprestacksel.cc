/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2011
________________________________________________________________________

-*/


#include "uiprestacksel.h"

#include "ctxtioobj.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uilistbox.h"


uiPreStackDataPackSelDlg::uiPreStackDataPackSelDlg( uiParent* p,
			const TypeSet<DataPack::FullID>& dpfids,
			const DBKey& selid )
    : uiDialog(p,uiDialog::Setup(tr("Select Data"),mNoDlgTitle,mNoHelpKey))
    , dpfids_(dpfids)
    , selid_(selid)
{
    datapackinpfld_ = new uiListBox( this );

    for ( int idx=0; idx<dpfids_.size(); idx++ )
    {
	datapackinpfld_->addItem(toUiString(DataPackMgr::nameOf(dpfids_[idx])));
	if ( dpfids_[idx] == selid )
	    datapackinpfld_->setCurrentItem( idx );
    }
    if ( selid.isUdf() && !dpfids_.isEmpty() )
	datapackinpfld_->setCurrentItem( 0 );
}


bool uiPreStackDataPackSelDlg::acceptOK()
{
    const int selidx = datapackinpfld_->currentItem();
    selid_ = dpfids_.validIdx( selidx ) ? dpfids_[selidx] : DBKey::getInvalid();
    return true;
}



uiPreStackSel::uiPreStackSel( uiParent* p, bool is2d )
    : uiGroup(p, "Prestack data selector")
    , selid_(DBKey::getInvalid())
{
    const uiSeisSel::Setup sssu( is2d, true );
    const IOObjContext ctxt( uiSeisSel::ioContext(sssu.geom_, true ) );
    seisinpfld_ = new uiSeisSel( this, ctxt, sssu );

    uiString seltxt = uiStrings::phrSelect(mJoinUiStrs(sPreStack(),sData()));
    datapackinpfld_ = new uiIOSelect( this, uiIOSelect::Setup(seltxt),
				mCB(this,uiPreStackSel,doSelDataPack));

    datapackinpfld_->display( false );
    setHAlignObj( seisinpfld_ );
}


bool uiPreStackSel::fillPar( IOPar& par ) const
{
    return seisinpfld_->fillPar( par );
}


void uiPreStackSel::usePar( const IOPar& par )
{
    return seisinpfld_->usePar( par );
}


void uiPreStackSel::setInput( const DBKey& mid )
{
    if ( dpfids_.isEmpty() )
	seisinpfld_->setInput( mid );
    else
    {
	const DataPack::FullID fid( mid.subID(0), mid.subID(1) );
	datapackinpfld_->setInput( DataPackMgr::nameOf(fid) );
    }
    selid_ = mid;
}


DBKey uiPreStackSel::getDBKey() const
{
    if ( dpfids_.isEmpty() )
	return selid_;

    BufferString mid( "#", selid_ );
    return mid.buf();
}


void uiPreStackSel::doSelDataPack( CallBacker* )
{
    uiPreStackDataPackSelDlg dlg( this, dpfids_, selid_ );
    if ( dlg.go() )
	setInput( dlg.getDBKey() );
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiPreStackSel::commitInput()
{
    if ( dpfids_.isEmpty() )
    {
	const IOObj* ioobj = seisinpfld_->ioobj();
	if ( !ioobj )
	    return false;

	selid_ = ioobj->key();
    }

    if ( selid_.isUdf() )
	mErrRet( uiStrings::phrSelect(tr("a valid input")) )

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
