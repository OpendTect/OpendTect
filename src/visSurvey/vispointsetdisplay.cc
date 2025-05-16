/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "color.h"
#include "datapointset.h"
#include "executor.h"
#include "selector.h"
#include "viscoord.h"
#include "visevent.h"
#include "vismaterial.h"
#include "vispointsetdisplay.h"


namespace visSurvey {


PointSetDisplay::PointSetDisplay()
    : visBase::VisualObjectImpl(true)
{
    ref();
    pointset_ = visBase::PointSet::create();
    RefMan<Geometry::RangePrimitiveSet> range =
					Geometry::RangePrimitiveSet::create();
    pointset_->addPrimitiveSet( range.ptr() );
    addChild( pointset_->osgNode());
    unRefNoDelete();
}


PointSetDisplay::~PointSetDisplay()
{
    setSceneEventCatcher( nullptr );
    transformation_ = nullptr;
    delete dpsdispprop_;
}


void PointSetDisplay::setDispProp( const DataPointSetDisplayProp* prop )
{
    delete dpsdispprop_;
    dpsdispprop_ = prop->clone();
}


void PointSetDisplay::setPointSize( int sz )
{
    if ( pointset_ )
        pointset_->setPointSize( sz );
}

int PointSetDisplay::getPointSize() const
{ return pointset_->getPointSize(); }


bool PointSetDisplay::setDataPointSet( const DataPointSet& dpset )
{
    auto& dps = const_cast<DataPointSet&>( dpset );
    return setPointDataPack( 0, &dps, nullptr );
}


bool PointSetDisplay::setPointDataPack( int /* attrib */, PointDataPack* dp,
					TaskRunner* /* taskrun */ )
{
    mDynamicCastGet(DataPointSet*,dps,dp);
    if ( !dps )
	return false;

    datapack_ = dps;
    setName( datapack_ ? datapack_->name() : BufferString::empty() );
    return true;
}


ConstRefMan<DataPack> PointSetDisplay::getDataPack( int attrib ) const
{
    return getPointDataPack( attrib );
}


ConstRefMan<PointDataPack> PointSetDisplay::getPointDataPack( int ) const
{
    return datapack_.ptr();
}


class PointSetDisplayUpdater : public Executor
{ mODTextTranslationClass(PointSetDisplayUpdater)

public:
PointSetDisplayUpdater( visBase::PointSet& pointset, DataPointSet& dps,
			DataPointSetDisplayProp& dpsdispprop )
    : Executor("Creating Point Display in 3D Scene")
    , pointset_(pointset)
    , dps_(&dps)
    , dpsdispprop_(dpsdispprop)
    , nrdone_(0)
    , colid_(dpsdispprop.dpsColID())
    , showselected_(dpsdispprop.showSelected())
    , nrpoints_(0)
{
    pointset_.removeAllPoints();
    pointset_.getMaterial()->clear();
}

od_int64 nrDone() const override
{ return nrdone_; }

od_int64 totalNr() const override
{ return dps_->size(); }

uiString uiNrDoneText() const override
{ return tr("Points done"); }

protected :

int nextStep() override
{
    if ( nrdone_ >= dps_->size() )
    {
	RefMan<Geometry::RangePrimitiveSet> range =
	    (Geometry::RangePrimitiveSet*) pointset_.getPrimitiveSet(0);

	if ( range )
	{
	    Interval<int> rg( 0, nrpoints_-1 );
	    range->setRange( rg );
	}

	pointset_.materialChangeCB( nullptr );
	pointset_.requestSingleRedraw();
	return Finished();
    }

    DataPointSet::RowID rowid = mCast(DataPointSet::RowID,nrdone_);
    nrdone_++;
    if ( showselected_ && !dps_->isSelected(rowid) )
	return MoreToDo();

    OD::Color col;
    if ( showselected_ )
    {
	int selgrp = dps_->selGroup( rowid );
	col = dpsdispprop_.getColor( (float)selgrp );
    }
    else
    {
	const float val = dps_->value( colid_, rowid );
	if ( mIsUdf(val) )
	    return MoreToDo();

	col = dpsdispprop_.getColor( val );
    }

    const int ptidx = pointset_.addPoint(
			    Coord3(dps_->coord(rowid),dps_->z(rowid)) );
    pointset_.getMaterial()->setColor( col, ptidx );
    nrpoints_++;
    return MoreToDo();
}

    visBase::PointSet&		pointset_;
    RefMan<DataPointSet>	dps_;
    const int			colid_;
    const bool			showselected_;
    DataPointSetDisplayProp&	dpsdispprop_;
    od_int64			nrdone_;
    int				nrpoints_;
};


Executor* PointSetDisplay::getUpdater()
{
    if ( !pointset_ )
	return nullptr;

    return new PointSetDisplayUpdater( *pointset_, *datapack_.ptr(),
				       *dpsdispprop_ );
}


void PointSetDisplay::update( TaskRunner* tskr )
{
    PtrMan<Executor> updater = getUpdater();
    if ( !updater )
	return;

    TaskRunner::execute( tskr, *updater );
}


class PointSetColorUpdater : public ParallelTask
{
public:
PointSetColorUpdater( visBase::PointSet& ps, DataPointSet& dps,
		      DataPointSetDisplayProp& dispprop )
    : ParallelTask("Updating Colors")
    , pointset_(ps)
    , dps_(&dps)
    , dpsdispprop_(dispprop)
{}

od_int64 nrIterations() const override { return dps_->size(); }

bool doWork( od_int64 start, od_int64 stop, int ) override
{
    for ( int idx=mCast(int,start); idx<=mCast(int,stop); idx++ )
    {
	const float val = dps_->value( dpsdispprop_.dpsColID(), idx );
	const OD::Color col = dpsdispprop_.getColor( val );
	pointset_.getMaterial()->setColor( col, idx );
    }

    return true;
}

protected:

    visBase::PointSet&		pointset_;
    RefMan<DataPointSet>	dps_;
    DataPointSetDisplayProp&	dpsdispprop_;

};

void PointSetDisplay::updateColors()
{
    if ( !pointset_ || pointset_->size() != datapack_->size() )
	return;

    PointSetColorUpdater updater( *pointset_, *datapack_, *dpsdispprop_ );
    updater.execute();
    pointset_->materialChangeCB( nullptr );
    requestSingleRedraw();
}


bool PointSetDisplay::removeSelections( TaskRunner* taskr )
{
    const Selector<Coord3>* selector = scene_ ? scene_->getSelector() : nullptr;
    if ( !selector || !selector->isOK() )
	return false;

    bool changed = false;
    for ( int idy=0; idy<pointset_->getCoordinates()->size(true); idy++ )
    {
	Coord3 pos = pointset_->getCoordinates()->getPos( idy );
	if ( selector->includes(pos) )
	{
	    const DataPointSet::RowID rid =
				datapack_->find( DataPointSet::Pos(pos) );
	    if ( rid < 0 )
		continue;

	    if ( dpsdispprop_->showSelected() )
		datapack_->setSelected( rid, -1 );
	    else
		datapack_->setValue( dpsdispprop_->dpsColID(), rid,mUdf(float));

	    changed = true;
	}
    }

    update( taskr );
    if ( changed )
	SurveyObject::removeSelections( taskr );

    return changed;
}


void PointSetDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_.ptr() == nt )
	return;

    transformation_ = nt;
    if ( pointset_ )
	pointset_->setDisplayTransformation( transformation_.ptr() );
}


const mVisTrans* PointSetDisplay::getDisplayTransformation() const
{
    return transformation_.ptr();
}


void PointSetDisplay::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );
    if ( pointset_ )
	pointset_->setPixelDensity( dpi );
}


void PointSetDisplay::getMousePosInfo( const visBase::EventInfo& eventinfo,
				       Coord3& pos,
				       BufferString& val,
				       uiString& info ) const
{
    info.setEmpty(); val.setEmpty();
    if ( !datapack_ )
	return;

    info = toUiString( datapack_->name() );
    if ( !dpsdispprop_ )
	return;

    if ( dpsdispprop_->showSelected() )
	info = info.appendPhrase(tr("Selection Group"), uiString::MoreInfo,
	    uiString::OnSameLine );
    else
	info = info.appendPhrase(
		toUiString(datapack_->colName( dpsdispprop_->dpsColID() )),
			uiString::MoreInfo,
	    uiString::OnSameLine );

    const BinID binid = SI().transform( pos );
    const DataPointSet::RowID rid = datapack_->findFirst( binid );
    if ( rid < 0 )
	return;

    if ( dpsdispprop_->showSelected() )
	val.add( datapack_->selGroup(rid) );
    else
	val.add( datapack_->value(dpsdispprop_->dpsColID(),rid) );
}

} // namespace visSurvey
