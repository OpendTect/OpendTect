/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		April 2014
________________________________________________________________________

-*/



#include "uibatchhostsdlg.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uitable.h"
#include "uitoolbutton.h"

#include "file.h"
#include "hostdata.h"
#include "oddirs.h"
#include "od_helpids.h"
#include "systeminfo.h"


static const int sIPCol		= 0;
static const int sHostNameCol	= 1;
static const int sDispNameCol	= 2;
static const int sPlfCol	= 3;
static const int sDataRootCol	= 4;

#define mAdvSettingsStr uiStrings::sAdvancedSettings()


uiBatchHostsDlg::uiBatchHostsDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Setup Distributed Processing"),mNoDlgTitle,
		       mODHelpKey(mBatchHostsDlgHelpID)))
    , hostdatalist_(*new HostDataList(true))
{
    const File::Path bhfp = hostdatalist_.getBatchHostsFilename();
    const BufferString bhfnm = bhfp.fullPath();
    const BufferString& datadir = bhfp.pathOnly();
    bool writeallowed = File::exists(datadir) && File::isWritable( datadir );
    if ( writeallowed && File::exists(bhfnm) )
	writeallowed = File::isWritable( bhfnm );

    if ( writeallowed )
	setOkText( uiStrings::sSave() );
    else
	setCtrlStyle( CloseOnly );

    uiGenInput* filefld = new uiGenInput( this, tr("BatchHosts file") );
    filefld->setElemSzPol( uiObject::WideMax );
    filefld->setText( bhfnm );
    filefld->setReadOnly();

    uiPushButton* advbut =
	new uiPushButton( this, mAdvSettingsStr, false );
    advbut->activated.notify( mCB(this,uiBatchHostsDlg,advbutCB) );
    advbut->setIcon( "settings" );
    advbut->attach( rightTo, filefld );
    advbut->attach( rightBorder );

    uiTable::Setup tsu( -1, 5 );
    tsu.rowdesc(uiStrings::sHost()).defrowlbl(true).selmode(uiTable::SingleRow);
    table_ = new uiTable( this, tsu, "Batch Hosts" );
    uiStringSet collbls;
    collbls.add( tr("IP address") ).add( uiStrings::sHostName() )
	   .add( tr("Display Name") ).add( uiStrings::sPlatform() )
	   .add( tr("Survey Data Root") );
    table_->setColumnLabels( collbls );
    table_->setPrefWidth( 800 );
    table_->resizeHeaderToContents( true );
    table_->setTableReadOnly( !writeallowed );
    table_->setSelectionMode( uiTable::NoSelection );
    table_->valueChanged.notify( mCB(this,uiBatchHostsDlg,changedCB) );
    table_->rowClicked.notify( mCB(this,uiBatchHostsDlg,hostSelCB) );
    table_->attach( leftAlignedBelow, filefld );

    autobox_ = new uiCheckBox( this,
	tr("Automatically fill in IP address or Hostname") );
    autobox_->setChecked( true );
    autobox_->attach( alignedBelow, table_ );

    uiButtonGroup* buttons = new uiButtonGroup( this, "", OD::Vertical );
    new uiToolButton( buttons, "addnew", uiStrings::phrAdd(uiStrings::sHost()),
			mCB(this,uiBatchHostsDlg,addHostCB) );
    uiToolButton::getStd( buttons, OD::Remove,
			  mCB(this,uiBatchHostsDlg,rmHostCB),
			  uiStrings::phrRemove(uiStrings::sHost()) );
    upbut_ = new uiToolButton( buttons, uiToolButton::UpArrow,
			uiStrings::sMoveUp(),
			mCB(this,uiBatchHostsDlg,moveUpCB) );
    downbut_ = new uiToolButton( buttons, uiToolButton::DownArrow,
			uiStrings::sMoveDown(),
			mCB(this,uiBatchHostsDlg,moveDownCB) );
    uiToolButton* testbut = new uiToolButton( buttons, "checkgreen",
			tr("Test Hosts"),
			mCB(this,uiBatchHostsDlg,testHostsCB) );
    buttons->attach( rightTo, table_ );
    buttons->setChildrenSensitive( writeallowed );
    testbut->setSensitive( true );

    fillTable();
}


uiBatchHostsDlg::~uiBatchHostsDlg()
{
    delete &hostdatalist_;
}


void uiBatchHostsDlg::advbutCB( CallBacker* )
{
    uiDialog dlg( this, uiDialog::Setup(mAdvSettingsStr,
					mNoDlgTitle,mNoHelpKey) );

    uiLabel* albl = new uiLabel( &dlg, tr("Settings for all platforms:") );
    albl->attach( leftBorder );
    const int portnr = hostdatalist_.firstPort();
    uiGenInput* portnrfld = new uiGenInput( &dlg, tr("First Port"),
					    IntInpSpec(portnr) );
    portnrfld->attach( ensureBelow, albl );

    uiSeparator* sep = new uiSeparator( &dlg );
    sep->attach( stretchedBelow, portnrfld );

    uiLabel* ulbl = new uiLabel( &dlg, tr("Settings for UNIX only:") );
    ulbl->attach( leftBorder );
    ulbl->attach( ensureBelow, sep );

    uiStringSet cmds;
    cmds += toUiString("ssh");
    cmds += toUiString("rsh");
    cmds += uiString::empty();
    uiGenInput* remoteshellfld = new uiGenInput( &dlg,
				tr("Remote shell command"),
				BoolInpSpec(true,cmds[0],cmds[1]) );
    remoteshellfld->setText( hostdatalist_.loginCmd() );
    remoteshellfld->attach( alignedBelow, portnrfld );
    remoteshellfld->attach( ensureBelow, ulbl );

    const StepInterval<int> nicelvlrg( -19, 19, 1 );
    const int nicelvl = hostdatalist_.niceLevel();
    uiGenInput* nicelvlfld = new uiGenInput( &dlg, tr("Nice level"),
				  IntInpSpec(nicelvl,nicelvlrg) );
    nicelvlfld->attach( alignedBelow, remoteshellfld );

    uiSeparator* sep2 = new uiSeparator( &dlg );
    sep2->attach( stretchedBelow, nicelvlfld );

    uiLabel* drlbl = new uiLabel( &dlg, tr("Default Survey Data Root:") );
    drlbl->attach( leftBorder );
    drlbl->attach( ensureBelow, sep2 );

    uiGenInput* unixdrfld = new uiGenInput( &dlg, tr("Unix hosts") );
    unixdrfld->setText( hostdatalist_.unixDataRoot() );
    unixdrfld->setElemSzPol( uiObject::Wide );
    unixdrfld->attach( ensureBelow, drlbl );
    unixdrfld->attach( alignedBelow, nicelvlfld );

    uiGenInput* windrfld = new uiGenInput( &dlg, tr("Windows hosts") );
    windrfld->setText( hostdatalist_.winDataRoot() );
    windrfld->setElemSzPol( uiObject::Wide );
    windrfld->attach( alignedBelow, unixdrfld );

    if ( !dlg.go() ) return;

    const int cmdres = (int)(!remoteshellfld->getBoolValue());
    hostdatalist_.setLoginCmd( mFromUiStringTodo(cmds[cmdres]) );
    hostdatalist_.setNiceLevel( nicelvlfld->getIntValue() );
    hostdatalist_.setFirstPort( portnrfld->getIntValue() );
    hostdatalist_.setUnixDataRoot( unixdrfld->text() );
    hostdatalist_.setWinDataRoot( windrfld->text() );
}


static void setIPAddress( uiTable& tbl, int row, const HostData& hd )
{
    const RowCol rc( row, sIPCol );
    tbl.setText( rc, hd.getIPAddress() );
    tbl.setCellColor( rc, Color::White() );
}


static void setHostName( uiTable& tbl, int row, const HostData& hd )
{
    const RowCol rc( row, sHostNameCol );
    tbl.setText( rc, hd.getHostName() );
    tbl.setCellColor( rc, Color::White() );
}


static void setDisplayName( uiTable& tbl, int row, const HostData& hd )
{
    const char* nm = hd.nrAliases()>0 ? hd.alias(0) : hd.getHostName();
    tbl.setText( RowCol(row,sDispNameCol), nm );
}


static void setPlatform( uiTable& tbl, int row, const HostData& hd )
{
    uiObject* cellobj = tbl.getCellObject( RowCol(row,2) );
    mDynamicCastGet(uiComboBox*,cb,cellobj)
    if ( !cb )
    {
	cb = new uiComboBox( 0, OD::Platform::TypeDef(),
			     "Platforms" );
	tbl.setCellObject( RowCol(row,sPlfCol), cb );
    }

    cb->setValue( hd.getPlatform().type() );
}


static void setDataRoot( uiTable& tbl, int row, const HostData& hd )
{
    const BufferString dataroot = hd.getDataRoot().fullPath();
    tbl.setText( RowCol(row,sDataRootCol), dataroot );
}


static void updateDisplayName( uiTable& tbl, int row, HostData& hd )
{
    BufferString dispnm = tbl.text( RowCol(row,sDispNameCol) );
    if ( dispnm.isEmpty() )
	dispnm = tbl.text( RowCol(row,sHostNameCol) );
    if ( dispnm.isEmpty() )
	dispnm = tbl.rowLabel( row );

    hd.setAlias( dispnm );
    setDisplayName( tbl, row, hd );
}


void uiBatchHostsDlg::fillTable()
{
    NotifyStopper ns( table_->valueChanged );

    const int nrhosts = hostdatalist_.size();
    table_->setNrRows( nrhosts );
    if ( nrhosts<4 )
	table_->setPrefHeightInRows( 4 );

    for ( int idx=0; idx<nrhosts; idx++ )
    {
	HostData* hd = hostdatalist_[idx];
	setIPAddress( *table_, idx, *hd );
	checkIPAddress( idx );
	setHostName( *table_, idx, *hd );
	setDisplayName( *table_, idx, *hd );
	setPlatform( *table_, idx, *hd );
	setDataRoot( *table_, idx, *hd );
    }

    table_->resizeColumnsToContents();
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setColumnStretchable( sDataRootCol, true );
}


void uiBatchHostsDlg::addHostCB( CallBacker* )
{
    HostData* hd = new HostData( 0 );
    hostdatalist_ += hd;
    fillTable();
    table_->selectRow( hostdatalist_.size()-1 );
    table_->setSelectionMode( uiTable::NoSelection );
    hostSelCB( 0 );
}


void uiBatchHostsDlg::rmHostCB( CallBacker* )
{
    if ( hostdatalist_.isEmpty() ) return;

    const int row = table_->currentRow();
    const BufferString hostname = table_->text( RowCol(row,1) );
    uiString msgtxt;
    if ( !hostname.isEmpty() )
	msgtxt = toUiString( hostname ).quote( true );
    else
	msgtxt = uiStrings::sHost().withNumber( row );

    const uiString msg(tr("%1 will be removed from this list").arg(msgtxt));
    if ( !uiMSG().askContinue( msg ) )
	return;

    table_->removeRow( row );
    delete hostdatalist_.removeSingle( row );

    const int lastrow = hostdatalist_.size()-1;
    table_->selectRow( row>=lastrow ? row-1 : row );
}


void uiBatchHostsDlg::moveUpCB( CallBacker* )
{
    const int row = table_->currentRow();
    if ( row==0 || !hostdatalist_.validIdx(row) ) return;

    hostdatalist_.swap( row, row-1 );
    fillTable();
    table_->selectRow( row-1 );
}


void uiBatchHostsDlg::moveDownCB( CallBacker* )
{
    const int row = table_->currentRow();
    if ( row==hostdatalist_.size()-1 ||
	 !hostdatalist_.validIdx(row) ) return;

    hostdatalist_.swap( row, row+1 );
    fillTable();
    table_->selectRow( row+1 );
}


void uiBatchHostsDlg::testHostsCB( CallBacker* )
{
    uiStringSet msgs;
    for ( int idx=0; idx<hostdatalist_.size(); idx++ )
    {
	const char* hostname = hostdatalist_[idx]->getHostName();
	BufferString msg;
	System::lookupHost( hostname, &msg );
	msgs.add( mToUiStringTodo(msg) );
    }

    if ( !msgs.isEmpty() )
	uiMSG().info( msgs );
}


static Color getColor( bool sel )
{
    mDefineStaticLocalObject( Color, bgcol, = uiMain::theMain().windowColor() );
    mDefineStaticLocalObject( Color, selcol, = bgcol.darker(0.3f) );
    return sel ? selcol : bgcol;
}


void uiBatchHostsDlg::hostSelCB( CallBacker* )
{
    const int row = table_->currentRow();
    upbut_->setSensitive( row>0 );
    downbut_->setSensitive( row!=hostdatalist_.size()-1 );

    for ( int idx=0; idx<table_->nrRows(); idx++ )
	table_->setHeaderBackground( idx, getColor(idx==row), true );
}


void uiBatchHostsDlg::changedCB( CallBacker* )
{
    const RowCol rc = table_->notifiedCell();
    const int row = rc.row();
    const int col = rc.col();
    if ( !hostdatalist_.validIdx(row) )
	return;

    NotifyStopper ns( table_->valueChanged );

    if ( col==sIPCol )
	ipAddressChanged( row );
    else if ( col==sHostNameCol )
	hostNameChanged( row );
    else if ( col==sDispNameCol )
	displayNameChanged( row );
    else if ( col==sPlfCol )
	platformChanged( row );
    else if ( col==sDataRootCol )
	dataRootChanged( row );
}


static Color getCellColor( bool isok )
{
    mDefineStaticLocalObject( Color, okcol, = Color::White() );
    mDefineStaticLocalObject( Color, errorcol, = Color::Red() );
    return isok ? okcol : errorcol;
}


void uiBatchHostsDlg::checkIPAddress( int row )
{
    HostData& hd = *hostdatalist_[row];
    const RowCol curcell = RowCol(row,sIPCol);
    const char* ipaddress = hd.getIPAddress();
    const bool isok = System::lookupHost( ipaddress );
    table_->setCellColor( curcell, getCellColor(isok) );
}


void uiBatchHostsDlg::ipAddressChanged( int row )
{
    HostData& hd = *hostdatalist_[row];
    const RowCol curcell = RowCol(row,sIPCol);
    const BufferString ipaddress = table_->text( curcell );
    hd.setIPAddress( ipaddress );
    checkIPAddress( row );

    if ( autobox_->isChecked() )
    {
	const FixedString hostname = System::hostName( ipaddress );
	hd.setHostName( hostname );
	setHostName( *table_, row, hd );
	updateDisplayName( *table_, row, hd );
    }
}


void uiBatchHostsDlg::hostNameChanged( int row )
{
    HostData& hd = *hostdatalist_[row];
    const RowCol curcell = RowCol(row,sHostNameCol);
    const BufferString hostname = table_->text( curcell );
    hd.setHostName( hostname );

    const FixedString ipaddress = System::hostAddress( hostname );
    table_->setCellColor( curcell, getCellColor(!ipaddress.isEmpty()) );

    if ( autobox_->isChecked() )
    {
	hd.setIPAddress( ipaddress.buf() );
	setIPAddress( *table_, row, hd );
	updateDisplayName( *table_, row, hd );
    }
}


void uiBatchHostsDlg::displayNameChanged( int row )
{
    HostData& hd = *hostdatalist_[row];
    updateDisplayName( *table_, row, hd );
    const BufferString oldnm = hd.alias( 0 );
    BufferString dispnm = table_->text( RowCol(row,sDispNameCol) );
    if ( dispnm.isEmpty() )
	dispnm = table_->text( RowCol(row,sHostNameCol) );
    if ( dispnm.isEmpty() )
	dispnm = table_->rowLabel( row );

    hd.setAlias( dispnm );
    if ( oldnm != dispnm )
	setDisplayName( *table_, row, hd );
}


void uiBatchHostsDlg::platformChanged( int row )
{
    HostData& hd = *hostdatalist_[row];
    uiObject* cellobj = table_->getCellObject( RowCol(row,sPlfCol) );
    mDynamicCastGet(uiComboBox*,cb,cellobj)
    if ( !cb ) return;

    OD::Platform plf( (OD::Platform::Type)cb->getIntValue() );
    hd.setPlatform( plf );
}


void uiBatchHostsDlg::dataRootChanged( int row )
{
    HostData& hd = *hostdatalist_[row];
    hd.setDataRoot( table_->text(RowCol(row,sDataRootCol)) );
}


bool uiBatchHostsDlg::acceptOK()
{
    uiRetVal uirv = hostdatalist_.check();
    if ( uirv.isError() )
	{ uiMSG().error( uirv ); return false; }

    // TODO: Support BatchHosts file selection?
    const bool res =
	hostdatalist_.writeHostFile( hostdatalist_.getBatchHostsFilename() );
    if ( !res )
    {
	uiMSG().error(uiStrings::phrCannotWrite(tr("BatchHosts file. "
			 "Please check file permissions.")));
	return false;
    }

    return true;
}
