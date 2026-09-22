/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "survgeom2d.h"

#include "coord.h"
#include "posinfo2d.h"
#include "testprog.h"
#include "undefval.h"


static bool testSpnrAt()
{
    RefMan<Survey::Geometry2D> geom = new Survey::Geometry2D( "TestLine" );
    mRunStandardTest( geom->isEmpty() && geom->spnrs().isEmpty(),
		      "Geometry starts out empty" );
    mRunStandardTest( mIsUdf(geom->spnrAt( 0 )),
		      "No SP number on empty geometry" );

    geom->add( Coord(1000.,2000.), 1, 10.f );
    geom->add( Coord(1100.,2100.), 2, 20.f );
    mRunStandardTest( geom->size() == 2 && geom->spnrs().size() == 2,
		      "Positions and SP numbers added together" );
    mRunStandardTest( geom->spnrAt(0) == 10.f && geom->spnrAt(1) == 20.f,
		      "SP numbers returned by spnrAt()" );
    mRunStandardTest( mIsUdf(geom->spnrAt( 2 )),
		      "No SP number past the end of the list" );

    // Positions added without SP numbers (crash report 20260622074807)
    PosInfo::Line2DPos pos( 3 );
    pos.coord_.x_ = 1200.;
    pos.coord_.y_ = 2200.;
    geom->dataAdmin().add( pos );
    mRunStandardTest( geom->size() == 3 && geom->spnrs().size() == 2,
		      "Position added without SP number" );
    mRunStandardTest( mIsUdf(geom->spnrAt( 2 )),
		      "No SP number for position without SP number" );

    Coord crd;
    float spnr = 0.f;
    mRunStandardTest( geom->getPosByTrcNr( 3, crd, spnr ),
		      "getPosByTrcNr() finds position without SP number" );

    return true;
}


static bool testAddSync()
{
    // Deliberately unsorted trace numbers + one duplicate
    RefMan<Survey::Geometry2D> geom = new Survey::Geometry2D( "TestLine3" );
    geom->add( Coord(0.,0.), 3, 30.f );
    geom->add( Coord(1.,1.), 1, 10.f );
    geom->add( Coord(2.,2.), 2, 20.f );
    geom->add( Coord(3.,3.), 3, 99.f ); // duplicate: must be rejected

    mRunStandardTest( geom->size() == 3 && geom->spnrs().size() == 3,
		      "Duplicate rejected without SP list drift" );

    const PosInfo::Line2DData& data = geom->data();
    const TypeSet<float>& spnrs = geom->spnrs();
    bool aligned = true;
    for ( int idx=0; idx<data.positions().size(); idx++ )
	aligned = aligned && mIsEqual( spnrs[idx],
			10.f * data.positions()[idx].nr_, 0.001f );
    mRunStandardTest( aligned, "SP numbers aligned after mid-list inserts" );

    return true;
}


static bool testSetEmpty()
{
    RefMan<Survey::Geometry2D> geom = new Survey::Geometry2D( "TestLine2" );
    geom->add( Coord(0.,0.), 1, 5.f );
    geom->setEmpty();
    mRunStandardTest( geom->isEmpty() && geom->spnrs().isEmpty(),
		      "setEmpty() clears positions and SP numbers" );

    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    return testSpnrAt() && testAddSync() && testSetEmpty() ? 0 : 1;
}
