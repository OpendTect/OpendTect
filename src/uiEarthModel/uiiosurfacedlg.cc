/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurfacedlg.cc,v 1.11 2003-12-18 12:45:10 nanne Exp $
________________________________________________________________________

-*/

#include "uiiosurfacedlg.h"
#include "uiiosurface.h"

#include "emsurfaceiodata.h"
#include "uimsg.h"
#include "ioobj.h"
#include "emsurface.h"
#include "emhorizon.h"
#include "emmanager.h"


uiWriteSurfaceDlg::uiWriteSurfaceDlg( uiParent* p, const EM::Surface& surf_ )
    : uiDialog(p,uiDialog::Setup("Output selection","","104.3.1"))
    , surf(surf_)
    , auxdataidx(-1)
{
    mDynamicCastGet(const EM::Horizon*,hor,&surf)
    iogrp = new uiSurfaceWrite( this, surf, (bool)hor );
}


bool uiWriteSurfaceDlg::acceptOK( CallBacker* )
{
    if ( !iogrp->processInput() )
	return false;

    if ( !auxDataOnly() )
	return true;

    BufferString attrnm = iogrp->auxDataName();
    bool rv = checkIfAlreadyPresent( attrnm.buf() );
    BufferString msg( "This surface already has an attribute called:\n" );
    msg += attrnm;
    msg += "\nDo you wish to overwrite this data?";
    if ( rv && !uiMSG().askGoOn(msg) )
	return false;

    if ( attrnm != surf.auxDataName(0) )
	const_cast<EM::Surface&>(surf).setAuxDataName( 0, attrnm.buf() );

    return true;
}


void uiWriteSurfaceDlg::getSelection( EM::SurfaceIODataSelection& sels )
{
    iogrp->getSelection( sels );
    sels.selvalues.erase();
    if ( auxDataOnly() )
	sels.selvalues += auxdataidx;
    else if ( surfaceAndData() )
	sels.selvalues += 0;
}


bool uiWriteSurfaceDlg::surfaceOnly() const
{
    return iogrp->surfaceOnly();
}


bool uiWriteSurfaceDlg::auxDataOnly() const
{
    return iogrp->saveAuxDataOnly();
}


bool uiWriteSurfaceDlg::surfaceAndData() const
{
    return iogrp->surfaceAndData();
}


IOObj* uiWriteSurfaceDlg::ioObj() const
{
    return iogrp->selIOObj();
}


bool uiWriteSurfaceDlg::checkIfAlreadyPresent( const char* attrnm )
{
    EM::SurfaceIOData sd;
    EM::EMM().getSurfaceData( surf.multiID(), sd );

    bool present = false;
    auxdataidx = -1;
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
    {
	if ( *sd.valnames[idx] == attrnm )
	{
	    present = true;
	    auxdataidx = idx;
	}
    }

    return present;
}



uiReadSurfaceDlg::uiReadSurfaceDlg( uiParent* p, bool ishor )
    : uiDialog(p,uiDialog::Setup("Input selection","","104.3.0"))
{
    iogrp = new uiSurfaceRead( this, ishor, false );
}


bool uiReadSurfaceDlg::acceptOK( CallBacker* )
{
    return iogrp->processInput();
}


IOObj* uiReadSurfaceDlg::ioObj() const
{
    return iogrp->selIOObj();
}


void uiReadSurfaceDlg::getSelection( EM::SurfaceIODataSelection& sels )
{
    iogrp->getSelection( sels );
}
