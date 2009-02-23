#ifndef wellwriter_h
#define wellwriter_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id: wellwriter.h,v 1.8 2009-02-23 16:06:42 cvsbruno Exp $
________________________________________________________________________


-*/

#include "wellio.h"
#include <iosfwd>

namespace Well
{
class Data;
class Log;

mClass Writer : public IO
{
public:

			Writer(const char* fnm,const Data&);

    bool		put() const;		//!< Just write all

    bool		putInfo() const;	//!< Write Info only
    bool		putTrack() const;	//!< Write track only
    bool		putLogs() const;	//!< Write logs only
    bool		putMarkers() const;	//!< Write Markers only
    bool		putD2T() const;		//!< Write D2T model only
    bool		putDispProps() const;	//!< Write display pars only

    bool		putInfo(std::ostream&) const;
    bool		putLog(std::ostream&,const Log&) const;
    bool		putMarkers(std::ostream&) const;
    bool		putD2T(std::ostream&) const;
    bool		putDispProps(std::ostream&) const;

    void		setBinaryWriteLogs( bool yn )	{ binwrlogs_ = yn; }

protected:

    const Data&		wd;
    bool		binwrlogs_;

    bool		wrHdr(std::ostream&,const char*) const;
    bool		putTrack(std::ostream&) const;

};

}; // namespace Well

#endif
