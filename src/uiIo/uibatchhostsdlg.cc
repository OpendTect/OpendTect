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
#include "survinfo.h"

#include <limits>


static const int sMode		= 0;
static const int sIPCol		= 1;
static const int sHostNameCol	= 2;
static const int sDispNameCol	= 3;
static const int sPlfCol	= 4;
static const int sDataRootCol	= 5;

mDefineEnumUtils(uiBatchHostsDlg,HostLookupMode,"Host resolution")
    { "Static IP", "Hostname DNS", nullptr };

uiBatchHostsDlg::uiBatchHostsDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Setup Distributed Computing"),mNoDlgTitle,
		       mODHelpKey(mBatchHostsDlgHelpID)))
    , hostdatalist_(*new HostDataList(true))
{
    const FilePath bhfp = hostdatalist_.getBatchHostsFilename();
    const BufferString bhfnm = bhfp.fullPath();
    BufferString datadir = bhfp.pathOnly();

    const bool direxists = File::exists( datadir );
    const bool diriswritable = File::isWritable( datadir );
    const bool fileexists = File::exists( bhfnm );
    const bool fileiswritable = File::isWritable( bhfnm );

    const bool writeallowed = direxists && diriswritable &&
				( !fileexists || fileiswritable );

    if ( writeallowed )
	setOkText( uiStrings::sSave() );
    else
	setCtrlStyle( CloseOnly );

    auto* filefld = new uiGenInput( this, tr("BatchHosts file") );
    filefld->setElemSzPol( uiObject::WideMax );
    filefld->setText( FilePath::getLongPath(bhfnm.buf()) );
    filefld->setReadOnly();

    auto* advbut = new uiPushButton( this, tr("Advanced Settings"), false );
    mAttachCB( advbut->activated, uiBatchHostsDlg::advbutCB );
    advbut->setIcon( "settings" );
    advbut->attach( rightTo, filefld );
    advbut->attach( rightBorder );

    uiTable::Setup tsu( -1, 6 );
    tsu.rowdesc(uiStrings::sHost()).defrowlbl(true).selmode(uiTable::SingleRow);
    table_ = new uiTable( this, tsu, "Batch Hosts" );
    uiStringSet collbls;
    collbls.add( tr("Host Lookup Mode") ).add( tr("IP address") )
	.add( uiStrings::sHostName() ).add( tr("Display Name") )
	.add( uiStrings::sPlatform() ).add( tr("Survey Data Root") );
    table_->setColumnLabels( collbls );
    table_->setPrefWidth( 800 );
    table_->resizeHeaderToContents( true );
    table_->setTableReadOnly( !writeallowed );
    table_->setSelectionMode( uiTable::NoSelection );
    mAttachCB( table_->valueChanged, uiBatchHostsDlg::changedCB );
    mAttachCB( table_->rowClicked, uiBatchHostsDlg::hostSelCB );
    table_->attach( leftAlignedBelow, filefld );

    autobox_ = new uiCheckBox( this,
	tr("Automatically fill in IP address or Hostname") );
    autobox_->setChecked( true );
    autobox_->attach( alignedBelow, table_ );

    auto* buttons = new uiButtonGroup( this, "", OD::Vertical );
    auto* addbut = new uiToolButton( buttons, "addnew",
			    uiStrings::phrAdd(tr("Host")),
			    mCB(this,uiBatchHostsDlg,addHostCB) );
    removebut_ = new uiToolButton( buttons, "remove",
			  uiStrings::phrRemove(tr("Host")),
			  mCB(this,uiBatchHostsDlg,rmHostCB) );
    upbut_ = new uiToolButton( buttons, uiToolButton::UpArrow,
			uiStrings::sMoveUp(),
			mCB(this,uiBatchHostsDlg,moveUpCB) );
    downbut_ = new uiToolButton( buttons, uiToolButton::DownArrow,
			uiStrings::sMoveDown(),
			mCB(this,uiBatchHostsDlg,moveDownCB) );
    auto* testbut = new uiToolButton( buttons, "checkgreen",
			tr("Test Hosts"),
			mCB(this,uiBatchHostsDlg,testHostsCB) );
    buttons->attach( rightTo, table_ );
    buttons->setChildrenSensitive( writeallowed );
    testbut->setSensitive( true );

    if ( !writeallowed )
    {
	addbut->setSensitive( false );
	removebut_->setSensitive( false );
	uiString errmsg = tr("Selected Batch Host %1 is not writable.")
	    .arg(diriswritable ? uiStrings::sFile() : uiStrings::sFolder());

	if ( __iswin__ )
	{
	    uiString details;
	    if ( !diriswritable )
	    {
		errmsg.append( tr("\nIt is advised to launch this process with "
				  "administrator rights") );

		FilePath fp( GetLibPlfDir() );
		fp.add( "od_BatchHosts.exe" );
		const BufferString longpath =
				FilePath::getLongPath( fp.fullPath() );
		details = tr("You can launch the process %1").arg( longpath );
	    }
	    else if ( fileexists && !fileiswritable )
	    {
		details = tr("Please change the read-write permissions of %1 "
		    "or move batchhost entry file to new editable location")
								.arg(bhfnm);
	    }

	    uiStringSet detailedmsg ( errmsg );
	    detailedmsg.add( details );

	    uiMSG().errorWithDetails( detailedmsg );
	}
	else
	    uiMSG().error( errmsg );
    }

    fillTable();
    hostSelCB( nullptr );
}


uiBatchHostsDlg::~uiBatchHostsDlg()
{
    detachAllNotifiers();
    delete &hostdatalist_;
}


void uiBatchHostsDlg::advbutCB( CallBacker* )
{
    uiDialog dlg( this,
	uiDialog::Setup(tr("Advanced Settings"),mNoDlgTitle,mNoHelpKey) );

    auto* albl = new uiLabel( &dlg, tr("Settings for all platforms:") );
    albl->attach( leftBorder );
    const PortNr_Type portnr = hostdatalist_.firstPort();
    const StepInterval<int> portrg( 1025,
			std::numeric_limits<PortNr_Type>::max(), 1 );
    auto* portnrfld = new uiGenInput( &dlg, tr("First Port"),
				      IntInpSpec(portnr,portrg) );
    portnrfld->attach( ensureBelow, albl );

    auto* sep = new uiSeparator( &dlg );
    sep->attach( stretchedBelow, portnrfld );

    auto* ulbl = new uiLabel( &dlg, tr("Settings for UNIX only:") );
    ulbl->attach( leftBorder );
    ulbl->attach( ensureBelow, sep );

    uiStringSet cmds;
    cmds += ::toUiString("ssh");
    cmds += ::toUiString("rsh");
    cmds += uiStrings::sEmptyString();
    auto* remoteshellfld = new uiGenInput( &dlg,
				tr("Remote shell command"),
				BoolInpSpec(true,cmds[0],cmds[1]) );
    remoteshellfld->setText( hostdatalist_.loginCmd() );
    remoteshellfld->attach( alignedBelow, portnrfld );
    remoteshellfld->attach( ensureBelow, ulbl );

    const StepInterval<int> nicelvlrg( -19, 19, 1 );
    const int nicelvl = hostdatalist_.niceLevel();
    auto* nicelvlfld = new uiGenInput( &dlg, tr("Nice level"),
				  IntInpSpec(nicelvl,nicelvlrg) );
    nicelvlfld->attach( alignedBelow, remoteshellfld );

    auto* sep2 = new uiSeparator( &dlg );
    sep2->attach( stretchedBelow, nicelvlfld );

    auto* drlbl = new uiLabel( &dlg, tr("Default Survey Data Root:") );
    drlbl->attach( leftBorder );
    drlbl->attach( ensureBelow, sep2 );

    auto* unixdrfld = new uiGenInput( &dlg, tr("Unix hosts") );
    unixdrfld->setText( hostdatalist_.unixDataRoot() );
    unixdrfld->setElemSzPol( uiObject::Wide );
    unixdrfld->attach( ensureBelow, drlbl );
    unixdrfld->attach( alignedBelow, nicelvlfld );

    auto* windrfld = new uiGenInput( &dlg, tr("Windows hosts") );
    windrfld->setText( hostdatalist_.winDataRoot() );
    windrfld->setElemSzPol( uiObject::Wide );
    windrfld->attach( alignedBelow, unixdrfld );

    if ( !dlg.go() )
	return;

    const bool remoteshell = remoteshellfld->getBoolValue();
    const int cmdres = remoteshell ? 0 : 1;
    hostdatalist_.setLoginCmd( cmds[cmdres].getString() );
    hostdatalist_.setNiceLevel( nicelvlfld->getIntValue() );
    hostdatalist_.setFirstPort( PortNr_Type(portnrfld->getIntValue()) );
    hostdatalist_.setUnixDataRoot( unixdrfld->text() );
    hostdatalist_.setWinDataRoot( windrfld->text() );
}


static void setIPAddress( uiTable& tbl, int row, const HostData& hd )
{
    const RowCol rc( row, sIPCol );
    tbl.setText( rc, hd.getIPAddress() );
}


static void setHostName( uiTable& tbl, int row, const HostData& hd )
{
    const RowCol rc( row, sHostNameCol );
    tbl.setText( rc, hd.getHostName() );
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
	cb = new uiComboBox( nullptr, OD::Platform::TypeNames(), "Platforms" );
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
	BufferString ipaddr = hd->getIPAddress();
	if ( !table_->getCellObject(RowCol(idx, sMode)) )
	{
	    auto* cb = new uiComboBox( nullptr,
				       HostLookupModeDef().strings(), "mode");
	    table_->setCellObject( RowCol(idx, sMode), cb );
	}

	mDynamicCastGet(uiComboBox*, lookupmode,
				table_->getCellObject( RowCol(idx, sMode) ));
	if ( hd->isStaticIP() )
	    lookupmode->setCurrentItem( StaticIP );
	else
	    lookupmode->setCurrentItem( NameDNS );

	setHostName( *table_, idx, *hd );
	setIPAddress( *table_, idx, *hd );
	checkHostData( idx );
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
    auto* hd = new HostData( nullptr );
    hostdatalist_ += hd;
    fillTable();
    table_->selectRow( hostdatalist_.size()-1 );
    table_->setSelectionMode( uiTable::NoSelection );
    hostSelCB( nullptr );
}


void uiBatchHostsDlg::rmHostCB( CallBacker* )
{
    if ( hostdatalist_.isEmpty() )
	return;

    const int row = table_->currentRow();
    const BufferString hostname = table_->text( RowCol(row,1) );
    uiString msgtxt;
    if ( !hostname.isEmpty() )
	msgtxt = tr( "Host %1" ).arg( hostname );
    else
	msgtxt = ( ::toUiString(table_->rowLabel(row)) );

    const uiString msg(tr("%1 will be removed from this list").arg(msgtxt));
    const bool res = uiMSG().askContinue( msg );
    if ( !res )
	return;

    table_->removeRow( row );
    delete hostdatalist_.removeSingle( row );

    const int lastrow = hostdatalist_.size()-1;
    table_->selectRow( row>=lastrow ? row-1 : row );
}


void uiBatchHostsDlg::moveUpCB( CallBacker* )
{
    const int row = table_->currentRow();
    if ( row==0 || !hostdatalist_.validIdx(row) )
	return;

    hostdatalist_.swap( row, row-1 );
    fillTable();
    table_->selectRow( row-1 );
}


void uiBatchHostsDlg::moveDownCB( CallBacker* )
{
    const int row = table_->currentRow();
    if ( row==hostdatalist_.size()-1 || !hostdatalist_.validIdx(row) )
	return;

    hostdatalist_.swap( row, row+1 );
    fillTable();
    table_->selectRow( row+1 );
}


void uiBatchHostsDlg::testHostsCB( CallBacker* )
{
    BufferStringSet msgs;
    for ( int idx=0; idx<hostdatalist_.size(); idx++ )
    {
	const char* hostname = hostdatalist_[idx]->getHostName();
	BufferString msg;
	System::lookupHost( hostname, &msg );
	msgs.add( msg );
    }

    const BufferString endmsg = msgs.cat();
    if ( !endmsg.isEmpty() )
	uiMSG().message( mToUiStringTodo(endmsg) );
}


static OD::Color getColor( bool sel )
{
    mDefineStaticLocalObject(
			OD::Color, bgcol, = uiMain::instance().windowColor() );
    mDefineStaticLocalObject( OD::Color, selcol, = bgcol.darker(0.3f) );
    return sel ? selcol : bgcol;
}


void uiBatchHostsDlg::hostSelCB( CallBacker* )
{
    const int row = table_->currentRow();
    upbut_->setSensitive( row>0 );
    downbut_->setSensitive( row!=hostdatalist_.size()-1 );
    removebut_->setSensitive( hostdatalist_.validIdx(row) );

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

    if ( col==sMode )
	lookupModeChanged( row );
    else if ( col==sIPCol )
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


static OD::Color getCellColor( bool isok, bool readonly )
{
    mDefineStaticLocalObject( OD::Color, okcol, = OD::Color::White() );
    mDefineStaticLocalObject( OD::Color, rocol, = OD::Color::LightGrey() );
    mDefineStaticLocalObject( OD::Color, errorcol, = OD::Color::Red() );
    return isok ? (readonly ? rocol : okcol) : errorcol;
}


void uiBatchHostsDlg::checkHostData( int row )
{
    HostData& hd = *hostdatalist_[row];
    uiString errmsg;
    const bool isok = hd.isOK( errmsg );
    const bool isstaticip = hd.isStaticIP();
    table_->setCellReadOnly( RowCol(row,sIPCol), !isstaticip );
    table_->setCellReadOnly( RowCol(row,sHostNameCol), isstaticip );

    table_->setColor( RowCol(row,sIPCol), getCellColor(isok,!isstaticip) );
    table_->setColor( RowCol(row,sHostNameCol),
			getCellColor(isok,isstaticip) );
}


void uiBatchHostsDlg::ipAddressChanged( int row )
{
    HostData& hd = *hostdatalist_[row];
    const RowCol curcell = RowCol(row,sIPCol);
    const BufferString ipaddress = table_->text( curcell );
    if ( !System::isValidIPAddress(ipaddress) )
    {
	uiMSG().error( tr("Invalid IP address") );
	return;
    }

    hd.setIPAddress( ipaddress );

    if ( autobox_->isChecked() )
    {
	setHostName( *table_, row, hd );
	updateDisplayName( *table_, row, hd );
    }

    checkHostData( row );
}


void uiBatchHostsDlg::hostNameChanged( int row )
{
    HostData& hd = *hostdatalist_[row];
    const RowCol curcell = RowCol(row,sHostNameCol);
    const BufferString hostname = table_->text( curcell );
    hd.setHostName( hostname );
    if ( autobox_->isChecked() )
    {
	setIPAddress( *table_, row, hd );
	updateDisplayName( *table_, row, hd );
    }

    checkHostData( row );
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
    if ( !cb )
	return;

    OD::Platform plf( sCast(OD::Platform::Type,cb->getIntValue()) );
    hd.setPlatform( plf );
}


void uiBatchHostsDlg::dataRootChanged( int row )
{
    HostData& hd = *hostdatalist_[row];
    hd.setDataRoot( table_->text(RowCol(row,sDataRootCol)) );
}


void uiBatchHostsDlg::lookupModeChanged( int row )
{
    HostData& hd = *hostdatalist_[row];
    mDynamicCastGet(uiComboBox*, cb, table_->getCellObject(RowCol(row, sMode)));
    if ( cb && cb->currentItem()==StaticIP )
	hd.setIPAddress( table_->text(RowCol(row,sIPCol)) );
    else
	hd.setHostName( table_->text(RowCol(row,sHostNameCol)) );

    checkHostData( row );
}


bool uiBatchHostsDlg::acceptOK( CallBacker* )
{
    uiStringSet errmsg;
    if ( !hostdatalist_.isOK(errmsg) )
    {
	uiMSG().errorWithDetails( errmsg );
	return false;
    }

    // TODO: Support BatchHosts file selection?
    const bool res =
	hostdatalist_.writeHostFile( hostdatalist_.getBatchHostsFilename() );
    if ( !res )
    {
	uiMSG().error( tr("Could not write BatchHosts file. "
			  "Please check file permissions."));
	return false;
    }

    return true;
}
