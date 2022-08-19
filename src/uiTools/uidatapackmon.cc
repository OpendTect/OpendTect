/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidatapackmon.h"

#include "uibutton.h"
#include "uifiledlg.h"
#include "uitextedit.h"

#include "datapack.h"
#include "filepath.h"
#include "od_ostream.h"
#include "timer.h"

#include <string>
#include <sstream>


static uiDataPackMonitor* uidpmondlg_ = nullptr;


uiDataPackMonitor::uiDataPackMonitor( uiParent* p, int repeatinsec )
    : uiDialog(p,Setup(tr("DataPack Information"),mNoDlgTitle,mNoHelpKey)
	    .applybutton(true).applytext(uiStrings::sReload()).modal(false))
    , updatetimer_(*new Timer("Update"))
    , updateinmssec_(repeatinsec*1000)
{
    setCtrlStyle( RunAndClose );
    setDeleteOnClose( true );
    setButtonText( OK, uiStrings::sSaveAs() );

    txtbr_ = new uiTextEdit( this, "DataPack manager", true );

    TypeSet<DataPackMgr::MgrID> dpmgrids;
    dpmgrids += DataPackMgr::BufID();
    dpmgrids += DataPackMgr::PointID();
    dpmgrids += DataPackMgr::SeisID();
    dpmgrids += DataPackMgr::FlatID();
    dpmgrids += DataPackMgr::SurfID();
    for ( const auto& dpmgrid : dpmgrids )
    {
	DataPackMgr& dpm = DataPackMgr::DPM( dpmgrid );
	mAttachCB( dpm.newPack, uiDataPackMonitor::refreshCB );
	mAttachCB( dpm.packToBeRemoved, uiDataPackMonitor::refreshCB );
    }

    mAttachCB( postFinalize(), uiDataPackMonitor::initDlg );
    mAttachCB( windowShown, uiDataPackMonitor::refreshCB );
    mAttachCB( updatetimer_.tick, uiDataPackMonitor::refreshCB );
    mAttachCB( applyPushed, uiDataPackMonitor::refreshCB );
    mAttachCB( objectToBeDeleted(), uiDataPackMonitor::deletedCB );
}


uiDataPackMonitor::~uiDataPackMonitor()
{
    detachAllNotifiers();
    delete &updatetimer_;
}


void uiDataPackMonitor::initDlg( CallBacker* )
{
    updatetimer_.start( updateinmssec_ );
    button( OK )->setIcon( "save" );
}


void uiDataPackMonitor::refreshCB( CallBacker* )
{
    mEnsureExecutedInMainThread( uiDataPackMonitor::refreshCB );
    if ( !canupdate_ )
	return;

    std::stringstream ss;
    od_ostream strm( ss );
    if ( !strm.isOK() )
	return;

    DataPackMgr::dumpDPMs( strm );
    const BufferString newtext( ss.str().c_str() );
    if ( newtext == txtbr_->text() )
	return;

    txtbr_->setText( newtext );
}


bool uiDataPackMonitor::acceptOK( CallBacker* )
{
    canupdate_ = false;

    const FilePath deffp( FilePath::getTempDir(), "dpacks.txt" );
    uiFileDialog dlg( this, false, deffp.fullPath(), "*.txt",
		      tr("Data pack dump") );
    if ( dlg.go() )
    {
	od_ostream strm( dlg.fileName() );
	if ( strm.isOK() )
	    strm << txtbr_->text() << od_endl;
    }

    canupdate_ = true;
    return false;
}


void uiDataPackMonitor::deletedCB( CallBacker* )
{
    uidpmondlg_ = nullptr;
}


void uiDataPackMonitor::launchFrom( uiParent* p, int refreshint )
{
    uidpmondlg_ = new uiDataPackMonitor( p, refreshint );
    uidpmondlg_->show();
}
