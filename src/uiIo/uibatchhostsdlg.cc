/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uibatchhostsdlg.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistatusbutton.h"
#include "uitable.h"
#include "uitextedit.h"
#include "uitoolbutton.h"

#include "file.h"
#include "hostdata.h"
#include "mmpserverclient.h"
#include "od_helpids.h"
#include "remjobexec.h"
#include "systeminfo.h"

#include <limits>

static const int sModeCol	= 0;
static const int sIPCol		= 1;
static const int sHostNameCol	= 2;
static const int sDispNameCol	= 3;
static const int sPlfCol	= 4;
static const int sStatusCol	= 5;
static const int sDataRootCol	= 6;

mDefineEnumUtils(uiBatchHostsDlg,HostLookupMode,"Host resolution")
    { "Static IP", "Hostname DNS", nullptr };


mDefineEnumUtils(uiBatchHostsDlg,Status,"Status")
{
    "Unknown",
    "OK",
    "Error",
    nullptr
};


static const char* sIconNames[] =
{
    "contexthelp",
    "ok",
    "cancel",
    0
};


template<>
void EnumDefImpl<uiBatchHostsDlg::Status>::init()
{
    uistrings_ += uiStrings::sUnknown();
    uistrings_ += uiStrings::sOk();
    uistrings_ += uiStrings::sErrors();
}


uiBatchHostsDlg::uiBatchHostsDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Setup Distributed Computing"),mNoDlgTitle,
		       mODHelpKey(mBatchHostsDlgHelpID)))
    , hostdatalist_(*new HostDataList(true))
{
    const char* bhfnm = hostdatalist_.getBatchHostsFilename();
    const FilePath bhfp = bhfnm;
    const BufferString datadir = bhfp.pathOnly();
    hoststatus_.setSize( hostdatalist_.size(), Unknown );

    const bool direxists = File::exists( datadir );
    const bool diriswritable = File::isWritable( datadir );
    const bool fileexists = File::exists( bhfnm );
    const bool fileiswritable = File::isWritable( bhfnm );
    readonly_ =  !direxists || !diriswritable ||
		(fileexists && !fileiswritable);
    if ( readonly_ )
	setCtrlStyle( CloseOnly );
    else
	setOkText( uiStrings::sSave() );

    auto* filefld = new uiGenInput( this, tr("BatchHosts file") );
    filefld->setElemSzPol( uiObject::WideMax );
    filefld->setFilename( bhfnm );
    filefld->setReadOnly();

    auto* advbut = new uiPushButton( this, tr("Advanced Settings"), false );
    mAttachCB( advbut->activated, uiBatchHostsDlg::advbutCB );
    advbut->setIcon( "settings" );
    advbut->attach( rightTo, filefld );
    advbut->attach( rightBorder );

    const uiTable::SelectionMode selmode =
		readonly_ ? uiTable::NoSelection : uiTable::Multi;
    uiTable::Setup tsu( -1, 6 );
    tsu.rowdesc(uiStrings::sHost()).defrowlbl(true).selmode(selmode);
    table_ = new uiTable( this, tsu, "Batch Hosts" );
    uiStringSet collbls;
    collbls.add( tr("Host Lookup Mode") ).add( tr("IP address") )
	.add( uiStrings::sHostName() ).add( tr("Display Name") )
	.add( uiStrings::sPlatform() ).add( uiStrings::sStatus() )
	.add( tr("Survey Data Root") );
    table_->setColumnLabels( collbls );
    table_->setPrefWidth( 800 );
    table_->resizeHeaderToContents( true );
    table_->setTableReadOnly( readonly_ );
    table_->setSelectionBehavior( uiTable::SelectRows );
    mAttachCB( table_->valueChanged, uiBatchHostsDlg::changedCB );
    mAttachCB( table_->rowClicked, uiBatchHostsDlg::hostSelCB );
    mAttachCB( table_->selectionChanged, uiBatchHostsDlg::hostSelCB );
    table_->attach( leftAlignedBelow, filefld );

    autohostbox_ = new uiCheckBox( this,
	tr("Automatically fill in IP address or Hostname") );
    autohostbox_->setChecked( true );
    autohostbox_->attach( alignedBelow, table_ );
    autohostbox_->setSensitive( !readonly_ );

    autoinfobox_ = new uiCheckBox( this,
	tr("Automatically fill in Platform and Data Root") );
    autoinfobox_->setChecked( false );
    autoinfobox_->attach( rightOf, autohostbox_ );
    autoinfobox_->setSensitive( !readonly_ );

    auto* buttons = new uiButtonGroup( this, "", OD::Vertical );
    new uiToolButton( buttons, "addnew",
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
    buttons->setChildrenSensitive( !readonly_ );
    testbut->setSensitive( true );
    autohostbox_->attach( ensureBelow, buttons );

    if ( readonly_ )
    {
	auto* sep = new uiSeparator( this );
	sep->attach( stretchedBelow, autohostbox_ );
	auto* infofld = new uiTextEdit( this, "ReadOnly Text", true );
	infofld->attach( ensureBelow, sep );

	uiString infotxt = tr("BatchHost file is read-only.\n"
		"Contact your system administrator when changes are required.");
	int prefh = 3;
	if ( __iswin__ )
	{
	    infotxt.append(
		"\n\nTo enable editing, start the program "
		"'Setup Distributed Computing' from the Start Menu with "
		"administrator rights." );
	    prefh += 3;
	}

	infofld->setPrefHeightInChar( prefh );
	infofld->setText( infotxt );
    }

    mAttachCB(afterPopup, uiBatchHostsDlg::initUI);
}


uiBatchHostsDlg::~uiBatchHostsDlg()
{
    detachAllNotifiers();
    delete &hostdatalist_;
}


void uiBatchHostsDlg::initUI( CallBacker* )
{
    uiUserShowWait usw( this, tr("Loading remote hosts") );
    fillTable();
    hostSelCB( nullptr );
}


void uiBatchHostsDlg::advbutCB( CallBacker* )
{
    uiDialog dlg( this,
	uiDialog::Setup(tr("Advanced Settings"),mNoDlgTitle,mNoHelpKey) );

    if ( readonly_ )
	dlg.setCtrlStyle( CloseOnly );

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

    dlg.getDlgGroup()->setSensitive( !readonly_ );
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


static OD::Color getCellColor( bool isok, bool readonly )
{
    mDefineStaticLocalObject( OD::Color, okcol, = OD::Color::White() );
    mDefineStaticLocalObject( OD::Color, rocol, = OD::Color(230,230,230) )
    mDefineStaticLocalObject( OD::Color, errorcol, = OD::Color::Red() );
    return isok ? (readonly ? rocol : okcol) : errorcol;
}


static void setLookupMode( uiTable& tbl, int row, const HostData& hd, bool ro )
{
    uiObject* cellobj = tbl.getCellObject( RowCol(row,sModeCol) );
    mDynamicCastGet(uiComboBox*,cb,cellobj)
    if ( !cb )
    {
	cb = new uiComboBox( nullptr,
			uiBatchHostsDlg::HostLookupModeDef().strings(), "mode");
	cb->setSensitive( !ro );
	tbl.setCellObject( RowCol(row,sModeCol), cb );
    }

    if ( hd.isStaticIP() )
	cb->setCurrentItem( uiBatchHostsDlg::StaticIP );
    else
	cb->setCurrentItem( uiBatchHostsDlg::NameDNS );
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


static void setDisplayName( uiTable& tbl, int row, const HostData& hd, bool ro )
{
    const char* nm = hd.nrAliases()>0 ? hd.alias(0) : hd.getHostName();
    tbl.setText( RowCol(row,sDispNameCol), nm );
    tbl.setColor( RowCol(row,sDispNameCol), getCellColor(true,ro) );
}


static void setPlatform( uiTable& tbl, int row, const HostData& hd, bool ro )
{
    uiObject* cellobj = tbl.getCellObject( RowCol(row,sPlfCol) );
    mDynamicCastGet(uiComboBox*,cb,cellobj)
    if ( !cb )
    {
	cb = new uiComboBox( nullptr, OD::Platform::TypeNames(), "Platforms" );
	cb->setSensitive( !ro );
	tbl.setCellObject( RowCol(row,sPlfCol), cb );
    }

    cb->setValue( hd.getPlatform().type() );
}


static void setStatus( uiTable& tbl, int row, uiBatchHostsDlg::Status status,
		       const uiPhraseSet& msg=uiPhraseSet() )
{
    uiObject* cellobj = tbl.getCellObject( RowCol(row,sStatusCol) );
    mDynamicCastGet(uiStatusButton*,sb,cellobj)
    if ( !sb )
    {
	sb = new uiStatusButton( nullptr, uiBatchHostsDlg::StatusDef(),
				 sIconNames, 0 );
	sb->setMaximumWidth( 70 );
	tbl.setCellObject( RowCol(row,sStatusCol), sb );
    }

    sb->setValue( status, msg );
}


static void setDataRoot( uiTable& tbl, int row, const HostData& hd, bool ro )
{
    const BufferString dataroot = hd.getDataRoot().fullPath();
    tbl.setText( RowCol(row,sDataRootCol), dataroot );
    tbl.setColor( RowCol(row,sDataRootCol), getCellColor(true,ro) );
}


static void updateDisplayName( uiTable& tbl, int row, HostData& hd, bool ro )
{
    BufferString dispnm = tbl.text( RowCol(row,sDispNameCol) );
    if ( dispnm.isEmpty() )
	dispnm = tbl.text( RowCol(row,sHostNameCol) );
    if ( dispnm.isEmpty() )
	dispnm = tbl.rowLabel( row );

    hd.setAlias( dispnm );
    setDisplayName( tbl, row, hd, ro );
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
	setLookupMode( *table_, idx, *hd, readonly_ );
	setHostName( *table_, idx, *hd );
	setIPAddress( *table_, idx, *hd );
	checkHostData( idx );
	setDisplayName( *table_, idx, *hd, readonly_ );
	setPlatform( *table_, idx, *hd, readonly_ );
	setStatus( *table_, idx,
		   hoststatus_.validIdx(idx) ? hoststatus_[idx] : Unknown );
	setDataRoot( *table_, idx, *hd, readonly_ );
    }

    table_->resizeColumnsToContents();
    table_->setColumnResizeMode( uiTable::ResizeToContents );
    table_->setColumnStretchable( sDataRootCol, true );
}


void uiBatchHostsDlg::addHostCB( CallBacker* )
{
    auto* hd = new HostData( nullptr );
    hostdatalist_ += hd;
    hoststatus_ += Unknown;
    fillTable();
    table_->selectRow( hostdatalist_.size()-1 );
    hostSelCB( nullptr );
}


void uiBatchHostsDlg::rmHostCB( CallBacker* )
{
    if ( hostdatalist_.isEmpty() )
	return;

    TypeSet<int> selrows;
    table_->getSelectedRows( selrows );
    if ( selrows.isEmpty() )
	return;

    ObjectSet<HostData> hosts2rm;
    uiString msg =
		tr("The following hosts will be removed from this list:\n\n");
    for ( const auto& row : selrows )
    {
	BufferString hostname = table_->text( RowCol(row,sHostNameCol) );
	if ( hostname.isEmpty() ) // Shouldn't be empty, but just in case
	    hostname = table_->text( RowCol(row,sIPCol) );
	if ( hostname.isEmpty() )
	    hostname = table_->rowLabel( row );

	msg.append( ::toUiString(hostname.buf()) ).addNewLine();

	hosts2rm.add( hostdatalist_[row] );
    }

    const bool res = uiMSG().askContinue( msg );
    if ( !res )
	return;

    for ( auto* host : hosts2rm )
    {
	const int hostidx = hostdatalist_.indexOf( host );
	hostdatalist_.removeSingle( hostidx );
	hoststatus_.removeSingle( hostidx );
    }

    fillTable();

    const int firstrow = selrows.first();
    const int lastrow = hostdatalist_.size()-1;
    table_->selectRow( firstrow>=lastrow ? firstrow-1 : firstrow );
}


void uiBatchHostsDlg::moveUpCB( CallBacker* )
{
    const int row = table_->currentRow();
    if ( row==0 || !hostdatalist_.validIdx(row) )
	return;

    hostdatalist_.swap( row, row-1 );
    hoststatus_.swap( row, row-1 );
    fillTable();
    table_->selectRow( row-1 );
}


void uiBatchHostsDlg::moveDownCB( CallBacker* )
{
    const int row = table_->currentRow();
    if ( row==hostdatalist_.size()-1 || !hostdatalist_.validIdx(row) )
	return;

    hostdatalist_.swap( row, row+1 );
    hoststatus_.swap( row, row+1 );
    fillTable();
    table_->selectRow( row+1 );
}


void uiBatchHostsDlg::testHostsCB( CallBacker* )
{
    uiRetVal uirv;
    uiUserShowWait usw( this, tr("Testing remote hosts") );
    for ( int idx=0; idx<hostdatalist_.size(); idx++ )
    {
	auto* hd = hostdatalist_[idx];
	uiString errmsg;
	if ( !hd->isOK(errmsg) )
	{
	    uirv.add( errmsg );
	    setStatus( *table_, idx, Unknown, uiPhraseSet(errmsg) );
	    continue;
	}

	if ( autohostbox_->isChecked() )
	{
	    setHostName( *table_, idx, *hd );
	    setIPAddress( *table_, idx, *hd );
	}

	const BufferString remhostaddress( hd->connAddress() );
	const Network::Authority auth( remhostaddress,
				       RemoteJobExec::remoteHandlerPort() );
	MMPServerClient mmpserver( auth );
	if ( mmpserver.isOK() )
	{
	    const Network::Service& serv = mmpserver.serverService();
	    BufferString plfname( mmpserver.serverPlatform().longName() );
	    BufferString authstr( serv.getAuthority().toString() );
	    BufferString drstr( mmpserver.serverDataRoot() );
	    BufferString defdrstr( hd->getDataRoot().fullPath() );
	    if ( autoinfobox_->isChecked() && !readonly_ )
	    {
		hd->setPlatform( mmpserver.serverPlatform() );
		setPlatform( *table_, idx, *hd, readonly_ );
	    }
	    else if ( plfname!=hd->getPlatform().longName() )
	    {
		uiRetVal msg( tr("%1 - platform name conflict: %2 vs %3")
			    .arg(authstr).arg(plfname)
			    .arg(hd->getPlatform().longName()) );
		uirv.add( msg );
		setStatus( *table_, idx, Error, msg );
		continue;
	    }

	    if ( drstr.isEmpty() )
	    {
		uiRetVal msg( tr("%1 - data root not set").arg(authstr) );
		uirv.add( msg );
		setStatus( *table_, idx, Error, msg );
		continue;
	    }
	    else if ( autoinfobox_->isChecked() && !readonly_ )
	    {
		hd->setDataRoot( FilePath(drstr) );
		setDataRoot( *table_, idx, *hd, readonly_ );
	    }
	    else if ( drstr!=defdrstr &&
				     !mmpserver.validServerDataRoot(defdrstr) )
	    {
		uiRetVal msg( tr("%1 - default data root not present")
								.arg(authstr) );
		uirv.add( msg );
		setStatus( *table_, idx, Error, msg );
		continue;
	    }

	    BufferString infostr;
	    infostr.add("Authority:").addTab().add(authstr).addNewLine();
	    infostr.add("PID:").addTab().add(serv.PID()).addNewLine();
	    infostr.add("Platform:").addTab().add(plfname).addNewLine();
	    infostr.add("ODVersion:").addTab();
	    infostr.add(mmpserver.serverODVer()).addNewLine();
	    infostr.add("Data Root:").addTab();
	    infostr.add(mmpserver.serverDataRoot()).addNewLine();
	    setStatus( *table_, idx, OK, uiPhraseSet(::toUiString(infostr)) );
	}
	else
	{
	    uirv.add( mmpserver.errMsg() );
	    setStatus( *table_, idx, Error, mmpserver.errMsg() );
	}
    }

    if ( !uirv.isOK() )
	uiMSG().errorWithDetails( uirv,
			uiStrings::phrCheck(uiStrings::sSettings()) );
}


void uiBatchHostsDlg::hostSelCB( CallBacker* )
{
    if ( readonly_ )
	return;

    const int row = table_->currentRow();
    upbut_->setSensitive( row>0 );
    downbut_->setSensitive( row!=hostdatalist_.size()-1 );
    removebut_->setSensitive( hostdatalist_.validIdx(row) );
}


void uiBatchHostsDlg::changedCB( CallBacker* )
{
    const RowCol rc = table_->notifiedCell();
    const int row = rc.row();
    const int col = rc.col();
    if ( !hostdatalist_.validIdx(row) )
	return;

    NotifyStopper ns( table_->valueChanged );

    if ( col==sModeCol )
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


void uiBatchHostsDlg::checkHostData( int row )
{
    HostData& hd = *hostdatalist_[row];
    uiString errmsg;
    const bool isok = hd.isOK( errmsg );
    const bool isstaticip = hd.isStaticIP();
    table_->setCellReadOnly( RowCol(row,sIPCol), !isstaticip );
    table_->setCellReadOnly( RowCol(row,sHostNameCol), isstaticip );

    table_->setColor( RowCol(row,sIPCol),
		      getCellColor(isok,readonly_||!isstaticip) );
    table_->setColor( RowCol(row,sHostNameCol),
		      getCellColor(isok,readonly_||isstaticip) );
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

    if ( autohostbox_->isChecked() )
    {
	setHostName( *table_, row, hd );
	updateDisplayName( *table_, row, hd, readonly_ );
    }

    checkHostData( row );
}


void uiBatchHostsDlg::hostNameChanged( int row )
{
    HostData& hd = *hostdatalist_[row];
    const RowCol curcell = RowCol(row,sHostNameCol);
    const BufferString hostname = table_->text( curcell );
    hd.setHostName( hostname );
    if ( autohostbox_->isChecked() )
    {
	uiUserShowWait usw( this, tr("Lookup remote host") );
	setIPAddress( *table_, row, hd );
	updateDisplayName( *table_, row, hd, readonly_ );
    }

    checkHostData( row );
}


void uiBatchHostsDlg::displayNameChanged( int row )
{
    HostData& hd = *hostdatalist_[row];
    updateDisplayName( *table_, row, hd, readonly_ );
    const BufferString oldnm = hd.alias( 0 );
    BufferString dispnm = table_->text( RowCol(row,sDispNameCol) );
    if ( dispnm.isEmpty() )
	dispnm = table_->text( RowCol(row,sHostNameCol) );
    if ( dispnm.isEmpty() )
	dispnm = table_->rowLabel( row );

    hd.setAlias( dispnm );
    if ( oldnm != dispnm )
	setDisplayName( *table_, row, hd, readonly_ );
}


void uiBatchHostsDlg::platformChanged( int row )
{
    HostData& hd = *hostdatalist_[row];
    const uiObject* cellobj = table_->getCellObject( RowCol(row,sPlfCol) );
    mDynamicCastGet(const uiComboBox*,cb,cellobj)
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
    const uiObject* cellobj = table_->getCellObject( RowCol(row,sModeCol) );
    mDynamicCastGet(const uiComboBox*,cb,cellobj)
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
