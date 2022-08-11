/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		May 2014
________________________________________________________________________

-*/



#include "uiseis2dfrom3d.h"

#include "uimsg.h"
#include "uiseissel.h"
#include "uiseissubsel.h"
#include "uitaskrunner.h"

#include "od_helpids.h"
#include "seiscube2linedata.h"


uiSeis2DFrom3D::uiSeis2DFrom3D( uiParent* p )
    : uiDialog(p,Setup(tr("Extract 2D data from 3D"),mNoDlgTitle,
		       mODHelpKey(mSeis2DExtractFrom3DHelpID)))
{
    data3dfld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Vol,true),
				uiSeisSel::Setup(Seis::Vol) );
    data3dfld_->selectionDone.notify( mCB(this,uiSeis2DFrom3D,cubeSel) );

    Seis::SelSetup ss( Seis::Line );
    ss.withoutz(true).withstep(false).multiline(true);
    subselfld_ = new uiSeis2DSubSel( this, ss );
    subselfld_->attach( alignedBelow, data3dfld_ );

    data2dfld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Line,false),
				uiSeisSel::Setup(Seis::Line) );
    data2dfld_->attach( alignedBelow, subselfld_ );
}


uiSeis2DFrom3D::~uiSeis2DFrom3D()
{
}


void uiSeis2DFrom3D::cubeSel( CallBacker* )
{
    const IOObj* ioobj = data3dfld_->ioobj( true );
    const StringView selattrnm = data2dfld_->getInput();
    if ( !ioobj || !selattrnm.isEmpty() ) return;

    const BufferString attrnm( ioobj->name(), "[2D]" );
    data2dfld_->setInputText( attrnm.buf() );
}


bool uiSeis2DFrom3D::acceptOK( CallBacker* )
{
    const IOObj* ioobj3d = data3dfld_->ioobj();
    const IOObj* ioobj2d = data2dfld_->ioobj();
    if ( !ioobj3d || !ioobj2d ) return false;

    TypeSet<Pos::GeomID> geomids;
    subselfld_->selectedGeomIDs( geomids );
    if ( geomids.isEmpty() )
    {
	uiMSG().error( tr("Please select at least one line") );
	return false;
    }

    Seis2DFrom3DExtractor extr( *ioobj3d, *ioobj2d, geomids );
    uiTaskRunner uitr( this );
    if ( !TaskRunner::execute(&uitr,extr) )
    {
	uiMSG().error( extr.uiMessage() );
	return false;
    }

    return true;
}
