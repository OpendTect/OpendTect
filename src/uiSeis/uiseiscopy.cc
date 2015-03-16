/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2014
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseiscopy.h"

#include "seiscopy.h"
#include "keystrs.h"
#include "scaler.h"
#include "ioman.h"
#include "zdomain.h"
#include "seissingtrcproc.h"

#include "uiseissel.h"
#include "uiseisioobjinfo.h"
#include "uiseislinesel.h"
#include "uiseistransf.h"
#include "uiscaler.h"
#include "uicombobox.h"
#include "uitaskrunner.h"
#include "od_helpids.h"


#include "uilabel.h"

uiSeisCopyCube::uiSeisCopyCube( uiParent* p, const IOObj* startobj )
    : uiDialog(p,Setup(tr("Copy cube"),mNoDlgTitle,
			mODHelpKey(mSeisImpCBVSHelpID)))
    , ismc_(false)
{
    setCtrlStyle( RunAndClose );

    IOObjContext inctxt( uiSeisSel::ioContext( Seis::Vol, true ) );
    uiSeisSel::Setup sssu( Seis::Vol );
    sssu.steerpol( uiSeisSel::Setup::InclSteer );

    inpfld_ = new uiSeisSel( this, inctxt, sssu );
    inpfld_->selectionDone.notify( mCB(this,uiSeisCopyCube,inpSel) );

    compfld_ = new uiLabeledComboBox( this, tr("Component(s)") );
    compfld_->attach( alignedBelow, inpfld_ );

    uiSeisTransfer::Setup sts( Seis::Vol );
    if ( startobj )
    {
	inpfld_->setInput( startobj->key() );
	SeisIOObjInfo oinf( *startobj );
	sts.zdomkey_ = oinf.zDomainDef().key();
	if ( sts.zdomkey_ != ZDomain::SI().key() )
	    inpfld_->setSensitive( false );
    }
    sts.withnullfill(true).withstep(true).onlyrange(false).fornewentry(true);
    transffld_ = new uiSeisTransfer( this, sts );
    transffld_->attach( alignedBelow, compfld_ );

    IOObjContext outctxt( uiSeisSel::ioContext( Seis::Vol, false ) );
    outfld_ = new uiSeisSel( this, outctxt, sssu );
    outfld_->attach( alignedBelow, transffld_ );

    postFinalise().notify( mCB(this,uiSeisCopyCube,inpSel) );
}


void uiSeisCopyCube::inpSel( CallBacker* cb )
{
    const IOObj* inioobj = inpfld_->ioobj( true );
    ismc_ = false;
    if ( !inioobj )
	return;

    transffld_->updateFrom( *inioobj );

    SeisIOObjInfo oinf( *inioobj );
    ismc_ = oinf.isOK() && oinf.nrComponents() > 1;
    if ( ismc_ )
    {
	BufferStringSet cnms; oinf.getComponentNames( cnms );
	compfld_->box()->setEmpty();
	compfld_->box()->addItem( "<All>" );
	compfld_->box()->addItems( cnms );
    }
    compfld_->display( ismc_ );
}


bool uiSeisCopyCube::acceptOK( CallBacker* )
{
    const IOObj* inioobj = inpfld_->ioobj();
    if ( !inioobj )
	return false;
    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
	return false;

    outioobj->pars().addFrom( inioobj->pars() );
    IOM().commitChanges( *outioobj );

    int compnr = ismc_ ? compfld_->box()->currentItem()-1 : -1;
    Executor* exec = transffld_->getTrcProc( *inioobj, *outioobj, "", "" );
    mDynamicCastGet(SeisSingleTraceProc*,stp,exec)
    SeisCubeCopier copier( stp, compnr );
    uiTaskRunner taskrunner( this );
    return taskrunner.execute( copier );
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
    uiTaskRunner taskrunner( this );
    return taskrunner.execute( copier );;
}
