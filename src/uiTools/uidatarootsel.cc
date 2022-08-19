/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidatarootsel.h"

#include "uibutton.h"
#include "uilineedit.h"
#include "uimsg.h"
#include "uisetdatadir.h"
#include "uitoolbutton.h"

#include "nrbytes2string.h"
#include "oddirs.h"
#include "systeminfo.h"

uiDataRootSel::uiDataRootSel( uiParent* p )
    : uiGroup(p)
    , dataroot_(GetBaseDataDir())
    , selectionChanged(this)
{
    auto* datarootbut = new uiPushButton( this, tr("Survey Data Root"), false );
    datarootbut->setIcon( "database" );
    datarootbut->attach( leftBorder );
    mAttachCB(datarootbut->activated, uiDataRootSel::dataRootSelCB);

    datarootlbl_ = new uiLineEdit( this, "Data Root Label" );
    datarootlbl_->setHSzPol( uiObject::WideMax );
    datarootlbl_->setReadOnly();
    datarootlbl_->setBackgroundColor( backgroundColor() );
    datarootlbl_->attach( rightOf, datarootbut );

    auto* infobut = new uiToolButton( this, "info", tr("Data Root Information"),
				      mCB(this,uiDataRootSel,dataRootInfoCB) );
    infobut->attach( rightTo, datarootlbl_ );

    datarootlbl_->setText( dataroot_ );
}


uiDataRootSel::~uiDataRootSel()
{
    detachAllNotifiers();
}


extern "C" { mGlobal(Basic) void SetCurBaseDataDirOverrule(const char*); }


void uiDataRootSel::dataRootSelCB( CallBacker* )
{
    uiSetDataDir dlg( this );
    if ( !dlg.go() || dataroot_ == dlg.selectedDir() )
	return;

    dataroot_ = dlg.selectedDir();
    SetCurBaseDataDirOverrule( dataroot_ );
    datarootlbl_->setText( dataroot_ );
    selectionChanged.trigger();
}


static BufferString getSizeStr( od_int64 nrb )
{
    NrBytesToStringCreator conv( nrb );
    return conv.getString( nrb, 3 );
}


void uiDataRootSel::dataRootInfoCB( CallBacker* )
{
    const BufferString fsnm = System::fileSystemName( dataroot_ );
    const BufferString fstp = System::fileSystemType( dataroot_ );
    const BufferString totalmem = getSizeStr( System::bytesTotal(dataroot_) );
    const BufferString freemem = getSizeStr( System::bytesFree(dataroot_) );
    const BufferString availmem = getSizeStr(System::bytesAvailable(dataroot_));
    uiString msg = tr("%1: %2\n%3: %4\n\n%5:\t%6\n%7:\t%8\n%9:\t%10")
	.arg(sKey::Name()).arg(fsnm)
	.arg(tr("File system")).arg(fstp)
	.arg(tr("Total disk space")).arg(totalmem)
	.arg(tr("Free disk space")).arg(freemem)
	.arg(tr("Available disk space")).arg(availmem);
    uiMSG().message( msg );
}
