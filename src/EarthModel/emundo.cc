/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/

#include "emundo.h"

#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "emmanager.h"
#include "emsurface.h"
#include "emhorizon3d.h"
#include "iopar.h"


const char* EM::SetPosUndoEvent::savedposstr_ = "Pos";

EM::ObjectID EM::EMUndo::getCurrentEMObjectID( bool forredo ) const
{
    const int curidx = indexOf( forredo ? currenteventid_ + 1
					: currenteventid_ );
    if ( !events_.validIdx(curidx) )
	return -1;

    const UndoEvent* curev = events_[curidx];
    mDynamicCastGet( const EMUndoEvent*, emundoev, curev );
    return emundoev ? emundoev->getObjectID() : -1;
}


EM::SetPosUndoEvent::SetPosUndoEvent( const Coord3& oldpos,
				      const EM::PosID& posid )
    : posid_(posid)
    , savedpos_(oldpos)
{}


const char* EM::SetPosUndoEvent::getStandardDesc() const
{ return "Set/Changed position"; }


bool EM::SetPosUndoEvent::unDo()
{
    EMManager& manager = EM::EMM();

    if ( !manager.getObject(posid_.objectID()))
	return true;

    EMObject* emobject = manager.getObject(posid_.objectID());
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


bool EM::SetPosUndoEvent::reDo()
{
    EMManager& manager = EM::EMM();

    if ( !manager.getObject(posid_.objectID()))
	return true;

    EMObject* emobject = manager.getObject(posid_.objectID());
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
EM::SetAllHor3DPosUndoEvent::SetAllHor3DPosUndoEvent( EM::Horizon3D* hor,
				EM::SectionID sid, Array2D<float>* oldarr )
    : horizon_( hor )
    , oldarr_( oldarr )
    , newarr_( 0 )
    , sid_( sid )
    , oldorigin_( hor->geometry().sectionGeometry(sid)->rowRange().start,
		  hor->geometry().sectionGeometry(sid)->colRange().start )
{}


EM::SetAllHor3DPosUndoEvent::SetAllHor3DPosUndoEvent( EM::Horizon3D* hor,
				EM::SectionID sid, Array2D<float>* oldarr,
				const RowCol& oldorigin )
    : horizon_( hor )
    , oldarr_( oldarr )
    , newarr_( 0 )
    , sid_( sid )
    , oldorigin_( oldorigin )
{}


EM::SetAllHor3DPosUndoEvent::~SetAllHor3DPosUndoEvent()
{
    delete oldarr_;
    delete newarr_;
}


const char* EM::SetAllHor3DPosUndoEvent::getStandardDesc() const
{ return "Bulk change"; }


bool EM::SetAllHor3DPosUndoEvent::unDo()
{
    if ( !EMM().objectExists(horizon_) )
	return false;

    if ( !newarr_ )
    {
	newarr_ = horizon_->createArray2D( sid_, 0 );
	neworigin_.row() =
		horizon_->geometry().sectionGeometry(sid_)->rowRange().start;
	neworigin_.col() =
		horizon_->geometry().sectionGeometry(sid_)->colRange().start;
    }

    if ( !newarr_ )
	return false;

    return setArray( *oldarr_, oldorigin_ );
}


bool EM::SetAllHor3DPosUndoEvent::reDo()
{
    return setArray( *newarr_, neworigin_ );
}


EM::ObjectID EM::SetAllHor3DPosUndoEvent::getObjectID() const
{ return horizon_ ? horizon_->id() : -1; }


bool EM::SetAllHor3DPosUndoEvent::setArray( const Array2D<float>& arr,
					    const RowCol& origin )
{
    if ( !EMM().objectExists(horizon_) )
	return false;

    mDynamicCastGet( Geometry::ParametricSurface*, surf,
		     horizon_->sectionGeometry( sid_ ) );

    StepInterval<int> curcolrg = surf->colRange();
    const StepInterval<int> targetcolrg( origin.col(),
	origin.col()+curcolrg.step*(arr.info().getSize(1)-1), curcolrg.step );

    while ( curcolrg.start-curcolrg.step>=targetcolrg.start )
    {
	const int newcol = curcolrg.start-curcolrg.step;
	surf->insertCol( newcol );
	curcolrg.start = newcol;
    }

    while ( curcolrg.stop+curcolrg.step<=targetcolrg.stop )
    {
	const int newcol = curcolrg.stop+curcolrg.step;
	surf->insertCol( newcol );
	curcolrg.stop = newcol;
    }

    StepInterval<int> currowrg = surf->rowRange();
    const StepInterval<int> targetrowrg( origin.row(),
	origin.row()+currowrg.step*(arr.info().getSize(0)-1), currowrg.step );

    while ( currowrg.start-currowrg.step>=targetrowrg.start )
    {
	const int newrow = currowrg.start-currowrg.step;
	surf->insertCol( newrow );
	currowrg.start = newrow;
    }

    while ( currowrg.stop+currowrg.step<=targetrowrg.stop )
    {
	const int newrow = currowrg.stop+currowrg.step;
	surf->insertCol( newrow );
	currowrg.stop = newrow;
    }

    if ( currowrg!=targetrowrg || curcolrg!=targetcolrg )
    { //new array is smaller
	Array2DImpl<float> tmparr( currowrg.nrSteps()+1, curcolrg.nrSteps()+1 );
	if ( !tmparr.isOK() )
	    return false;

	tmparr.setAll( mUdf(float) );
	Array2DPaste( tmparr, arr, currowrg.nearestIndex( targetrowrg.start ),
		      curcolrg.nearestIndex( targetcolrg.start ), false );

	return horizon_->setArray2D( tmparr, sid_, false, 0, true );
    }

    const RowCol start( targetrowrg.start, targetcolrg.start );
    const RowCol stop( targetrowrg.stop, targetcolrg.stop );
    horizon_->geometry().sectionGeometry(sid_)->expandWithUdf( start, stop );

    return horizon_->setArray2D( arr, sid_, false, 0, false );
}



EM::SetPosAttribUndoEvent::SetPosAttribUndoEvent( const EM::PosID& pid,
						  int attr, bool yesno )
    : posid_( pid )
    , attrib_( attr )
    , yn_( yesno )
{}


const char* EM::SetPosAttribUndoEvent::getStandardDesc() const
{ return "Set/Changed position attribute"; }

#define mSetPosAttribUndoEvenUndoRedo( arg ) \
    EMManager& manager = EM::EMM(); \
 \
    EMObject* emobject = manager.getObject(posid_.objectID()); \
    if ( !emobject ) return true; \
 \
    emobject->setPosAttrib( posid_, attrib_, arg, false ); \
    return true

bool EM::SetPosAttribUndoEvent::unDo()
{
    mSetPosAttribUndoEvenUndoRedo( !yn_ );
}


bool EM::SetPosAttribUndoEvent::reDo()
{
    mSetPosAttribUndoEvenUndoRedo( yn_ );
}


EM::PosIDChangeEvent::PosIDChangeEvent( const EM::PosID& from,
					const EM::PosID& to,
					const Coord3& tosprevpos )
    : from_(from)
    , to_(to)
    , savedpos_(tosprevpos)
{ }


const char* EM::PosIDChangeEvent::getStandardDesc() const
{
    return "Changed posid";
}


bool EM::PosIDChangeEvent::unDo()
{
    EM::EMManager& emm = EM::EMM();
    EM::EMObject* emobject = emm.getObject(from_.objectID());
    if ( !emobject ) return false;

    const bool  geomchecks  = emobject->enableGeometryChecks(false);
    const Coord3 frompos = emobject->getPos( from_ );
    emobject->changePosID( to_, from_, false );
    emobject->setPos( to_, savedpos_, false );
    savedpos_ = frompos;
    emobject->enableGeometryChecks( geomchecks );
    return true;
}


bool EM::PosIDChangeEvent::reDo()
{
    EM::EMManager& emm = EM::EMM();
    EM::EMObject* emobject = emm.getObject(from_.objectID());
    if ( !emobject ) return false;

    const bool  geomchecks  = emobject->enableGeometryChecks(false);
    const Coord3 topos = emobject->getPos( to_ );
    emobject->changePosID( from_, to_, false );
    emobject->setPos( from_, savedpos_, false );
    savedpos_ = topos;
    emobject->enableGeometryChecks( geomchecks );
    return true;
}


EM::SetPrefColorEvent::SetPrefColorEvent( const EM::ObjectID& objid,
					  const Color& oldcol,
					  const Color& newcol )
    : objectid_( objid )
    , oldcolor_( oldcol )
    , newcolor_( newcol )
{}


const char* EM::SetPrefColorEvent::getStandardDesc() const
{ return "Color change"; }


bool EM::SetPrefColorEvent::unDo()
{
    EM::EMObject* emobj = EM::EMM().getObject( objectid_ );
    if ( !emobj ) return false;

    emobj->setPreferredColor( oldcolor_, false );
    return true;
}


bool EM::SetPrefColorEvent::reDo()
{
    EM::EMObject* emobj = EM::EMM().getObject( objectid_ );
    if ( !emobj ) return false;

    emobj->setPreferredColor( newcolor_, false );
    return true;
}
