/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "segyuiscandata.h"
#include "segyhdr.h"
#include "segyhdrdef.h"
#include "od_istream.h"


SEGY::uiScanDef::uiScanDef()
    : hdrdef_(0)
{
    reInit();
}


void SEGY::uiScanDef::reInit()
{
    revision_ = ns_ = -1;
    format_ = 5;
    hdrsswapped_ = dataswapped_ = false;
    coordscale_ = sampling_.start = sampling_.step = 1.0f;
    delete hdrdef_; hdrdef_ = new TrcHeaderDef;
}


SEGY::uiScanDef::~uiScanDef()
{
    delete hdrdef_;
}


SEGY::TrcHeader* SEGY::uiScanDef::getTrcHdr( od_istream& strm ) const
{
    char thbuf[SegyTrcHeaderLength];
    strm.getBin( thbuf, SegyTrcHeaderLength );
    if ( !strm.isOK() )
	return 0;

    return new SEGY::TrcHeader( (unsigned char*)thbuf, revision_==1, *hdrdef_ );
}



SEGY::uiScanData::uiScanData( const char* fnm )
    : filenm_(fnm)
    , usable_(true)
    , nrtrcs_(0)
    , inls_(mUdf(int),0,1)
    , crls_(mUdf(int),0,1)
    , xrg_(mUdf(double),0.)
    , yrg_(mUdf(double),0.)
    , refnrs_(mUdf(float),0.f)
    , offsrg_(mUdf(float),0.f)
{
}


void SEGY::uiScanData::getFromSEGYBody( od_istream& strm, const uiScanDef& def,
					DataClipSampler* cs )
{
    int nrtrcs = 0;
    while ( true )
    {
	PtrMan<TrcHeader> thdr = def.getTrcHdr( strm );
	if ( !thdr )
	    return;

	if ( nrtrcs++ == 10 )
	    return;
    }
}


void SEGY::uiScanData::merge( const SEGY::uiScanData& sd )
{
    //TODO implement
}
