/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          May 2002
 RCS:		$Id: uiseistransf.cc,v 1.26 2004-09-20 16:17:37 bert Exp $
________________________________________________________________________

-*/

#include "uiseistransf.h"
#include "uiseissubsel.h"
#include "uiseisfmtscale.h"
#include "uigeninput.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "seissingtrcproc.h"
#include "seiscbvs.h"
#include "seistrcsel.h"
#include "seisresampler.h"
#include "cubesampling.h"
#include "survinfo.h"
#include "ptrman.h"
#include "iopar.h"
#include "ioobj.h"
#include "conn.h"
#include "errh.h"


uiSeisTransfer::uiSeisTransfer( uiParent* p, bool with_format, bool fne )
	: uiGroup(p,"Seis transfer pars")
	, is2d(false)
	, issteer(false)
{
    selfld = new uiSeisSubSel( this, fne );

    scfmtfld = new uiSeisFmtScale( this, with_format );
    scfmtfld->attach( alignedBelow, selfld );

    remnullfld = new uiGenInput( this, "Null traces",
	    			BoolInpSpec("Discard","Pass") );
    remnullfld->attach( alignedBelow, scfmtfld );

    setHAlignObj( remnullfld );
    mainwin()->finaliseDone.notify( mCB(this,uiSeisTransfer,updFldsForType) );
}


void uiSeisTransfer::updateFrom( const IOObj& ioobj )
{
    setInput(ioobj);
}


void uiSeisTransfer::setInput( const IOObj& ioobj )
{
    scfmtfld->updateFrom( ioobj );
    selfld->setInput( ioobj );
    is2d = selfld->is2D();
    issteer = scfmtfld->isSteer();
}


void uiSeisTransfer::updFldsForType( CallBacker* )
{
    selfld->set2D( is2d );
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


uiSeisIOObjInfo::SpaceInfo uiSeisTransfer::spaceInfo() const
{
    int ntr = selfld->expectedNrTraces();
    uiSeisIOObjInfo::SpaceInfo si( selfld->expectedNrSamples(), ntr,
				   maxBytesPerSample() );
    if ( is2d )
	si.expectednrtrcs = ntr;

    return si;
}


bool uiSeisTransfer::removeNull() const
{
    return remnullfld->getBoolValue();
}


void uiSeisTransfer::getSelData( SeisSelData& sd ) const
{
    IOPar iop;
    selfld->fillPar( iop );
    sd.usePar( iop );
}


SeisResampler* uiSeisTransfer::getResampler() const
{
    if ( selfld->isAll() ) return 0;

    CubeSampling cs;
    selfld->getSampling( cs.hrg );
    selfld->getZRange( cs.zrg );
    return new SeisResampler( cs, is2d );
}


Executor* uiSeisTransfer::getTrcProc( const IOObj& inobj,
				      const IOObj& outobj,
				      const char* extxt,
				      const char* worktxt ) const
{
    scfmtfld->updateIOObj( const_cast<IOObj*>(&outobj) );
    SeisSelection sel; sel.key_ = inobj.key();
    getSelData( sel.seldata_ );
    SeisSingleTraceProc* stp = new SeisSingleTraceProc( sel, &outobj, extxt,
	    						worktxt );
    stp->setScaler( scfmtfld->getScaler() );
    stp->skipNullTraces( removeNull() );
    stp->setResampler( getResampler() );

    return stp;
}
