/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uifaultsetcopy.h"

#include "uiioobjsel.h"
#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uitaskrunner.h"

#include "ctxtioobj.h"
#include "dirlist.h"
#include "emfault3d.h"
#include "emfaultset3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "filepath.h"


uiFault2FaultSet::uiFault2FaultSet( uiParent* p )
    : uiDialog(p,Setup(tr("Copy Faults to FaultSet"),mNoDlgTitle,mTODOHelpKey))
{
    IOObjContext ctxt = mIOObjContext( EMFault3D );
    infld_ = new uiIOObjSelGrp( this, ctxt, tr("Select Faults") );

    IOObjContext fsctxt = mIOObjContext( EMFaultSet3D );
    fsctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, fsctxt, tr("Output FaultSet") );
    outfld_->attach( alignedBelow, infld_ );
}


uiFault2FaultSet::~uiFault2FaultSet()
{
    detachAllNotifiers();
}


void uiFault2FaultSet::setInput( const TypeSet<MultiID>& mids )
{
    infld_->setChosen( mids );
}


MultiID uiFault2FaultSet::key() const
{
    return outfld_->key();
}


bool uiFault2FaultSet::acceptOK( CallBacker* )
{
    const int nrinp = infld_->nrChosen();
    if ( nrinp==0 )
    {
	uiMSG().error( tr("Please select at least one fault") );
	return false;
    }

    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
	return false;

    const EM::ObjectID oid = EM::EMM().createObject(
				EM::FaultSet3D::typeStr(), outioobj->name() );
    RefMan<EM::FaultSet3D> fltset;
    mDynamicCast(EM::FaultSet3D*,fltset,EM::EMM().getObject(oid))

    uiTaskRunner taskrunner( this );
    TypeSet<MultiID> inputids;

    infld_->getChosen( inputids );
    PtrMan<Executor> fltloader = EM::EMM().objectLoader( inputids );
    if ( fltloader && !TaskRunner::execute(&taskrunner,*fltloader) )
    {
	uiMSG().error( fltloader->uiMessage() );
	return false;
    }

    for ( int idx=0; idx<inputids.size(); idx++ )
    {
	const EM::ObjectID objid = EM::EMM().getObjectID( inputids[idx] );
	RefMan<EM::EMObject> emobj = EM::EMM().getObject( objid );
	mDynamicCastGet(EM::Fault3D*,f3d,emobj.ptr())
	if ( !f3d )
	    continue;

	fltset->addFault( f3d );
    }

    PtrMan<Executor> saver = fltset->saver();
    if ( saver && !TaskRunner::execute(&taskrunner,*saver) )
    {
	uiMSG().error( saver->uiMessage() );
	return false;
    }

    const uiString msg = tr("Faults successfully written to FaultSet.\n"
			    "Do you want to copy more faults?");
    const bool ret = uiMSG().askGoOn( msg, uiStrings::sYes(),
				    tr("No, close window") );
    return !ret;
}



// uiCopyFaultSet
uiCopyFaultSet::uiCopyFaultSet( uiParent* p, const IOObj& ioobj )
    : uiDialog(p,Setup(uiStrings::phrCopy(uiStrings::sFaultSet()),mNoDlgTitle,
		       mTODOHelpKey))
{
    IOObjContext ctxt = mIOObjContext(EMFaultSet3D);
    inpfld_ = new uiIOObjSel( this, ctxt,
			      uiStrings::phrInput(uiStrings::sFaultSet()) );
    inpfld_->setInput( ioobj );
    inpfld_->selectionDone.notify( mCB(this,uiCopyFaultSet,inpSelCB) );

    surflist_ = new uiListBox( this, uiListBox::Setup(OD::ChooseAtLeastOne,
			    uiStrings::phrSelect(uiStrings::sFault(mPlural))) );
    surflist_->attach( alignedBelow, inpfld_ );

    ctxt.forread_ = false;
    outfld_ = new uiIOObjSel( this, ctxt,
			      uiStrings::phrOutput(uiStrings::sFaultSet()) );
    outfld_->attach( alignedBelow, surflist_ );

    postFinalize().notify( mCB(this,uiCopyFaultSet,inpSelCB) );
}


void uiCopyFaultSet::inpSelCB( CallBacker* )
{
    const IOObj* selobj = inpfld_->ioobj( true );
    if ( !selobj )
	return;

    surflist_->setEmpty();
    DirList dl( selobj->fullUserExpr(), File::FilesInDir, "*.flt" );
    if ( dl.isEmpty() )
	return;

    BufferStringSet idstrs;
    for ( int idx=0; idx<dl.size(); idx++ )
    {
	const FilePath fp = dl.fullPath( idx );
	idstrs.add( fp.baseName() );
    }

    surflist_->addItems( idstrs );
}


bool uiCopyFaultSet::acceptOK( CallBacker* )
{
    const IOObj* inpobj = inpfld_->ioobj();
    if ( !inpobj )
	return false;

    BufferStringSet selflts;
    surflist_->getChosen( selflts );
    if ( selflts.isEmpty() )
	return false;

    const IOObj* outobj = outfld_->ioobj();
    if ( !outobj )
	return false;

    const BufferString inpdirnm = inpobj->fullUserExpr();
    const BufferString outdirnm = outobj->fullUserExpr();
    if ( !File::isDirectory(outdirnm) )
	File::createDir( outdirnm );

    BufferString errmsg;
    for ( int idx=0; idx<selflts.size(); idx++ )
    {
	FilePath inpfp( inpdirnm, selflts.get(idx) );
	inpfp.setExtension( ".flt" );
	FilePath outfp( outdirnm, toString(idx+1) );
	outfp.setExtension( ".flt" );
	if ( !File::copy(inpfp.fullPath(),outfp.fullPath(),&errmsg) )
	{
	    uiMSG().error( toUiString("%1: %2")
		    .arg(uiStrings::phrCannotCopy(uiStrings::sFault()))
		    .arg(toUiString(errmsg)) );
	    return false;
	}
    }

    return true;
}
