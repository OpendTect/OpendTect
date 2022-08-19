/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

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
#include "od_helpids.h"

#include <math.h>


uiWriteFlattenedCube::uiWriteFlattenedCube( uiParent* p, EM::ObjectID horid )
	: uiDialog(p,Setup(uiStrings::phrCreate(tr("flattened %2")
			   .arg(uiStrings::sSeismic().toLower())),
			   uiStrings::phrCreate(tr("Seismic flattened on '%2'")
			   .arg(mToUiStringTodo(getHorNm(horid))))
			  , mODHelpKey(mFlattenedCubeHelpID) ))
	, hormid_(EM::EMM().getMultiID(horid))
	, pp_(*new Pos::EMSurfaceProvider3D)
	, seisselin_(0)
{
    IOPar iop;
    iop.set( IOPar::compKey(sKey::Surface(),Pos::EMSurfaceProvider::id1Key()),
	     hormid_ );
    pp_.usePar( iop );
    uiTaskRunner taskrunner( p );
    if ( !pp_.initialize(&taskrunner) )
    {
	new uiLabel( this, tr("Cannot initialize %1")
					.arg(uiStrings::sHorizon().toLower()) );
	return;
    }

    uiSeisSel::Setup su( Seis::Vol );
    seisselin_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Vol,true), su);

    uiString txt = tr("%1 value of horizon")
	.arg( SI().zIsTime() ? uiStrings::sTime() : uiStrings::sDepth() );
    MouseCursorManager::setOverride( MouseCursor::Wait );
    pp_.getZRange( horzrg_ );
    MouseCursorManager::restoreOverride();
    defzval_ = horzrg_.center() * SI().zDomain().userFactor();
    defzval_ = (float) mNINT32(defzval_);
    zvalfld_ = new uiGenInput( this, txt, FloatInpSpec(defzval_) );
    zvalfld_->attach( alignedBelow, seisselin_ );

    seisselout_ = new uiSeisSel(this,uiSeisSel::ioContext(Seis::Vol,false),su);
    seisselout_->attach( alignedBelow, zvalfld_ );
}


BufferString uiWriteFlattenedCube::getHorNm( EM::ObjectID horid )
{
    MultiID mid( EM::EMM().getMultiID( horid ) );
    return EM::EMM().objectName( mid );
}


uiWriteFlattenedCube::~uiWriteFlattenedCube()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiWriteFlattenedCube::acceptOK( CallBacker* )
{
    if ( !seisselin_ ) return true;

    const IOObj* inioobj = seisselin_->ioobj();
    if ( !inioobj )
	return false;
    const IOObj* outioobj = seisselout_->ioobj();
    if ( !outioobj )
	return false;

    float zval = zvalfld_->getFValue();
    if ( mIsUdf(zval) ) zval = defzval_;
    zval /= SI().zDomain().userFactor();
    if ( !SI().zRange(false).includes(zval,false) )
	mErrRet(tr("Please provide a Z value inside the survey Z Range"))

    return doWork( *inioobj, *outioobj, zval );
}


class uiWriteFlattenedCubeMaker : public Executor
{ mODTextTranslationClass(uiWriteFlattenedCubeMaker);
public:

uiWriteFlattenedCubeMaker( SeisTrcReader& rdr, SeisTrcWriter& wrr,
			   Pos::Provider3D& pp, Interval<float> hz, float zval )
    : Executor("Create flattened cube")
    , rdr_(rdr)
    , wrr_(wrr)
    , pp_(pp)
    , msg_(tr("Creating cube"))
    , nrdone_(0)
    , totnr_(mCast(int,pp.estNrPos()))
    , horzrg_(hz)
    , zval_(zval)
{
}

uiString uiMessage() const override	{ return msg_; }
uiString uiNrDoneText() const override	{ return tr("Traces written"); }
od_int64 nrDone() const override	{ return nrdone_; }
od_int64 totalNr() const override	{ return totnr_; }

int nextStep() override
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
    outtrc_.info() = intrc_.info();
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
    uiString		msg_;
};


bool uiWriteFlattenedCube::doWork( const IOObj& inioobj, const IOObj& outioobj,
					float zval )
{
    MouseCursorManager::setOverride( MouseCursor::Wait );
    uiTaskRunner taskrunner( this );
    DataPointSet dps( pp_.is2D(), true );
    if ( !dps.extractPositions(pp_,ObjectSet<DataColDef>(),0,&taskrunner) )
	return false;

    const float zwdth = SI().zRange(false).width();
    const Interval<float> maxzrg( -zwdth, zwdth );
    auto* tsd = new Seis::TableSelData( dps.bivSet(), &maxzrg );
    SeisTrcReader rdr( inioobj );
    rdr.setSelData( tsd );
    SeisTrcWriter wrr( outioobj );
    uiWriteFlattenedCubeMaker cm( rdr, wrr, pp_, horzrg_, zval );
    MouseCursorManager::restoreOverride();
    return TaskRunner::execute( &taskrunner, cm );
}
