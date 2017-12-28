/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2011
________________________________________________________________________

-*/


#include "uiprestacksel.h"

#include "ioobjctxt.h"
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

    int curidx = 0;
    DataPack::FullID curfid = DataPack::FullID::getFromDBKey( selid );
    for ( int idx=0; idx<dpfids_.size(); idx++ )
    {
	datapackinpfld_->addItem(toUiString(DataPackMgr::nameOf(dpfids_[idx])));
	if ( dpfids_[idx] == curfid )
	    curidx = idx;
    }
    if ( !dpfids_.isEmpty() )
	datapackinpfld_->setCurrentItem( curidx );
}


bool uiPreStackDataPackSelDlg::acceptOK()
{
    const int curidx = datapackinpfld_->currentItem();
    if ( curidx < 0 )
	return false;

    selid_.setInvalid();
    dpfids_[curidx].putInDBKey( selid_ );
    return true;
}



uiPreStackSel::uiPreStackSel( uiParent* p, bool is2d )
    : uiGroup(p, "Prestack data selector")
{
    const uiSeisSel::Setup sssu( is2d, true );
    const IOObjContext ctxt( uiSeisSel::ioContext(sssu.geom_, true ) );
    seisinpfld_ = new uiSeisSel( this, ctxt, sssu );

    uiString seltxt = uiStrings::phrSelect(uiStrings::sPreStackData());
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


void uiPreStackSel::setInput( const DBKey& dbky )
{
    if ( dbky.isValid() )
	seisinpfld_->setInput( dbky );
    if ( dbky.hasAuxKey() )
    {
	const DataPack::FullID fid = DataPack::FullID::getFromDBKey( dbky );
	datapackinpfld_->setInput( fid.toString() );
    }
}


DBKey uiPreStackSel::getDBKey() const
{
    DBKey ret;
    if ( dpfids_.isEmpty() )
	ret = seisinpfld_->key( true );
    else
    {
	const BufferString dpidstr( datapackinpfld_->getKey() );
	DataPack::FullID fid = DataPack::FullID::getFromString( dpidstr );
	ret = DBKey::getInvalid();
	fid.putInDBKey( ret );
    }
    return ret;
}


void uiPreStackSel::doSelDataPack( CallBacker* )
{
    DBKey dbky = getDBKey();
    uiPreStackDataPackSelDlg dlg( this, dpfids_, dbky );
    if ( dlg.go() )
	setInput( dlg.getDBKey() );
}


#define mErrRet(msg) { uiMSG().error( msg ); return false; }

bool uiPreStackSel::inputOK()
{
    DBKey dbky = getDBKey();
    if ( !dbky.isValid() && !dbky.hasAuxKey() )
	mErrRet( uiStrings::phrSelect(tr("a valid input")) )
    return true;
}


void uiPreStackSel::setDataPackInp( const TypeSet<DataPack::FullID>& ids )
{
    dpfids_ = ids;

    const bool havedps = dpfids_.isEmpty();
    if ( havedps )
    {
	BufferStringSet kys, nms;
	for ( int idx=0; idx<dpfids_.size(); idx++ )
	{
	    const DataPack::FullID fid = dpfids_[idx];
	    kys.add( fid.toString() );
	    nms.add( DPM(fid).nameOf(fid) );
	}
	datapackinpfld_->setEntries( kys, nms );
	datapackinpfld_->setInputText( nms.get(0) );
    }

    datapackinpfld_->display( havedps );
    seisinpfld_->display( !havedps );
}
