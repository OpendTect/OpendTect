/*+
________________________________________________________________________

 Copyright:	(C) 1995-2026 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "testprog.h"

#include "visvsgcoltablookup.h"
#include "visvsgcoltabmapping.h"

#include "coltabsequence.h"
#include "math2.h"
#include "typeset.h"

namespace
{

const float cValues[] = { 0.f, 2.5f, 5.f, 10.f, -4.f, 25.f, mUdf(float) };
const int cNrValues = 7;

TypeSet<unsigned char> indicesOf( const dGBVis::ColTabMapping& mapping )
{
    TypeSet<unsigned char> ret;
    for ( od_int64 pos=0; pos<mapping.size(); pos++ )
	ret += mapping.indexAt( pos );

    return ret;
}


void fixedRange( dGBVis::ColTabMapping& mapping )
{
    mapping.mapper().setup_.type_ = ColTab::MapperSetup::Fixed;
    mapping.mapper().setup_.range_ = Interval<float>( 0.f, 10.f );
    mapping.setData( cValues, cNrValues );
    mapping.update();
}

} // namespace


static bool testRange()
{
    dGBVis::ColTabMapping mapping;
    fixedRange( mapping );
    const TypeSet<unsigned char> idxs = indicesOf( mapping );
    mRunStandardTest( mapping.size()==cNrValues, "Mapping: One index a value" );
    mRunStandardTest( idxs[0]==0, "Mapping: The range start is the first" );
    mRunStandardTest( idxs[1]==63, "Mapping: A quarter of the range" );
    mRunStandardTest( idxs[2]==127, "Mapping: The middle of the range" );
    mRunStandardTest( idxs[3]==254, "Mapping: The range end is the last" );
    mRunStandardTest( idxs[4]==0 && idxs[5]==254,
		      "Mapping: Values outside the range are clipped" );
    mRunStandardTest( idxs[6]==dGBVis::ColTabMapping::nrSteps(),
		      "Mapping: An undefined value gets the undefined index" );
    mRunStandardTest( mapping.isUndefAt(6) && !mapping.isUndefAt(0) &&
		      !mapping.isUndefAt(5),
		      "Mapping: The undefined band marks the undefined value" );
    mRunStandardTest( mapping.indices()[1]==255 && mapping.indices()[13]==0,
		      "Mapping: The bands are interleaved, 255 is defined" );
    return true;
}


static bool testAgainstMapper()
{
    dGBVis::ColTabMapping mapping;
    mapping.mapper().setup_.nrsegs_ = 4;
    fixedRange( mapping );
    const TypeSet<unsigned char> idxs = indicesOf( mapping );
    for ( int idx=0; idx<cNrValues-1; idx++ )
    {
	const int expected = ColTab::Mapper::snappedPosition(
		&mapping.mapper(), cValues[idx], dGBVis::ColTabMapping::nrSteps(),
		dGBVis::ColTabMapping::nrSteps() );
	mRunStandardTest( Math::Abs(int(idxs[idx])-expected)<=1,
			  "Mapping: Segments follow the mapper" );
    }

    mRunStandardTest( idxs[0]==31 && idxs[1]==95,
		      "Mapping: A segment maps to its centre" );
    return true;
}


static bool testFlip()
{
    dGBVis::ColTabMapping mapping;
    mapping.mapper().setup_.flipseq_ = true;
    fixedRange( mapping );
    const TypeSet<unsigned char> idxs = indicesOf( mapping );
    mRunStandardTest( idxs[0]==254 && idxs[3]==0,
		      "Mapping: Flipping reverses the indices" );
    mRunStandardTest( mapping.isUndefAt(6),
		      "Mapping: Flipping keeps the undefined value undefined" );
    return true;
}


static bool testColors()
{
    ColTab::Sequence seq;
    seq.setColor( 0.f, 255, 0, 0 );
    seq.setColor( 1.f, 0, 0, 255 );
    seq.setUndefColor( OD::Color(10,20,30) );
    dGBVis::ColTabLookup lookup;
    lookup.setSequence( seq );

    dGBVis::ColTabMapping mapping;
    fixedRange( mapping );
    const TypeSet<unsigned char> idxs = indicesOf( mapping );
    const float lastidx = float( dGBVis::ColTabMapping::nrSteps()-1 );
    const vsg::vec4 first = lookup.colorAt( float(idxs[0])/lastidx );
    const vsg::vec4 last = lookup.colorAt( float(idxs[3])/lastidx );
    const vsg::vec4 udf = lookup.undefColor();
    mRunStandardTest( first.r>0.99f && first.b<0.01f,
		      "Mapping: The range start is the first color" );
    mRunStandardTest( last.b>0.999f && last.r<0.001f,
		      "Mapping: The range end is exactly the last color" );
    mRunStandardTest( Math::Abs(udf.r-10.f/255.f)<0.01f &&
		      Math::Abs(udf.b-30.f/255.f)<0.01f,
		      "Mapping: Undefined values get the undefined color" );
    return true;
}


int mTestMainFnName( int argc, char** argv )
{
    mInitTestProg();

    if ( !testRange() ||
	 !testAgainstMapper() ||
	 !testFlip() ||
	 !testColors() )
	return 1;

    return 0;
}
