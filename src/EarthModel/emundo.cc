/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "emundo.h"

#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "emmanager.h"
#include "emhorizon3d.h"

namespace EM
{

const char* SetPosUndoEvent::savedposstr_ = "Pos";

EMUndo::EMUndo()
{}


EMUndo::~EMUndo()
{}


ObjectID EMUndo::getCurrentEMObjectID( bool forredo ) const
{
    const int curidx = indexOf( forredo ? currenteventid_ + 1
					: currenteventid_ );
    if ( !events_.validIdx(curidx) )
	return ObjectID::udf();

    const UndoEvent* curev = events_[curidx];
    mDynamicCastGet( const EMUndoEvent*, emundoev, curev );
    return emundoev ? emundoev->getObjectID() : ObjectID::udf();
}



// EMUndoEvent
EMUndoEvent::EMUndoEvent()
{}


EMUndoEvent::~EMUndoEvent()
{}



// SetPosUndoEvent
SetPosUndoEvent::SetPosUndoEvent( const Coord3& oldpos,
				      const PosID& posid )
    : posid_(posid)
    , savedpos_(oldpos)
{}


SetPosUndoEvent::~SetPosUndoEvent()
{}


const char* SetPosUndoEvent::getStandardDesc() const
{ return "Set/Changed position"; }


bool SetPosUndoEvent::unDo()
{
    if ( !EMM().getObject(posid_.objectID()))
	return true;

    EMObject* emobject = EMM().getObject(posid_.objectID());
    if ( !emobject ) return false;

    const bool haschecks = emobject->enableGeometryChecks( false );

    bool res = false;
    const Coord3 proxy = emobject->getPos( posid_ );
    if ( emobject->setPos(posid_,savedpos_,false) )
    {
	savedpos_ = proxy;
	res = true;
    }

    emobject->enableGeometryChecks( haschecks );
    return res;
}


bool SetPosUndoEvent::reDo()
{
    if ( !EMM().getObject(posid_.objectID()))
	return true;

    EMObject* emobject = EMM().getObject(posid_.objectID());
    if ( !emobject ) return false;

    bool res = false;
    const bool haschecks = emobject->enableGeometryChecks(false);

    const Coord3 proxy = emobject->getPos( posid_ );
    if ( emobject->setPos( posid_, savedpos_, false ) )
    {
	savedpos_ = proxy;
	res = true;
    }

    emobject->enableGeometryChecks( haschecks );

    return res;
}


//SetAllHor3DPosUndoEvent
SetAllHor3DPosUndoEvent::SetAllHor3DPosUndoEvent( Horizon3D* hor,
				Array2D<float>* oldarr )
    : horizon_( hor )
    , oldarr_( oldarr )
    , newarr_( 0 )
    , oldorigin_( hor->geometry().geometryElement()->rowRange().start_,
                  hor->geometry().geometryElement()->colRange().start_ )
{}


SetAllHor3DPosUndoEvent::SetAllHor3DPosUndoEvent( Horizon3D* hor,
				Array2D<float>* oldarr,
				const RowCol& oldorigin )
    : horizon_( hor )
    , oldarr_( oldarr )
    , newarr_( 0 )
    , oldorigin_( oldorigin )
{}


SetAllHor3DPosUndoEvent::~SetAllHor3DPosUndoEvent()
{
    delete oldarr_;
    delete newarr_;
}


const char* SetAllHor3DPosUndoEvent::getStandardDesc() const
{ return "Bulk change"; }


bool SetAllHor3DPosUndoEvent::unDo()
{
    if ( !EMM().objectExists(horizon_) )
	return false;

    if ( !newarr_ )
    {
	newarr_ = horizon_->createArray2D( nullptr );
	neworigin_.row() =
                horizon_->geometry().geometryElement()->rowRange().start_;
	neworigin_.col() =
                horizon_->geometry().geometryElement()->colRange().start_;
    }

    if ( !newarr_ )
	return false;

    return setArray( *oldarr_, oldorigin_ );
}


bool SetAllHor3DPosUndoEvent::reDo()
{
    return setArray( *newarr_, neworigin_ );
}


ObjectID SetAllHor3DPosUndoEvent::getObjectID() const
{
    return horizon_ ? horizon_->id() : ObjectID::udf();
}


bool SetAllHor3DPosUndoEvent::setArray( const Array2D<float>& arr,
					const RowCol& origin )
{
    if ( !EMM().objectExists(horizon_) )
	return false;

    mDynamicCastGet(Geometry::ParametricSurface*,surf,
		    horizon_->geometryElement())

    StepInterval<int> curcolrg = surf->colRange();
    const StepInterval<int> targetcolrg( origin.col(),
                                         origin.col()+curcolrg.step_*(arr.info().getSize(1)-1), curcolrg.step_ );

    while ( curcolrg.start_-curcolrg.step_>=targetcolrg.start_ )
    {
        const int newcol = curcolrg.start_-curcolrg.step_;
	surf->insertCol( newcol );
        curcolrg.start_ = newcol;
    }

    while ( curcolrg.stop_+curcolrg.step_<=targetcolrg.stop_ )
    {
        const int newcol = curcolrg.stop_+curcolrg.step_;
	surf->insertCol( newcol );
        curcolrg.stop_ = newcol;
    }

    StepInterval<int> currowrg = surf->rowRange();
    const StepInterval<int> targetrowrg( origin.row(),
                                         origin.row()+currowrg.step_*(arr.info().getSize(0)-1), currowrg.step_ );

    while ( currowrg.start_-currowrg.step_>=targetrowrg.start_ )
    {
        const int newrow = currowrg.start_-currowrg.step_;
	surf->insertCol( newrow );
        currowrg.start_ = newrow;
    }

    while ( currowrg.stop_+currowrg.step_<=targetrowrg.stop_ )
    {
        const int newrow = currowrg.stop_+currowrg.step_;
	surf->insertCol( newrow );
        currowrg.stop_ = newrow;
    }

    if ( currowrg!=targetrowrg || curcolrg!=targetcolrg )
    { //new array is smaller
	Array2DImpl<float> tmparr( currowrg.nrSteps()+1, curcolrg.nrSteps()+1 );
	if ( !tmparr.isOK() )
	    return false;

	tmparr.setAll( mUdf(float) );
        Array2DPaste( tmparr, arr, currowrg.nearestIndex( targetrowrg.start_ ),
                      curcolrg.nearestIndex( targetcolrg.start_ ), false );

	return horizon_->setArray2D( tmparr, false, 0, true );
    }

    const RowCol start( targetrowrg.start_, targetcolrg.start_ );
    const RowCol stop( targetrowrg.stop_, targetcolrg.stop_ );
    horizon_->geometry().geometryElement()->expandWithUdf( start, stop );

    return horizon_->setArray2D( arr, false, 0, false );
}



// SetPosAttribUndoEvent
SetPosAttribUndoEvent::SetPosAttribUndoEvent( const PosID& pid,
						  int attr, bool yesno )
    : posid_( pid )
    , attrib_( attr )
    , yn_( yesno )
{}


SetPosAttribUndoEvent::~SetPosAttribUndoEvent()
{}


const char* SetPosAttribUndoEvent::getStandardDesc() const
{ return "Set/Changed position attribute"; }


bool SetPosAttribUndoEvent::unDo()
{
    EMObject* emobject = EMM().getObject( posid_.objectID() );
    if ( !emobject )
	return true;

    emobject->setPosAttrib( posid_, attrib_, !yn_, false );
    return true;
}


bool SetPosAttribUndoEvent::reDo()
{
    EMObject* emobject = EMM().getObject( posid_.objectID() );
    if ( !emobject )
	return true;

    emobject->setPosAttrib( posid_, attrib_, yn_, false ); \
    return true;
}



// PosIDChangeEvent
PosIDChangeEvent::PosIDChangeEvent( const PosID& from,
					const PosID& to,
					const Coord3& tosprevpos )
    : from_(from)
    , to_(to)
    , savedpos_(tosprevpos)
{}


PosIDChangeEvent::~PosIDChangeEvent()
{}


const char* PosIDChangeEvent::getStandardDesc() const
{
    return "Changed posid";
}


bool PosIDChangeEvent::unDo()
{
    EMManager& emm = EMM();
    EMObject* emobject = emm.getObject(from_.objectID());
    if ( !emobject ) return false;

    const bool  geomchecks  = emobject->enableGeometryChecks(false);
    const Coord3 frompos = emobject->getPos( from_ );
    emobject->changePosID( to_, from_, false );
    emobject->setPos( to_, savedpos_, false );
    savedpos_ = frompos;
    emobject->enableGeometryChecks( geomchecks );
    return true;
}


bool PosIDChangeEvent::reDo()
{
    EMManager& emm = EMM();
    EMObject* emobject = emm.getObject(from_.objectID());
    if ( !emobject ) return false;

    const bool  geomchecks  = emobject->enableGeometryChecks(false);
    const Coord3 topos = emobject->getPos( to_ );
    emobject->changePosID( from_, to_, false );
    emobject->setPos( from_, savedpos_, false );
    savedpos_ = topos;
    emobject->enableGeometryChecks( geomchecks );
    return true;
}



// SetPrefColorEvent
SetPrefColorEvent::SetPrefColorEvent( const ObjectID& objid,
					  const OD::Color& oldcol,
					  const OD::Color& newcol )
    : objectid_(objid)
    , oldcolor_(oldcol)
    , newcolor_(newcol)
{}


SetPrefColorEvent::~SetPrefColorEvent()
{}


const char* SetPrefColorEvent::getStandardDesc() const
{ return "Color change"; }


bool SetPrefColorEvent::unDo()
{
    EMObject* emobj = EMM().getObject( objectid_ );
    if ( !emobj ) return false;

    emobj->setPreferredColor( oldcolor_, false );
    return true;
}


bool SetPrefColorEvent::reDo()
{
    EMObject* emobj = EMM().getObject( objectid_ );
    if ( !emobj ) return false;

    emobj->setPreferredColor( newcolor_, false );
    return true;
}

} // namespace EM
