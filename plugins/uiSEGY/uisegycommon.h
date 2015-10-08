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
class uiParent;

#define mMaxReasonableNS 25000
	// Time: 50 (2ms) or 100 seconds (4ms); Depth: 25 km (1m), 100 km (4m)


namespace SEGY
{


mExpClass(uiSEGY) FullSpec
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

bool displayWarnings(const BufferStringSet&,bool withstop=false);
void displayReport(uiParent*,const IOPar&,const char* fnm=0);

} // namespace uiSEGY



#endif
