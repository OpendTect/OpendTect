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
#include "vispointset.h"


namespace visSurvey {


PointSetDisplay::PointSetDisplay()
    : VisualObjectImpl( true )
    , data_(nullptr)
    , transformation_(nullptr)
    , dpsdispprop_(nullptr)
    , pointset_( visBase::PointSet::create() )
{
   refPtr( pointset_ );
   Geometry::RangePrimitiveSet* range =
	    Geometry::RangePrimitiveSet::create();
   pointset_->addPrimitiveSet( range );
   addChild( pointset_->osgNode());
}


PointSetDisplay::~PointSetDisplay()
{
    setSceneEventCatcher( 0 );
    setDisplayTransformation( 0 );
    if ( data_ )
	data_->unRef();
    delete dpsdispprop_;

    unRefAndZeroPtr( pointset_ );

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


bool PointSetDisplay::setDataPack( DataPackID dpsid )
{
    auto data = DPM( DataPackMgr::PointID() ).get<DataPointSet>( dpsid );
    if ( !data ) return false;

    data_ = data;
    data_->ref();

    setName( data_ ? data_->name() : BufferString::empty() );

    return true;
}


class PointSetDisplayUpdater : public Executor
{ mODTextTranslationClass(PointSetDisplayUpdater)

public:
PointSetDisplayUpdater( visBase::PointSet& pointset, DataPointSet& dps,
			DataPointSetDisplayProp& dpsdispprop )
    : Executor(mFromUiStringTodo(tr("Creating Point Display in 3D Scene")))
    , pointset_(pointset)
    , data_(&dps)
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
{ return data_->size(); }

uiString uiNrDoneText() const override
{ return tr("Points done"); }

protected :

int nextStep() override
{
    if ( nrdone_ >= data_->size() )
    {
	Geometry::RangePrimitiveSet* range =
	    (Geometry::RangePrimitiveSet*) pointset_.getPrimitiveSet(0);

	if ( range )
	{
	    Interval<int> rg( 0, nrpoints_-1 );
	    range->setRange( rg );
	}
	pointset_.materialChangeCB( 0 );
	pointset_.requestSingleRedraw();
	return Finished();
    }

    DataPointSet::RowID rowid = mCast(DataPointSet::RowID,nrdone_);
    nrdone_++;
    if ( showselected_ && !data_->isSelected(rowid) )
	return MoreToDo();

    OD::Color col;
    if ( showselected_ )
    {
	int selgrp = data_->selGroup( rowid );
	col = dpsdispprop_.getColor( (float)selgrp );
    }
    else
    {
	const float val = data_->value( colid_, rowid );
	if ( mIsUdf(val) )
	    return MoreToDo();

	col = dpsdispprop_.getColor( val );
    }

    const int ptidx = pointset_.addPoint(
			    Coord3(data_->coord(rowid),data_->z(rowid)) );
    pointset_.getMaterial()->setColor( col, ptidx );
    nrpoints_++;
    return MoreToDo();
}

    visBase::PointSet&		pointset_;
    RefMan<DataPointSet>	data_;
    const int			colid_;
    const bool			showselected_;
    DataPointSetDisplayProp&	dpsdispprop_;
    od_int64			nrdone_;
    int				nrpoints_;
};


Executor* PointSetDisplay::getUpdater()
{
    if ( !pointset_ ) return 0;

    PointSetDisplayUpdater* ret = new PointSetDisplayUpdater( *pointset_,
							*data_, *dpsdispprop_ );
    return ret;
}


void PointSetDisplay::update( TaskRunner* tskr )
{
    PtrMan<Executor> updater = getUpdater();
    if ( !updater ) return;

    TaskRunner::execute( tskr, *updater );
}


class PointSetColorUpdater : public ParallelTask
{
public:
PointSetColorUpdater( visBase::PointSet& ps, DataPointSet& dps,
		      DataPointSetDisplayProp& dispprop )
    : ParallelTask("Updating Colors")
    , pointset_(ps)
    , data_(&dps)
    , dpsdispprop_(dispprop)
{}

od_int64 nrIterations() const override { return data_->size(); }

bool doWork( od_int64 start, od_int64 stop, int ) override
{
    for ( int idx=mCast(int,start); idx<=mCast(int,stop); idx++ )
    {
	const float val = data_->value( dpsdispprop_.dpsColID(), idx );
	const OD::Color col = dpsdispprop_.getColor( val );
	pointset_.getMaterial()->setColor( col, idx );
    }

    return true;
}

protected:

    visBase::PointSet&		pointset_;
    RefMan<DataPointSet>	data_;
    DataPointSetDisplayProp&	dpsdispprop_;

};

void PointSetDisplay::updateColors()
{
    if ( !pointset_ || pointset_->size() != data_->size() ) return;

    PointSetColorUpdater updater( *pointset_, *data_, *dpsdispprop_ );
    updater.execute();
    pointset_->materialChangeCB( 0 );
    requestSingleRedraw();
}


bool PointSetDisplay::removeSelections( TaskRunner* taskr )
{
    const Selector<Coord3>* selector = scene_ ? scene_->getSelector() : 0;
    if ( !selector || !selector->isOK() )
	return false;

    bool changed = false;
    for ( int idy=0; idy<pointset_->getCoordinates()->size(true); idy++ )
    {
	Coord3 pos = pointset_->getCoordinates()->getPos( idy );
	if ( selector->includes(pos) )
	{
	    DataPointSet::RowID rid = data_->find( DataPointSet::Pos(pos) );
	    if ( rid < 0 )
		continue;

	    if ( dpsdispprop_->showSelected() )
		data_->setSelected( rid, -1 );
	    else
		data_->setValue( dpsdispprop_->dpsColID(), rid, mUdf(float) );

	    changed = true;
	}
    }

    update( taskr );
    return changed;
}


void PointSetDisplay::setDisplayTransformation( const mVisTrans* nt )
{
    if ( transformation_ == nt )
	return;

    if ( transformation_ )
	transformation_->unRef();

    transformation_ = nt;
    if ( transformation_ )
	transformation_->ref();

    if ( pointset_ )
        pointset_->setDisplayTransformation( transformation_ );
}


const mVisTrans* PointSetDisplay::getDisplayTransformation() const
{ return transformation_; }


void PointSetDisplay::setPixelDensity( float dpi )
{
    VisualObjectImpl::setPixelDensity( dpi );

    if ( pointset_ )
	pointset_->setPixelDensity( dpi );
}


void PointSetDisplay::getMousePosInfo( const visBase::EventInfo& eventinfo,
				       Coord3& pos,
				       BufferString& val,
				       BufferString& info ) const
{
    info = ""; val = "";
    if ( !data_ ) return;

    info = data_->name();
    if ( !dpsdispprop_ )
	return;

    info += ": ";
    if ( dpsdispprop_->showSelected() )
	info += "Selection Group";
    else
	info += data_->colName( dpsdispprop_->dpsColID() );

    BinID binid = SI().transform( pos );
    DataPointSet::RowID rid = data_->findFirst( binid );
    if ( rid < 0 )
	return;

    if ( dpsdispprop_->showSelected() )
	val.add( data_->selGroup(rid) );
    else
	val.add( data_->value(dpsdispprop_->dpsColID(),rid) );
}

} // namespace visSurvey
