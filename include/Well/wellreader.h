#ifndef wellreader_h
#define wellreader_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
 RCS:		$Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "ranges.h"
#include "bufstring.h"
class IOObj;
class BufferStringSet;


namespace Well
{
class Data;
class ReadAccess;

/*!\brief Reads Well::Data from any data store */

mExpClass(Well) Reader
{
public:

			Reader(const MultiID&,Data&);
			Reader(const IOObj&,Data&);
			~Reader();
    bool		isUsable() const	{ return ra_; }

    bool		get() const;		//!< Just read all

    bool		getInfo() const;	//!< Read Info only
    bool		getTrack() const;	//!< Read Track only
    bool		getLogs() const;	//!< Read logs only
    bool		getMarkers() const;	//!< Read Markers only
    bool		getD2T() const;		//!< Read D2T model only
    bool		getCSMdl() const;	//!< Read Checkshot model only
    bool		getDispProps() const;	//!< Read display props only
    bool		getLog(const char* lognm) const; //!< Read this one only
    void		getLogInfo(BufferStringSet& lognms) const;

    Interval<float>	getLogDahRange(const char* lognm) const;
			//!< If no log with this name, returns [undef,undef]
    Interval<float>	getAllLogsDahRange() const;
			//!< If no log returns [undef,undef]

    const OD::String&	errMsg() const		{ return errmsg_; }

    /* DEPRECATED, will only read from OD internal data store! */
			Reader(const char*,Data&);

protected:

    ReadAccess*		ra_;
    mutable BufferString errmsg_;

private:

    void		init(const IOObj&,Data&);

};


}; // namespace Well


#endif
