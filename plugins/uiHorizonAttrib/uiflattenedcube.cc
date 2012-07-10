/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2008
________________________________________________________________________

-*/

static const char* rcsID mUnusedVar = "$Id: uiflattenedcube.cc,v 1.16 2012-07-10 08:05:28 cvskris Exp $";

#include "uiflattenedcube.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfaceposprov.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seisread.h"
#include "seiswrite.h"
#include "seisselectionimpl.h"
#include "survinfo.h"
#include "executor.h"
#include "datapointset.h"

#include "uiseissel.h"
#include "uigeninput.h"
#include "uitaskrunner.h"
#include "uilabel.h"
#include "uimsg.h"
#include "mousecursor.h"

#include <math.h>


uiWriteFlattenedCube::uiWriteFlattenedCube( uiParent* p, EM::ObjectID horid )
	: uiDialog(p,Setup("Create flattened seismics",
			    BufferString("Create seismics flattened on '",
					 getHorNm(horid),"'")
		    	  ,"104.0.10"))
	, inctio_(*mMkCtxtIOObj(SeisTrc))
	, outctio_(*mMkCtxtIOObj(SeisTrc))
	, hormid_(EM::EMM().getMultiID(horid))
    	, pp_(*new Pos::EMSurfaceProvider3D)
    	, seisselin_(0)
{
    IOPar iop;
    iop.set( IOPar::compKey(sKey::Surface(),Pos::EMSurfaceProvider::id1Key()),
	     hormid_ );
    pp_.usePar( iop );
    uiTaskRunner tr( p );
    if ( !pp_.initialize(&tr) )
    {
	new uiLabel( this, "Cannot initialize horizon" );
	return;
    }

    uiSeisSel::Setup su( Seis::Vol );
    seisselin_ = new uiSeisSel( this, inctio_, su );

    BufferString txt( SI().zIsTime() ? "Time" : "Depth", " value of horizon" );
    MouseCursorManager::setOverride( MouseCursor::Wait );
    pp_.getZRange( horzrg_ );
    MouseCursorManager::restoreOverride();
    defzval_ = horzrg_.center() * SI().zDomain().userFactor();
    defzval_ = mNINT32(defzval_);
    zvalfld_ = new uiGenInput( this, txt, FloatInpSpec(defzval_) );
    zvalfld_->attach( alignedBelow, seisselin_ );

    outctio_.ctxt.forread = false;
    seisselout_ = new uiSeisSel( this, outctio_, su );
    seisselout_->attach( alignedBelow, zvalfld_ );
}


BufferString uiWriteFlattenedCube::getHorNm( EM::ObjectID horid )
{
    MultiID mid( EM::EMM().getMultiID( horid ) );
    return EM::EMM().objectName( mid );
}


uiWriteFlattenedCube::~uiWriteFlattenedCube()
{
    delete inctio_.ioobj; delete &inctio_;
    delete outctio_.ioobj; delete &outctio_;
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiWriteFlattenedCube::acceptOK( CallBacker* )
{
    if ( !seisselin_ ) return true;

    seisselin_->commitInput();
    if ( !inctio_.ioobj )
	mErrRet("Please provide the input seismic cube")

    float zval = zvalfld_->getfValue();
    if ( mIsUdf(zval) ) zval = defzval_;
    zval /= SI().zDomain().userFactor();
    if ( !SI().zRange(false).includes(zval,false) )
	mErrRet("Please provide a Z value inside the survey Z Range")

    seisselout_->commitInput();
    if ( !outctio_.ioobj )
	mErrRet("Please enter a name for the new cube")

    return doWork( zval );
}


class uiWriteFlattenedCubeMaker : public Executor
{
public:

uiWriteFlattenedCubeMaker( SeisTrcReader& rdr, SeisTrcWriter& wrr,
			   Pos::Provider3D& pp, Interval<float> hz, float zval )
    : Executor("Create flattened cube")
    , rdr_(rdr)
    , wrr_(wrr)
    , pp_(pp)
    , msg_("Creating cube")
    , nrdone_(0)
    , totnr_(pp.estNrPos())
    , horzrg_(hz)
    , zval_(zval)
{
}

const char* message() const	{ return msg_.buf(); }
const char* nrDoneText() const	{ return "Traces written"; }
od_int64 nrDone() const		{ return nrdone_; }
od_int64 totalNr() const	{ return totnr_; }

int nextStep()
{
    if ( !rdr_.get(intrc_) )
    {
	msg_ = rdr_.errMsg();
	return msg_.isEmpty() ? Finished() : ErrorOccurred();
    }

    if ( outtrc_.size() < 1 )
    {
	outtrc_ = intrc_; // get all components + info
	const Interval<float> inzrg( intrc_.zRange() );
	const StepInterval<float> outzrg( zval_ + inzrg.start - horzrg_.stop,
					  zval_ + inzrg.stop - horzrg_.start,
					  intrc_.info().sampling.step );
	const int nrsamps = outzrg.nrSteps() + 1;
	outtrc_.reSize( nrsamps, false );
	outtrc_.info().sampling.start = outzrg.start;
    }

    const float horz = pp_.adjustedZ( intrc_.info().coord, zval_ );
    outtrc_.info().binid = intrc_.info().binid;
    outtrc_.info().coord = intrc_.info().coord;
    outtrc_.info().nr = intrc_.info().nr;
    outtrc_.info().pick = horz;
    for ( int icomp=0; icomp<outtrc_.nrComponents(); icomp++ )
    {
	for ( int isamp=0; isamp<outtrc_.size(); isamp++ )
	{
	    const float z = outtrc_.samplePos( isamp ) - zval_ + horz;
	    outtrc_.set( isamp, intrc_.getValue(z,icomp), icomp );
	}
    }

    if ( !wrr_.put(outtrc_) )
	{ msg_ = wrr_.errMsg(); return ErrorOccurred(); }

    nrdone_++;
    return MoreToDo();
}

    SeisTrcReader&	rdr_;
    SeisTrcWriter&	wrr_;
    Pos::Provider3D&	pp_;
    const float		zval_;
    Interval<float>	horzrg_;
    SeisTrc		intrc_;
    SeisTrc		outtrc_;
    int			nrdone_;
    int			totnr_;
    BufferString	msg_;
};


bool uiWriteFlattenedCube::doWork( float zval )
{
    MouseCursorManager::setOverride( MouseCursor::Wait );
    DataPointSet dps( pp_, ObjectSet<DataColDef>(), 0, true );
    const float zwdth = SI().zRange(false).width();
    const Interval<float> maxzrg( -zwdth, zwdth );
    Seis::TableSelData* tsd = new Seis::TableSelData( dps.bivSet(), &maxzrg );
    SeisTrcReader rdr( inctio_.ioobj );
    rdr.setSelData( tsd );
    SeisTrcWriter wrr( outctio_.ioobj );
    uiWriteFlattenedCubeMaker cm( rdr, wrr, pp_, horzrg_, zval );
    uiTaskRunner tr( this );
    MouseCursorManager::restoreOverride();
    return tr.execute( cm );
}
