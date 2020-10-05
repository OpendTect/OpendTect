#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		June 2015
________________________________________________________________________

-*/

#include "uisegytoolsmod.h"
#include "segyfiledef.h"
class uiParent;


namespace SEGY
{

mGlobal(uiSEGYTools) int cMaxReasonableNrSamples();
	// default value is 25000 samples.
	// Time: 50 (2ms) or 100 seconds (4ms); Depth: 25 km (1m), 100 km (4m)


mExpClass(uiSEGYTools) FullSpec
{
public:

			FullSpec(Seis::GeomType,bool isvsp=false);

    int			rev_;
    FileSpec		spec_;
    FilePars		pars_;
    FileReadOpts	readopts_;
    bool		zinfeet_;

    bool		isVSP() const		{ return isvsp_; }
    Seis::GeomType	geomType() const	{ return readopts_.geomType(); }
    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    bool		isvsp_;
};

} // namespace SEGY


typedef SEGY::FilePars		FilePars;
typedef SEGY::FileReadOpts	FileReadOpts;
typedef SEGY::FullSpec		FullSpec;


namespace uiSEGY
{

bool displayWarnings(uiParent*,const uiStringSet&,bool withstop=false,
		     int nrskipped=0);
void displayReport(uiParent*,const IOPar&,const char* fnm=0);

void initClasses();

} // namespace uiSEGY
