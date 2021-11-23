#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
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

    const Data&		wd_;

};

}; // namespace Well


