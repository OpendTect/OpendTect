#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"
#include "commondefs.h"

class BufferStringSet;
class uiString;

namespace Well
{
class Data;
class Log;

/*!\brief Base class for object reading data from data store into Well::Data */

mExpClass(Well) WriteAccess
{
public:

    virtual		~WriteAccess();
			mOD_DisableCopy(WriteAccess)

    virtual bool	put() const			= 0; //!< Just write all

    virtual bool	needsInfoAndTrackCombined() const	= 0;
    virtual bool	putInfo() const			= 0;
    virtual bool	putTrack() const		= 0;
    virtual bool	putLogs() const			= 0;
    virtual bool	putMarkers() const		= 0;
    virtual bool	putD2T() const			= 0;
    virtual bool	putCSMdl() const		= 0; //!< Checkshot mdl
    virtual bool	putDispProps() const		= 0;
    virtual bool	putLog(const Log&) const	= 0;
    virtual bool	putDefLogs() const		{ return false; }
    virtual bool	swapLogs(const Log&,const Log&) const
			{ return false; }
    virtual bool	renameLog(const char* oldnm,const char* newnm)
			{ return false; }

    virtual const uiString& errMsg() const		= 0;

    virtual bool	isFunctional() const		{ return true; }
    virtual bool	canSwapLogs()			{ return false; }

protected:
			WriteAccess(const Data&);

    const Data&		wd_;
};

} // namespace Well
