/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "segyuiscandata.h"


SEGY::uiScanData::uiScanData( const char* fnm )
    : filenm_(fnm)
    , usable_(true)
    , nrtrcs_(0)
    , revision_(-1)
    , hdrsswapped_(false)
    , dataswapped_(false)
    , ns_(0)
    , trcnrs_(crls_)
{
}
