/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurfacedlg.cc,v 1.13 2005-04-06 10:54:30 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiiosurfacedlg.h"
#include "uiiosurface.h"

#include "emsurfaceiodata.h"
#include "emsurfaceauxdata.h"
#include "uimsg.h"
#include "ioobj.h"
#include "emsurface.h"
#include "emhorizon.h"
#include "emmanager.h"
#include "uigeninput.h"


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

