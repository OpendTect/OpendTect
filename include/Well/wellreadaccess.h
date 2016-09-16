#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
________________________________________________________________________


-*/

#include "wellcommon.h"
#include "uistrings.h"
class BufferStringSet;


namespace Well
{

/*!\brief Base class for object reading data from data store into Well::Data */

mExpClass(Well) ReadAccess
{
public:

			ReadAccess( Data& d ) : wd_(d)	{}
    virtual		~ReadAccess()			{}

    virtual bool	getInfo() const			= 0;
    virtual bool	getTrack() const		= 0;
    virtual bool	getLogs() const			= 0;
    virtual bool	getMarkers() const		= 0;
    virtual bool	getD2T() const			= 0;
    virtual bool	getCSMdl() const		= 0; //!< Checkshot mdl
    virtual bool	getDispProps() const		= 0;
    virtual bool	getLog(const char* lognm) const	= 0;
    virtual void	getLogInfo(BufferStringSet& lognms) const = 0;

    virtual const uiString& errMsg() const		= 0;
    Data&		data()				{ return wd_; }
    const Data&		data() const			{ return wd_; }

protected:

    Data&		wd_;

    bool		addToLogSet(Log*) const;
    bool		updateDTModel(D2TModel&,bool ischeckshot,
					uiString& errmsg) const;

};

}; // namespace Well
