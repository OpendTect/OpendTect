#ifndef wellwriteaccess_h
#define wellwriteaccess_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
 RCS:		$Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "gendefs.h"
class BufferStringSet;


namespace Well
{
class Data;
class Log;

/*!\brief Base class for object reading data from data store into Well::Data */

mExpClass(Well) WriteAccess
{
public:

			WriteAccess( const Data& d ) : wd_(d)	{}
    virtual		~WriteAccess()				{}

    virtual bool	put() const			= 0; //!< Just write all

    virtual bool	putInfoAndTrack() const		= 0;
    virtual bool	putTrack() const		= 0;
    virtual bool	putLogs() const			= 0;
    virtual bool	putMarkers() const		= 0;
    virtual bool	putD2T() const			= 0;
    virtual bool	putCSMdl() const		= 0; //!< Checkshot mdl
    virtual bool	putDispProps() const		= 0;
    virtual bool	putLog(const Log&) const	= 0;

    virtual const OD::String& errMsg() const		= 0;

protected:

    const Data&		wd_;

};

}; // namespace Well


#endif
