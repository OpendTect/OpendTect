#ifndef segyuiscandata_h
#define segyuiscandata_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "ranges.h"
#include "bufstring.h"


namespace SEGY
{

/*!\brief info to collect for SEG-Y scanning in UI */

mExpClass(uiSEGY) uiScanData
{
public:

				uiScanData(const char* fnm);

	const BufferString	filenm_;
	bool			usable_;
	int			revision_;
	bool			hdrsswapped_;
	bool			dataswapped_;
	int			ns_;
	int			nrtrcs_;
	StepInterval<float>	zrg_;
	Interval<double>	xrg_;
	Interval<double>	yrg_;
	Interval<float>		offsrg_;
	StepInterval<int>	inls_;
	StepInterval<int>	crls_;
	StepInterval<int>&	trcnrs_; //!< ref to crls_
	Interval<float>		refnrs_;
	int			bytenrkey1_;
	int			bytenrkey2_;

protected:

};

} // namespace SEGY


#endif
