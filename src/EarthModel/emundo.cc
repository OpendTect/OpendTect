/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "emundo.h"

#include "arrayndimpl.h"
#include "arrayndalgo.h"
#include "emmanager.h"
#include "emsurface.h"
#include "emhorizon3d.h"
#include "errh.h"
#include "iopar.h"


const char* EM::SetPosUndoEvent::savedposstr = "Pos";


EM::SetPosUndoEvent::SetPosUndoEvent( const Coord3& oldpos_,
					    const EM::PosID& posid_ )
    : posid( posid_ )
    , savedpos( oldpos_ )
{}


const char* EM::SetPosUndoEvent::getStandardDesc() const
{ return "Set/Changed position"; }


bool EM::SetPosUndoEvent::unDo()
{
    EMManager& manager = EM::EMM();

    if ( !manager.getObject(posid.objectID()))
	return true;

    EMObject* emobject = manager.getObject(posid.objectID());
    if ( !emobject ) return false;

    const bool haschecks = emobject->enableGeometryChecks( false );
    
    bool res = false;
    const Coord3 proxy = emobject->getPos( posid );
    if ( emobject->setPos( posid, savedpos, false ) )
    {
	savedpos = proxy;
	res = true;
    }

    emobject->enableGeometryChecks( haschecks );
    return res;
}


bool EM::SetPosUndoEvent::reDo()
{
    EMManager& manager = EM::EMM();

    if ( !manager.getObject(posid.objectID()))
	return true;

    EMObject* emobject = manager.getObject(posid.objectID());
    if ( !emobject ) return false;

    bool res = false;
    const bool haschecks = emobject->enableGeometryChecks(false);

    const Coord3 proxy = emobject->getPos( posid );
    if ( emobject->setPos( posid, savedpos, false ) )
    {
	savedpos = proxy;
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
	neworigin_.row =
		horizon_->geometry().sectionGeometry(sid_)->rowRange().start;
	neworigin_.col =
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


bool EM::SetAllHor3DPosUndoEvent::setArray( const Array2D<float>& arr,
					    const RowCol& origin )
{
    if ( !EMM().objectExists(horizon_) )
	return false;
    
    mDynamicCastGet( Geometry::ParametricSurface*, surf,
		     horizon_->sectionGeometry( sid_ ) );

    StepInterval<int> curcolrg = surf->colRange();
    const StepInterval<int> targetcolrg( origin.col,
	origin.col+curcolrg.step*(arr.info().getSize(1)-1), curcolrg.step );

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
    const StepInterval<int> targetrowrg( origin.row,
	origin.row+currowrg.step*(arr.info().getSize(0)-1), currowrg.step );

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

	return horizon_->setArray2D( tmparr, sid_, false, 0 );
    }
    
    const RowCol start( targetrowrg.start, targetcolrg.start );
    const RowCol stop( targetrowrg.stop, targetcolrg.stop );
    horizon_->geometry().sectionGeometry(sid_)->expandWithUdf( start, stop );

    return horizon_->setArray2D( arr, sid_, false, 0 );
}



EM::SetPosAttribUndoEvent::SetPosAttribUndoEvent( const EM::PosID& pid,
						  int attr, bool yesno )
    : posid( pid )
    , attrib( attr )
    , yn( yesno )
{}


const char* EM::SetPosAttribUndoEvent::getStandardDesc() const
{ return "Set/Changed position attribute"; }

#define mSetPosAttribUndoEvenUndoRedo( arg ) \
    EMManager& manager = EM::EMM(); \
 \
    EMObject* emobject = manager.getObject(posid.objectID()); \
    if ( !emobject ) return true; \
 \
    emobject->setPosAttrib( posid, attrib, arg, false ); \
    return true

bool EM::SetPosAttribUndoEvent::unDo()
{
    mSetPosAttribUndoEvenUndoRedo( !yn );
}


bool EM::SetPosAttribUndoEvent::reDo()
{
    mSetPosAttribUndoEvenUndoRedo( yn );
}


EM::PosIDChangeEvent::PosIDChangeEvent( const EM::PosID& from_,
				        const EM::PosID& to_,
				        const Coord3& tosprevpos_ )
    : from( from_ )
    , to( to_ )
    , savedpos( tosprevpos_ )
{ }


const char* EM::PosIDChangeEvent::getStandardDesc() const
{
    return "Changed posid";
}


bool EM::PosIDChangeEvent::unDo()
{
    EM::EMManager& emm = EM::EMM();
    EM::EMObject* emobject = emm.getObject(from.objectID());
    if ( !emobject ) return false;

    const bool  geomchecks  = emobject->enableGeometryChecks(false);
    const Coord3 frompos = emobject->getPos( from );
    emobject->changePosID( to, from, false );
    emobject->setPos( to, savedpos, false );
    savedpos = frompos;
    emobject->enableGeometryChecks( geomchecks );
    return true;
}


bool EM::PosIDChangeEvent::reDo()
{
    EM::EMManager& emm = EM::EMM();
    EM::EMObject* emobject = emm.getObject(from.objectID());
    if ( !emobject ) return false;

    const bool  geomchecks  = emobject->enableGeometryChecks(false);
    const Coord3 topos = emobject->getPos( to );
    emobject->changePosID( from, to, false );
    emobject->setPos( from, savedpos, false );
    savedpos = topos;
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
