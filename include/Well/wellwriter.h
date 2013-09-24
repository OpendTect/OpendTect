#ifndef wellwriter_h
#define wellwriter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "wellio.h"
#include "od_iosfwd.h"

namespace Well
{
class Data;
class Log;

/*!
\brief Writes Well::Data.
*/

mExpClass(Well) Writer : public IO
{
public:

			Writer(const char* fnm,const Data&);

    bool		put() const;		//!< Just write all

    bool		putInfoAndTrack() const;//!< Write Info and track
    bool		putTrack() const;	//!< Write track only
    bool		putLogs() const;	//!< Write logs only
    bool		putMarkers() const;	//!< Write Markers only
    bool		putD2T() const;		//!< Write D2T model only
    bool		putCSMdl() const;	//!< Write Check shot model only
    bool		putDispProps() const;	//!< Write display pars only

    bool		putInfoAndTrack(od_ostream&) const;
    bool		putLog(od_ostream&,const Log&) const;
    bool		putMarkers(od_ostream&) const;
    bool		putD2T(od_ostream&) const;
    bool		putCSMdl(od_ostream&) const;
    bool		putDispProps(od_ostream&) const;

    void		setBinaryWriteLogs( bool yn )	{ binwrlogs_ = yn; }

protected:

    const Data&		wd;
    bool		binwrlogs_;

    bool		wrHdr(od_ostream&,const char*) const;
    bool		putTrack(od_ostream&) const;
    bool		doPutD2T(bool) const;
    bool		doPutD2T(od_ostream&,bool) const;

};

}; // namespace Well

#endif

