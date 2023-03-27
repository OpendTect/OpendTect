/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uidialog.h"
#include "uigroup.h"

#include "uilabel.h"
#include "uilineedit.h"
#include "uimain.h"

#include "moddepmgr.h"
#include "prog.h"

#include <QApplication>
#include <QDebug>
#include <QScreen>


class uiScreenGrp : public uiGroup
{
public:
			uiScreenGrp(uiParent*,QScreen*);
protected:
    QScreen*		qscreen_;
};


#define mAddField( txt, fn ) \
{ \
    auto* le = new uiLineEdit( this, txt ); \
    if ( txt ) \
	new uiLabel( this, toUiString(txt), le ); \
    if ( attachobj ) \
	le->attach( alignedBelow, attachobj ); \
    attachobj = le; \
    \
    QString dbgstr; \
    QDebug dbg( &dbgstr ); \
    dbg << qscreen_->fn; \
    le->setText( BufferString(dbgstr) ); \
}

uiScreenGrp::uiScreenGrp( uiParent* p, QScreen* qscreen )
    : uiGroup(p,"Screen Group")
    , qscreen_(qscreen)
{
    uiObject* attachobj = nullptr;
    mAddField( "Name", name() );
    mAddField( "Manufacturer", manufacturer() );
    mAddField( "Model", model() );
    mAddField( "Serial Number", serialNumber() );
    mAddField( "Geometry", geometry() );
    mAddField( "Virtual Geometry", virtualGeometry() );
    mAddField( "Av. Geometry", availableGeometry() );
    mAddField( "Av. Virt Geometry", availableVirtualGeometry() );
    mAddField( "Size", size() );
    mAddField( "Virtual Size", virtualSize() );
    mAddField( "Av. Size", availableSize() );
    mAddField( "Av. Virt Size", availableVirtualSize() );
    mAddField( "Depth", depth() );
    mAddField( "Refresh Rate", refreshRate() );
    mAddField( "Device Pixel Ratio", devicePixelRatio() );
    mAddField( "Logical Dots per Inch", logicalDotsPerInch() );
    mAddField( "Logical Dots per Inch X", logicalDotsPerInchX() );
    mAddField( "Logical Dots per Inch Y", logicalDotsPerInchY() );
    mAddField( "Physical Dots per Inch", physicalDotsPerInch() );
    mAddField( "Physical Dots per Inch X", physicalDotsPerInchX() );
    mAddField( "Physical Dots per Inch Y", physicalDotsPerInchY() );
}


class uiScreenDlg : public uiDialog
{
public:
			uiScreenDlg(uiParent*);

protected:
};



uiScreenDlg::uiScreenDlg( uiParent* p )
    : uiDialog(p,Setup(toUiString("Layout Test"),mNoDlgTitle,mTODOHelpKey))
{
    QList<QScreen*> screens = QApplication::screens();
    uiGroup* prevgrp = nullptr;
    for ( auto* screen : screens )
    {
	auto* grp = new uiScreenGrp( this, screen );
	if ( prevgrp )
	    grp->attach( rightTo, prevgrp );

	prevgrp = grp;
    }
}



int mProgMainFnName( int argc, char** argv )
{
    mInitProg( OD::UiProgCtxt )
    SetProgramArgs( argc, argv, false );
    uiMain app( argc, argv );

    OD::ModDeps().ensureLoaded( "General" );

    PIM().loadAuto( false );
    OD::ModDeps().ensureLoaded( "uiBase" );
    PtrMan<uiDialog> mw = new uiScreenDlg( nullptr );
    app.setTopLevel( mw );
    PIM().loadAuto( true );
    mw->go();

    return app.exec();
}
