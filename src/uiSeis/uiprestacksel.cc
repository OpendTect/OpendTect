/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiprestacksel.h"

#include "ctxtioobj.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uilistbox.h"


uiPreStackDataPackSelDlg::uiPreStackDataPackSelDlg( uiParent* p,
			const TypeSet<DataPack::FullID>& dpfids,
			const MultiID& selid )
    : uiDialog(p,uiDialog::Setup(tr("Select Data"),mNoDlgTitle,mNoHelpKey))
    , dpfids_(dpfids)
    , selid_(selid)
{
    datapackinpfld_ = new uiListBox( this );

    for ( int idx=0; idx<dpfids_.size(); idx++ )
    {
	datapackinpfld_->addItem(toUiString(DataPackMgr::nameOf(dpfids_[idx])));
	if ( dpfids_[idx] == DataPack::FullID(selid) )
	    datapackinpfld_->setCurrentItem( idx );
    }
    if ( selid.isUdf() && !dpfids_.isEmpty() )
	datapackinpfld_->setCurrentItem( 0 );
}


uiPreStackDataPackSelDlg::~uiPreStackDataPackSelDlg()
{}


bool uiPreStackDataPackSelDlg::acceptOK( CallBacker* )
{
    const int selidx = datapackinpfld_->currentItem();
    selid_ = dpfids_.validIdx( selidx ) ? dpfids_[selidx].asMultiID() :
								MultiID::udf();
    return true;
}



uiPreStackSel::uiPreStackSel( uiParent* p, bool is2d )
    : uiGroup(p, "Prestack data selector")
    , selid_(MultiID::udf())
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


uiPreStackSel::~uiPreStackSel()
{}


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


MultiID uiPreStackSel::getMultiID() const
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
	setInput( dlg.getMultiID() );
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
	mErrRet( tr("Please select a valid input") )

    return true;
}


void uiPreStackSel::setDataPackInp( const TypeSet<DataPack::FullID>& ids )
{
    dpfids_ = ids;
    if ( !dpfids_.isEmpty() )
	setInput( dpfids_[0].asMultiID() );

    datapackinpfld_->display( true );
    seisinpfld_->display( false );
}
