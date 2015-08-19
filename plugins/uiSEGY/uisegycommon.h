#ifndef uisegycommon_h
#define uisegycommon_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2015
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uisegymod.h"
#include "seistype.h"
#include "segyfiledef.h"

#define mMaxReasonableNS 25000
	// Time: 50 (2ms) or 100 seconds (4ms); Depth: 25 km (1m), 100 km (4m)


namespace SEGY
{


mExpClass(uiSEGY) FullSpec
{
public:

			FullSpec(Seis::GeomType,bool isvsp=false);

    bool		rev0_;
    FileSpec		spec_;
    FilePars		pars_;
    FileReadOpts	readopts_;
    bool		zinfeet_;

    bool		isVSP() const		{ return isvsp_; }
    Seis::GeomType	geomType() const	{ return readopts_.geomType(); }

    bool		isvsp_;
};

} // namespace SEGY

typedef SEGY::FileSpec		FileSpec;
typedef SEGY::FilePars		FilePars;
typedef SEGY::FileReadOpts	FileReadOpts;
typedef SEGY::FullSpec		FullSpec;



#endif
