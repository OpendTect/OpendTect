/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          May 2002
 RCS:		$Id: uiseisfmtscale.cc,v 1.1 2002-05-24 14:39:14 bert Exp $
________________________________________________________________________

-*/

#include "uiseisfmtscale.h"
#include "datachar.h"
#include "ioman.h"
#include "iodir.h"
#include "iopar.h"
#include "scaler.h"

#include "uigeninput.h"


uiSeisFmtScale::uiSeisFmtScale( uiParent* p )
	: uiGroup(p,"Seis format and scale")
{
    imptypefld = new uiGenInput( this, "Storage",
		 StringListInpSpec(DataCharacteristics::UserTypeNames) );
    scalefld = new uiGenInput( this, "Scaling: factor/shift",
				FloatInpSpec(1), FloatInpSpec(0) );
    scalefld->attach( alignedBelow, imptypefld );

    setHAlignObj( imptypefld->uiObj() );
}


Scaler* uiSeisFmtScale::getScaler() const
{
    Scaler* sc = 0;
    if ( !scalefld->isUndef(0) && !scalefld->isUndef(1) )
    {
	sc = new LinScaler( scalefld->getValue(1), scalefld->getValue(0) );
	if ( sc->isEmpty() )
	    { delete sc; sc = 0; }
    }
    return sc;
}


int uiSeisFmtScale::getFormat() const
{
    return imptypefld->getIntValue();
}


void uiSeisFmtScale::updateIOObj( IOObj* ioobj ) const
{
    if ( !ioobj ) return;

    const int tp = getFormat();
    ioobj->pars().set( "Data storage", DataCharacteristics::UserTypeNames[tp] );
    IOM().to( ioobj->key() );
    IOM().dirPtr()->commitChanges( ioobj );
}
