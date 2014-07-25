#ifndef wellreadaccess_h
#define wellreadaccess_h

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

/*!\brief Base class for object reading data from data store into Well::Data */

mExpClass(Well) ReadAccess
{
public:

			ReadAccess( Data& d ) : wd_(d)	{}
    virtual		~ReadAccess()			{}

    virtual bool	get() const			= 0; //!< Just read all

    virtual bool	getInfo() const			= 0;
    virtual bool	getTrack() const		= 0;
    virtual bool	getLogs() const			= 0;
    virtual bool	getMarkers() const		= 0;
    virtual bool	getD2T() const			= 0;
    virtual bool	getCSMdl() const		= 0; //!< Checkshot mdl
    virtual bool	getDispProps() const		= 0;
    virtual bool	getLog(const char* lognm) const	= 0;
    virtual void	getLogInfo(BufferStringSet& lognms) const = 0;

    virtual Interval<float> getLogDahRange(const char*) const = 0;
    virtual Interval<float> getAllLogsDahRange() const	= 0;

    virtual const OD::String& errMsg() const		= 0;

protected:

    Data&		wd_;
};

}; // namespace Well


#endif
