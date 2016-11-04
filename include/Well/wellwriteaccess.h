#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
________________________________________________________________________


-*/

#include "wellcommon.h"
class uiString;


namespace Well
{

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

    virtual const uiString& errMsg() const		= 0;

    virtual bool	isFunctional() const		{ return true; }
    const Data&		data() const			{ return wd_; }

protected:

    const Data&		wd_;

};

}; // namespace Well
