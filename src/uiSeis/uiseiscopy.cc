/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseiscopy.h"

#include "seiscopy.h"
#include "keystrs.h"
#include "scaler.h"

#include "uiseissel.h"
#include "uiseisioobjinfo.h"
#include "uiseislinesel.h"
#include "uiscaler.h"
#include "uitaskrunner.h"
#include "od_helpids.h"


#include "uilabel.h"

uiSeisCopyCube::uiSeisCopyCube( uiParent* p, const IOObj* startobj )
	: uiDialog(p,Setup(tr("Copy cube"),mNoDlgTitle,mTODOHelpKey))
{
    setCtrlStyle( RunAndClose );
    new uiLabel( this, "TODO: implement" );
}


uiSeisCopyCube::~uiSeisCopyCube()
{
}


bool uiSeisCopyCube::acceptOK( CallBacker* )
{
    return true;
}


uiSeisCopyLineSet::uiSeisCopyLineSet( uiParent* p, const IOObj* obj )
    : uiDialog(p,Setup("Copy 2D Seismic Data",uiStrings::sEmptyString(),
                       mODHelpKey(mSeisCopyLineSetHelpID) ))
{
    IOObjContext ioctxt = uiSeisSel::ioContext( Seis::Line, true );
    inpfld_ = new uiSeisSel( this, ioctxt, uiSeisSel::Setup(Seis::Line) );
    inpfld_->selectionDone.notify( mCB(this,uiSeisCopyLineSet,inpSel) );

    subselfld_ = new uiSeis2DMultiLineSel( this, "Select Lines to copy", true );
    subselfld_->attach( alignedBelow, inpfld_ );
    if ( obj )
    {
	inpfld_->setInput( obj->key() );
	subselfld_->setInput( obj->key() );
    }

    scalefld_ = new uiScaler( this, "Scale values", true );
    scalefld_->attach( alignedBelow, subselfld_ );

    ioctxt.forread = false;
    outpfld_ = new uiSeisSel( this, ioctxt, uiSeisSel::Setup(Seis::Line) );
    outpfld_->attach( alignedBelow, scalefld_ );
}


void uiSeisCopyLineSet::inpSel( CallBacker* )
{
    if ( inpfld_->ioobj(true) )
	subselfld_->setInput( inpfld_->key() );
}


bool uiSeisCopyLineSet::acceptOK( CallBacker* )
{
    IOPar par;
    subselfld_->fillPar( par );
    Scaler* scaler = scalefld_->getScaler();
    if ( scaler )
	par.set( sKey::Scale(), scaler->toString() );

    const IOObj* inioobj = inpfld_->ioobj();
    if ( !inioobj )
	return false;
    const IOObj* outioobj = outpfld_->ioobj();
    if ( !outioobj )
	return false;

    SeisLineSetCopier copier( *inioobj, *outioobj, par );
    uiTaskRunner dlg( this );
    return TaskRunner::execute( &dlg, copier );
}
