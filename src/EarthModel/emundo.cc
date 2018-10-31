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

DBKey EM::Undo::getCurrentEMObjectID( bool forredo ) const
{
    const int curidx = indexOf( forredo ? currenteventid_ + 1
					: currenteventid_ );
    if ( !events_.validIdx(curidx) )
	return DBKey();

    const ::UndoEvent* curev = events_[curidx];
    mDynamicCastGet( const EM::UndoEvent*, emundoev, curev );
    return emundoev ? emundoev->getObjectID() : DBKey();
}


EM::SetPosUndoEvent::SetPosUndoEvent( const Coord3& oldpos,
				      const EM::PosID& posid )
    : posid_(posid)
    , savedpos_(oldpos)
{}


const char* EM::SetPosUndoEvent::getStandardDesc() const
{
    return "Set/Changed position";
}


bool EM::SetPosUndoEvent::unDo()
{
    ObjectManager& manager = EM::MGR();

    if ( !manager.getObject(getObjectID()))
	return true;

    Object* emobject = manager.getObject(getObjectID());
    if ( !emobject )
	return false;

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
    ObjectManager& manager = EM::MGR();

    if ( !manager.getObject(getObjectID()))
	return true;

    Object* emobject = manager.getObject(getObjectID());
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
						Array2D<float>* oldarr )
    : horizon_( hor )
    , oldarr_( oldarr )
    , newarr_( 0 )
    , oldorigin_( hor->geometry().geometryElement()->rowRange().start,
		  hor->geometry().geometryElement()->colRange().start )
{}


EM::SetAllHor3DPosUndoEvent::SetAllHor3DPosUndoEvent( EM::Horizon3D* hor,
						Array2D<float>* oldarr,
						const RowCol& oldorigin )
    : horizon_( hor )
    , oldarr_( oldarr )
    , newarr_( 0 )
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
    if ( !MGR().objectExists(horizon_) )
	return false;

    if ( !newarr_ )
    {
	newarr_ = horizon_->createArray2D( 0 );
	neworigin_.row() =
		horizon_->geometry().geometryElement()->rowRange().start;
	neworigin_.col() =
		horizon_->geometry().geometryElement()->colRange().start;
    }

    if ( !newarr_ )
	return false;

    return setArray( *oldarr_, oldorigin_ );
}


bool EM::SetAllHor3DPosUndoEvent::reDo()
{
    return setArray( *newarr_, neworigin_ );
}


DBKey EM::SetAllHor3DPosUndoEvent::getObjectID() const
{ return horizon_ ? horizon_->id() : DBKey::getInvalid(); }


bool EM::SetAllHor3DPosUndoEvent::setArray( const Array2D<float>& arr,
					    const RowCol& origin )
{
    if ( !MGR().objectExists(horizon_) )
	return false;

    mDynamicCastGet( Geometry::ParametricSurface*, surf,
		     horizon_->geometryElement() );

    StepInterval<int> curcolrg = surf->colRange();
    const StepInterval<int> targetcolrg( origin.col(),
	origin.col()+curcolrg.step*(arr.getSize(1)-1), curcolrg.step );

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
	origin.row()+currowrg.step*(arr.getSize(0)-1), currowrg.step );

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

	return horizon_->setArray2D( tmparr, false, 0, true );
    }

    const BinID start( targetrowrg.start, targetcolrg.start );
    const BinID stop( targetrowrg.stop, targetcolrg.stop );
    horizon_->geometry().geometryElement()->expandWithUdf( start, stop );

    return horizon_->setArray2D( arr, false, 0, false );
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
    ObjectManager& manager = MGR(); \
 \
    Object* emobject = manager.getObject(getObjectID()); \
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


EM::SetPrefColorEvent::SetPrefColorEvent( const DBKey& objid,
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
    EM::Object* emobj = MGR().getObject( objectid_ );
    if ( !emobj ) return false;

    emobj->setPreferredColor( oldcolor_ );
    return true;
}


bool EM::SetPrefColorEvent::reDo()
{
    Object* emobj = MGR().getObject( objectid_ );
    if ( !emobj )
	return false;

    emobj->setPreferredColor( newcolor_ );
    return true;
}
