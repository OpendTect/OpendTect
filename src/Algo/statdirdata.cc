/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "statdirdata.h"


Stats::SectorPartData::SectorPartData( float v, float p, int cnt )
    : val_(v), pos_(p), count_(cnt)
{}


Stats::SectorPartData::SectorPartData( const SectorPartData& oth )
    : val_(oth.val_), pos_(oth.pos_), count_(oth.count_)
{}


Stats::SectorPartData::~SectorPartData()
{}


bool Stats::SectorPartData::operator ==( const SectorPartData& oth ) const
{
    return pos_ == oth.pos_;
}


// DirectionalData

Stats::DirectionalData::DirectionalData( const DirectionalData& oth )
{
    *this = oth;
}


Stats::DirectionalData::DirectionalData( int nrsect, int nrparts )
{
    init( nrsect, nrparts );
}


Stats::DirectionalData::~DirectionalData()
{}


float Stats::DirectionalData::angle( int isect, int bound ) const
{
    if ( isEmpty() )
	return mUdf(float);

    float fullc; Angle::getFullCircle( setup_.angletype_, fullc );
    const float angstep = fullc / size();
    const float centerang = setup_.angle0_ + angstep * isect;
    return centerang + bound * angstep * .5f;
}


float Stats::DirectionalData::angle( int isect, Angle::Type t, int bound ) const
{
    const float ang = angle( isect, bound );
    return Angle::convert( setup_.angletype_, ang, t );
}


int Stats::DirectionalData::sector( float ang, Angle::Type t ) const
{
    return sector( Angle::convert(t,ang,setup_.angletype_) );
}


int Stats::DirectionalData::sector( float ang ) const
{
    if ( mIsUdf(ang) )
	return 0;

    ang -= setup_.angle0_;
    const float usrang = Angle::convert(setup_.angletype_,ang,Angle::UsrDeg);
    const float fsect = size() * (usrang / 360);
    int sect = mNINT32(fsect);
    if ( sect >= size() ) sect = 0;
    return sect;
}


int Stats::DirectionalData::part( int isect, float pos ) const
{
    const int nrparts = nrParts( isect );
    const float fpart = nrparts * pos / setup_.usrposrg_.width();
    int prt = (int)fpart;
    if ( prt<0 ) prt = 0;
    if ( prt>=nrparts ) prt = nrparts-1;
    return prt;
}


Stats::DirectionalData&
Stats::DirectionalData::operator=( const DirectionalData& oth )
{
    deepCopy( *this, oth );
    return *this;
}


void Stats::DirectionalData::init( int nrsect, int nrparts )
{
    erase();

    for ( int isect=0; isect<nrsect; isect++ )
    {
	SectorData* sd = new SectorData;
	*this += sd;
	for ( int ipart=0; ipart<nrparts; ipart++ )
	    *sd += SectorPartData( 0, (ipart + .5f) / nrparts, 0 );
    }
}
