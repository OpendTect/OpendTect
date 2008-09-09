/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurfacedlg.cc,v 1.31 2008-09-09 17:22:03 cvsyuancheng Exp $
________________________________________________________________________

-*/

#include "uiiosurfacedlg.h"
#include "uiiosurface.h"

#include "ctxtioobj.h"
#include "emfault.h"
#include "emhorizon3d.h"
#include "emhorizon2d.h"
#include "emmanager.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "filegen.h"
#include "ioobj.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uitaskrunner.h"


uiWriteSurfaceDlg::uiWriteSurfaceDlg( uiParent* p, const EM::Surface& surf )
    : uiDialog(p,uiDialog::Setup("Output selection","","104.3.1"))
    , surface_(surf)
{
    mDynamicCastGet(const EM::Horizon3D*,hor,&surface_)
    iogrp_ = new uiSurfaceWrite( this, surface_,
	   			 uiSurfaceWrite::Setup(surface_.getTypeStr())
	   			 .withdisplayfld(true).withsubsel(true) );
}


bool uiWriteSurfaceDlg::acceptOK( CallBacker* )
{
    return iogrp_->processInput();
}


void uiWriteSurfaceDlg::getSelection( EM::SurfaceIODataSelection& sels )
{
    iogrp_->getSelection( sels );
    sels.selvalues.erase();
}


IOObj* uiWriteSurfaceDlg::ioObj() const
{
    return iogrp_->selIOObj();
}


bool uiWriteSurfaceDlg::replaceInTree() const
{
    return iogrp_->replaceInTree();
}


uiReadSurfaceDlg::uiReadSurfaceDlg( uiParent* p, const char* typ )
    : uiDialog(p,uiDialog::Setup("Input selection","","104.3.0"))
{
    iogrp_ = new uiSurfaceRead( this,
	    uiSurfaceRead::Setup(typ).withattribfld(false) );
}


bool uiReadSurfaceDlg::acceptOK( CallBacker* )
{
    return iogrp_->processInput();
}


IOObj* uiReadSurfaceDlg::ioObj() const
{
    return iogrp_->selIOObj();
}


void uiReadSurfaceDlg::getSelection( EM::SurfaceIODataSelection& sels )
{
    iogrp_->getSelection( sels );
}



uiStoreAuxData::uiStoreAuxData( uiParent* p, const EM::Horizon3D& surf )
    : uiDialog(p,uiDialog::Setup("Output selection","Specify attribute name",
				 "104.3.2"))
    , surface_(surf)
{
    attrnmfld_ = new uiGenInput( this, "Attribute" );
    attrnmfld_->setText( surface_.auxdata.auxDataName(0) );
}


const char* uiStoreAuxData::auxdataName() const
{ return attrnmfld_->text(); }


bool uiStoreAuxData::acceptOK( CallBacker* )
{
    dooverwrite_ = false;
    BufferString attrnm = attrnmfld_->text();
    const bool ispres = checkIfAlreadyPresent( attrnm.buf() );
    if ( ispres )
    {
	BufferString msg( "This surface already has an attribute called:\n" );
	msg += attrnm; msg += "\nDo you wish to overwrite this data?";
	if ( !uiMSG().askGoOn(msg) )
	    return false;
	dooverwrite_ = true;
    }

    if ( attrnm != surface_.auxdata.auxDataName(0) )
	const_cast<EM::Horizon3D&>(surface_).
	    auxdata.setAuxDataName( 0, attrnm.buf() );

    return true;
}


bool uiStoreAuxData::checkIfAlreadyPresent( const char* attrnm )
{
    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( surface_.multiID(), sd );

    bool present = false;
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
    {
	if ( *sd.valnames[idx] == attrnm )
	{
	    present = true;
	    break;
	}
    }

    return present;
}



uiCopySurface::uiCopySurface( uiParent* p, const IOObj& ioobj )
    : uiDialog(p,Setup("Copy surface","","104.0.0"))
    , ctio_(mkCtxtIOObj(ioobj))
{
    inpfld = new uiSurfaceRead( this,
	    uiSurfaceRead::Setup(ioobj.group()).withattribfld(true)
	   				       .withsubsel(true) );
    inpfld->setIOObj( ioobj.key() );

    ctio_.ctxt.forread = false;
    outfld = new uiIOObjSel( this, ctio_, "Output Horizon" );
    outfld->attach( alignedBelow, inpfld );
}


uiCopySurface::~uiCopySurface()
{
    delete ctio_.ioobj;
    delete &ctio_;
}


CtxtIOObj& uiCopySurface::mkCtxtIOObj( const IOObj& ioobj )
{
    return !strcmp(ioobj.group(),EM::Horizon3D::typeStr()) ? 
			*mMkCtxtIOObj(EMHorizon3D) : 
	 ( !strcmp(ioobj.group(),EM::Horizon2D::typeStr()) ? 
	   		*mMkCtxtIOObj(EMHorizon2D) : *mMkCtxtIOObj(EMFault) );
}


#define mErrRet(msg) { uiMSG().error(msg); return false; }

bool uiCopySurface::acceptOK( CallBacker* )
{
    if ( !inpfld->processInput() ) return false;
    if ( !outfld->commitInput(true) )
	mErrRet("Please select output surface")

    const IOObj* ioobj = inpfld->selIOObj();
    if ( !ioobj ) mErrRet("Cannot find surface")

    EM::SurfaceIOData sd;
    EM::SurfaceIODataSelection sdsel( sd );
    inpfld->getSelection( sdsel );

    RefMan<EM::EMObject> emobj = EM::EMM().createTempObject( ioobj->group() );
    if ( !emobj ) mErrRet("Cannot create object")
    emobj->setMultiID( ioobj->key() );

    mDynamicCastGet(EM::Surface*,surface,emobj.ptr())
    PtrMan<Executor> loader = surface->geometry().loader( &sdsel );
    if ( !loader ) mErrRet("Cannot read surface")

    uiTaskRunner loaddlg( this );
    if ( !loaddlg.execute(*loader) ) return false;

    IOObj* newioobj = outfld->ctxtIOObj().ioobj;
    const MultiID& mid = newioobj->key();
    emobj->setMultiID( mid );
    PtrMan<Executor> saver = surface->geometry().saver( 0, &mid );
    if ( !saver ) mErrRet("Cannot save surface")

    uiTaskRunner savedlg( this );
    if ( !savedlg.execute(*saver) ) return false;

    const BufferString oldsetupname = EM::Surface::getSetupFileName( *ioobj );
    const BufferString newsetupname = EM::Surface::getSetupFileName( *newioobj);
    if ( File_exists(oldsetupname) ) 
	File_copy( oldsetupname, newsetupname, false );

    return true;
}
