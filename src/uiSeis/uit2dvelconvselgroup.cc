/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nageswara
 Date:          July 2010
 RCS:           $Id: uit2dvelconvselgroup.cc,v 1.1 2010/07/08 05:54:22 cvsnageswara Exp $
________________________________________________________________________

-*/

#include "uit2dvelconvselgroup.h"

#include "uimsg.h"
#include "uiseissel.h"
#include "uiveldesc.h"

#include "ctxtioobj.h"
#include "iopar.h"
#include "multiid.h"
#include "timedepthconv.h"


uiT2DVelConvSelGroup::uiT2DVelConvSelGroup( uiParent* p )
    : uiT2DConvSelGroup( p, "Velocity T2D convolve group" )
{
    IOObjContext ctxt = uiVelSel::ioContext();
    ctxt.forread = true;
    uiSeisSel::Setup su( false, false ); su.seltxt( "Velocity cube" );
    velsel_ = new uiVelSel( this, ctxt, su, false );
}


bool uiT2DVelConvSelGroup::fillPar( IOPar& par ) const
{
    if ( !velsel_->getIOObj(true) )
    {
	uiMSG().message( "please select velocity cube" );
	return false;
    }

    MultiID mid = velsel_->getIOObj(true)->key();
    par.set( Time2DepthStretcher::sKeyVelData(), mid );

    return true;
}


bool uiT2DVelConvSelGroup::usePar( const IOPar& iop )
{
    return true;
}


void uiT2DVelConvSelGroup::initClass()
{
    uiT2DConvSelGroup::factory().addCreator( create, "Velocity" );
}
