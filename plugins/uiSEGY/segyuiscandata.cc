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


SEGY::uiScanData::uiScanData( const char* fnm )
    : filenm_(fnm)
    , usable_(true)
    , nrtrcs_(0)
    , inls_(mUdf(int),0,1)
    , crls_(mUdf(int),0,1)
    , zrg_(mUdf(float),0.f,1.f)
    , xrg_(mUdf(double),0.)
    , yrg_(mUdf(double),0.)
    , refnrs_(mUdf(float),0.f)
    , offsrg_(mUdf(float),0.f)
{
}


void SEGY::uiScanData::merge( const SEGY::uiScanData& sd )
{
    //TODO implement
}
