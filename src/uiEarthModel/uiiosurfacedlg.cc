/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurfacedlg.cc,v 1.15 2005-10-06 19:13:37 cvskris Exp $
________________________________________________________________________

-*/

#include "uiiosurfacedlg.h"
#include "uiiosurface.h"

#include "emsurfaceiodata.h"
#include "emsurfaceauxdata.h"
#include "emsurfacetr.h"
#include "uimsg.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "emsurface.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiexecutor.h"


uiWriteSurfaceDlg::uiWriteSurfaceDlg( uiParent* p, const EM::Surface& surf )
    : uiDialog(p,uiDialog::Setup("Output selection","","104.3.1"))
    , surface_(surf)
{
    mDynamicCastGet(const EM::Horizon*,hor,&surface_)
    iogrp_ = new uiSurfaceWrite( this, surface_, (bool)hor );
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


uiReadSurfaceDlg::uiReadSurfaceDlg( uiParent* p, bool ishor )
    : uiDialog(p,uiDialog::Setup("Input selection","","104.3.0"))
{
    iogrp_ = new uiSurfaceRead( this, ishor, false );
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



uiStoreAuxData::uiStoreAuxData( uiParent* p, const EM::Surface& surf )
    : uiDialog(p,uiDialog::Setup("Output selection","Specify attribute name",
				 "104.3.2"))
    , surface_(surf)
    , auxdataidx_(-1)
{
    attrnmfld_ = new uiGenInput( this, "Attribute" );
    attrnmfld_->setText( surface_.auxdata.auxDataName(0) );
}


bool uiStoreAuxData::acceptOK( CallBacker* )
{
    BufferString attrnm = attrnmfld_->text();
    bool rv = checkIfAlreadyPresent( attrnm.buf() );
    BufferString msg( "This surface already has an attribute called:\n" );
    msg += attrnm;
    msg += "\nDo you wish to overwrite this data?";
    if ( rv && !uiMSG().askGoOn(msg) )
	return false;

    if ( attrnm != surface_.auxdata.auxDataName(0) )
	const_cast<EM::Surface&>(surface_).
	    auxdata.setAuxDataName( 0, attrnm.buf());

    return true;
}


bool uiStoreAuxData::checkIfAlreadyPresent( const char* attrnm )
{
    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( surface_.multiID(), sd );

    bool present = false;
    auxdataidx_ = -1;
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
    {
	if ( *sd.valnames[idx] == attrnm )
	{
	    present = true;
	    auxdataidx_ = idx;
	}
    }

    return present;
}



uiCopySurface::uiCopySurface( uiParent* p, const IOObj& ioobj )
    : uiDialog(p,Setup("Copy surface","","104.0.0"))
    , ctio_(mkCtxtIOObj(ioobj))
{
    const bool ishor = !strcmp(ioobj.group(),EM::Horizon::typeStr());
    inpfld = new uiSurfaceRead( this, ishor, false );
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
    return !strcmp(ioobj.group(),EM::Horizon::typeStr())
	? *mMkCtxtIOObj(EMHorizon) : *mMkCtxtIOObj(EMFault);
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
    PtrMan<Executor> loader = surface->geometry.loader( &sdsel );
    if ( !loader ) mErrRet("Cannot read surface")

    uiExecutor loaddlg( this, *loader );
    if ( !loaddlg.go() ) return false;

    IOObj* newioobj = outfld->ctxtIOObj().ioobj;
    const MultiID& mid = newioobj->key();
    emobj->setMultiID( mid );
    PtrMan<Executor> saver = surface->geometry.saver( 0, &mid );
    if ( !saver ) mErrRet("Cannot save surface")

    uiExecutor savedlg( this, *saver );
    if ( !savedlg.go() ) return false;

    return true;
}
