/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          May 2002
 RCS:		$Id: uiseisfmtscale.cc,v 1.2 2002-05-30 22:09:41 bert Exp $
________________________________________________________________________

-*/

#include "uiseisfmtscale.h"
#include "uiscaler.h"
#include "datachar.h"
#include "ioman.h"
#include "iodir.h"
#include "iopar.h"

#include "uigeninput.h"


uiSeisFmtScale::uiSeisFmtScale( uiParent* p )
	: uiGroup(p,"Seis format and scale")
{
    imptypefld = new uiGenInput( this, "Storage",
		 StringListInpSpec(DataCharacteristics::UserTypeNames) );
    scalefld = new uiScaler( this, 0, true );
    scalefld->attach( alignedBelow, imptypefld );

    setHAlignObj( imptypefld->uiObj() );
}


Scaler* uiSeisFmtScale::getScaler() const
{
    return scalefld->getScaler();
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
