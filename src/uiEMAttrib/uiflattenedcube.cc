/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          May 2008
________________________________________________________________________

-*/


#include "uiflattenedcube.h"

#include "emmanager.h"
#include "emhorizon3d.h"
#include "emsurfaceposprov.h"
#include "seistrctr.h"
#include "seistrc.h"
#include "seisprovider.h"
#include "seisstorer.h"
#include "seistableseldata.h"
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


uiWriteFlattenedCube::uiWriteFlattenedCube( uiParent* p, const DBKey& horid )
	: uiDialog(p,Setup(uiStrings::phrCreate(tr("flattened %2")
                  .arg(uiStrings::sSeisObjName(true, true, false)
                     .toLower())),
                 uiStrings::phrCreate(tr("Seismics flattened on '%2'")
			   .arg(getHorNm(horid)))
			  , mODHelpKey(mFlattenedCubeHelpID) ))
	, hormid_(horid)
	, pp_(*new Pos::EMSurfaceProvider3D)
	, seisselin_(0)
{
    IOPar iop;
    iop.set( IOPar::compKey(sKey::Surface(),Pos::EMSurfaceProvider::id1Key()),
	     hormid_ );
    pp_.usePar( iop );
    if ( !pp_.initialize(uiTaskRunnerProvider(p)) )
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


BufferString uiWriteFlattenedCube::getHorNm( const DBKey& horid )
{
    return EM::Hor3DMan().objectName( horid );
}


uiWriteFlattenedCube::~uiWriteFlattenedCube()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiWriteFlattenedCube::acceptOK()
{
    if ( !seisselin_ ) return true;

    const IOObj* inioobj = seisselout_->ioobj();
    if ( !inioobj )
	return false;
    const IOObj* outioobj = seisselout_->ioobj();
    if ( !outioobj )
	return false;

    float zval = zvalfld_->getFValue();
    if ( mIsUdf(zval) ) zval = defzval_;
    zval /= SI().zDomain().userFactor();
    if ( !SI().zRange().includes(zval,false) )
	mErrRet(tr("Please provide a Z value inside the survey Z Range"))

    return doWork( *inioobj, *outioobj, zval );
}


class uiWriteFlattenedCubeMaker : public Executor
{ mODTextTranslationClass(uiWriteFlattenedCubeMaker);
public:

uiWriteFlattenedCubeMaker( Seis::Provider& prov, Seis::Storer& strr,
			   Pos::Provider3D& pp, Interval<float> hz, float zval )
    : Executor("Create flattened cube")
    , prov_(prov)
    , storer_(strr)
    , pp_(pp)
    , msg_(tr("Creating cube"))
    , nrdone_(0)
    , totnr_(mCast(int,pp.estNrPos()))
    , horzrg_(hz)
    , zval_(zval)
{
}

uiString message() const	{ return msg_; }
uiString nrDoneText() const	{ return tr("Traces written"); }
od_int64 nrDone() const		{ return nrdone_; }
od_int64 totalNr() const	{ return totnr_; }

int nextStep()
{
    const uiRetVal uirv = prov_.getNext( intrc_ );
    if ( !uirv.isOK() )
    {
	if ( isFinished(uirv) )
	    return Finished();

	msg_ = uirv;
	return ErrorOccurred();
    }

    if ( outtrc_.size() < 1 )
    {
	outtrc_ = intrc_; // get all components + info
	const Interval<float> inzrg( intrc_.zRange() );
	const StepInterval<float> outzrg( zval_ + inzrg.start - horzrg_.stop,
					  zval_ + inzrg.stop - horzrg_.start,
					  intrc_.info().sampling_.step );
	const int nrsamps = outzrg.nrSteps() + 1;
	outtrc_.reSize( nrsamps, false );
	outtrc_.info().sampling_.start = outzrg.start;
    }

    const float horz = pp_.adjustedZ( intrc_.info().coord_, zval_ );
    outtrc_.info().setTrcKey( intrc_.info().trcKey() );
    outtrc_.info().coord_ = intrc_.info().coord_;
    outtrc_.info().pick_ = horz;
    for ( int icomp=0; icomp<outtrc_.nrComponents(); icomp++ )
    {
	for ( int isamp=0; isamp<outtrc_.size(); isamp++ )
	{
	    const float z = outtrc_.samplePos( isamp ) - zval_ + horz;
	    outtrc_.set( isamp, intrc_.getValue(z,icomp), icomp );
	}
    }

    msg_ = storer_.put( outtrc_ );
    nrdone_++;
    return msg_.isEmpty() ? MoreToDo() : ErrorOccurred();
}

    Seis::Provider&	prov_;
    Seis::Storer&	storer_;
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
    uiRetVal uirv;
    PtrMan<Seis::Provider> prov = Seis::Provider::create( inioobj.key(), &uirv);
    if ( !prov )
	{ errmsg_ = uirv; return false; }

    uiTaskRunnerProvider trprov( this );
    RefMan<DataPointSet> dps = new DataPointSet( pp_.is2D(), true );
    if ( !dps->extractPositions(pp_,ObjectSet<DataColDef>(),trprov) )
	return false;

    auto* tsd = new Seis::TableSelData( dps->bivSet() );
    prov->setSelData( tsd );
    const float zwdth = SI().zRange().width();
    prov->setZExtension( Interval<float>(-zwdth,zwdth) );

    Seis::Storer storer( outioobj );
    uiWriteFlattenedCubeMaker cm( *prov, storer, pp_, horzrg_, zval );
    MouseCursorManager::restoreOverride();

    return trprov.execute( cm );
}
