/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          May 2002
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiseistransf.cc,v 1.49 2011/04/26 13:21:30 cvsbert Exp $";

#include "uiseistransf.h"
#include "uiseissubsel.h"
#include "uiseisioobjinfo.h"
#include "uiseisfmtscale.h"
#include "uigeninput.h"
#include "uimainwin.h"
#include "uimsg.h"
#include "seissingtrcproc.h"
#include "seiscbvs.h"
#include "seisselection.h"
#include "seisresampler.h"
#include "seis2dline.h"
#include "cubesampling.h"
#include "survinfo.h"
#include "ptrman.h"
#include "iopar.h"
#include "ioobj.h"
#include "conn.h"
#include "errh.h"


uiSeisTransfer::uiSeisTransfer( uiParent* p, const uiSeisTransfer::Setup& s )
	: uiGroup(p,"Seis transfer pars")
	, setup_(s)
{
    selfld = uiSeisSubSel::get( this, setup_ );

    static const char* choices[] = { "Discard", "Pass", "Add", 0 };
    if ( !setup_.is2d_ && !setup_.isps_ && setup_.withnullfill_ )
	remnullfld = new uiGenInput( this, "Null traces",
				     StringListInpSpec(choices) );
    else
	remnullfld = new uiGenInput( this, "Null traces",
				     BoolInpSpec(true,choices[0],choices[1]) );
    remnullfld->attach( alignedBelow, selfld );

    scfmtfld = new uiSeisFmtScale( this, setup_.geomType(),
	    			   !setup_.fornewentry_ );
    scfmtfld->attach( alignedBelow, remnullfld );

    setHAlignObj( remnullfld );
}


uiSeis2DSubSel* uiSeisTransfer::selFld2D()
{
    mDynamicCastGet(uiSeis2DSubSel*,ret,selfld)
    return ret;
}


uiSeis3DSubSel* uiSeisTransfer::selFld3D()
{
    mDynamicCastGet(uiSeis3DSubSel*,ret,selfld)
    return ret;
}


void uiSeisTransfer::updateFrom( const IOObj& ioobj )
{
    setInput( ioobj );
}


void uiSeisTransfer::setInput( const IOObj& ioobj )
{
    scfmtfld->updateFrom( ioobj );
    selfld->setInput( ioobj );
}


int uiSeisTransfer::maxBytesPerSample() const
{
    DataCharacteristics dc(
	    (DataCharacteristics::UserType)scfmtfld->getFormat() );
    return (int)dc.nrBytes();
}


SeisIOObjInfo::SpaceInfo uiSeisTransfer::spaceInfo() const
{
    const int ntr = selfld->expectedNrTraces();
    SeisIOObjInfo::SpaceInfo si( selfld->expectedNrSamples(),
	    	selfld->expectedNrTraces(), maxBytesPerSample() );

    if ( setup_.is2d_ )
	si.expectednrtrcs = -1;

    return si;
}


bool uiSeisTransfer::removeNull() const
{
    return setup_.withnullfill_ ? remnullfld->getIntValue() == 0
				: remnullfld->getBoolValue();
}


bool uiSeisTransfer::fillNull() const
{
    return remnullfld->getIntValue() == 2;
}


void uiSeisTransfer::setSteering( bool yn )
{
    scfmtfld->setSteering( yn );
}


Seis::SelData* uiSeisTransfer::getSelData() const
{
    IOPar iop;
    selfld->fillPar( iop );
    return Seis::SelData::get( iop );
}


SeisResampler* uiSeisTransfer::getResampler() const
{
    if ( selfld->isAll() ) return 0;

    CubeSampling cs;
    selfld->getSampling( cs.hrg );
    selfld->getZRange( cs.zrg );
    return new SeisResampler( cs, Seis::is2D(setup_.geomType()) );
}


Executor* uiSeisTransfer::getTrcProc( const IOObj& inobj,
				      const IOObj& outobj,
				      const char* extxt,
				      const char* worktxt,
				      const char* attrnm2d,
				      const char* linenm2d ) const
{
    scfmtfld->updateIOObj( const_cast<IOObj*>(&outobj) );

    PtrMan<Seis::SelData> seldata = getSelData();
    IOPar iop;
    iop.set( "ID", inobj.key() );
    if ( seldata )
    {
	if ( linenm2d && *linenm2d )
	    seldata->lineKey().setLineName( linenm2d );
	seldata->lineKey().setAttrName( attrnm2d );
	seldata->fillPar( iop );
    }
    else if ( setup_.is2d_ )
    {
	LineKey lk( linenm2d, attrnm2d );
	iop.set( sKey::LineKey, lk );
    }

    SeisSingleTraceProc* stp = new SeisSingleTraceProc( &inobj, &outobj,
	    				extxt, &iop, worktxt );
    stp->setScaler( scfmtfld->getScaler() );
    stp->skipNullTraces( removeNull() );
    stp->fillNullTraces( fillNull() );
    stp->setResampler( getResampler() );
    stp->setExtTrcToSI( scfmtfld->extendTrcToSI() );

    return stp;
}
