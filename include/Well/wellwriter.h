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
#include "bufstring.h"
class IOObj;

namespace Well
{
class Data;
class Log;
class WriteAccess;

/*!\brief Writes Well::Data to any data storage. */

mExpClass(Well) Writer
{
public:

			Writer(const MultiID&,const Data&);
			Writer(const IOObj&,const Data&);
			~Writer();
    bool		isUsable() const	{ return wa_; }

    bool		put() const;		//!< Just write all

    bool		putInfoAndTrack() const;//!< Write Info and track
    bool		putTrack() const;	//!< Write track only
    bool		putLogs() const;	//!< Write logs only
    bool		putMarkers() const;	//!< Write Markers only
    bool		putD2T() const;		//!< Write D2T model only
    bool		putCSMdl() const;	//!< Write Check shot model only
    bool		putDispProps() const;	//!< Write display pars only
    bool		putLog(const Log&) const;

    const OD::String&	errMsg() const		{ return errmsg_; }

    /* DEPRECATED, will only read from OD internal data store! */
			Writer(const char*,const Data&);

protected:

    WriteAccess*	wa_;
    mutable BufferString errmsg_;

private:

    void		init(const IOObj&,const Data&);

};


}; // namespace Well

#endif
