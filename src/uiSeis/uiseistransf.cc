/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          May 2002
 RCS:		$Id: uiseistransf.cc,v 1.20 2004-07-28 16:44:45 bert Exp $
________________________________________________________________________

-*/

#include "uiseistransf.h"
#include "uiseisfmtscale.h"
#include "uigeninput.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "seissingtrcproc.h"
#include "seiscbvs.h"
#include "cbvsreadmgr.h"
#include "binidselimpl.h"
#include "survinfo.h"
#include "ptrman.h"
#include "iopar.h"
#include "ioobj.h"
#include "conn.h"
#include "errh.h"


uiSeisTransfer::uiSeisTransfer( uiParent* p, bool with_format )
	: uiGroup(p,"Seis transfer pars")
	, is2d(false)
	, issteer(false)
{
    scfmtfld = new uiSeisFmtScale( this, with_format );

    remnullfld = new uiGenInput( this, "Null traces",
	    			BoolInpSpec("Discard","Pass") );
    remnullfld->attach( alignedBelow, scfmtfld );

    setHAlignObj( remnullfld );
    mainwin()->finaliseDone.notify( mCB(this,uiSeisTransfer,updFldsForType) );
}


void uiSeisTransfer::updateFrom( const IOObj& ioobj )
{
    set2D( SeisTrcTranslator::is2D(ioobj) );
    scfmtfld->updateFrom( ioobj );
}


void uiSeisTransfer::updFldsForType( CallBacker* )
{
    scfmtfld->setSteering( issteer );
    scfmtfld->set2D( is2d );
}


void uiSeisTransfer::setSteering( bool yn )
{
    issteer = yn;
    updFldsForType(0);
}


void uiSeisTransfer::set2D( bool yn )
{
    is2d = yn;
    updFldsForType(0);
}


int uiSeisTransfer::maxBytesPerSample() const
{
    if ( issteer ) return 4;
    DataCharacteristics dc(
	    (DataCharacteristics::UserType)scfmtfld->getFormat() );
    return (int)dc.nrBytes();
}


bool uiSeisTransfer::removeNull() const
{
    return remnullfld->getBoolValue();
}


Executor* uiSeisTransfer::getTrcProc( const SeisSelection& insel,
				      const IOObj* outobj,
				      const char* extxt,
				      const char* worktxt ) const
{
    scfmtfld->updateIOObj( const_cast<IOObj*>(outobj) );
    SeisSingleTraceProc* stp = new SeisSingleTraceProc( insel, outobj, extxt,
	    						worktxt );
    stp->setScaler( scfmtfld->getScaler() );
    stp->skipNullTraces( removeNull() );

    return stp;
}
