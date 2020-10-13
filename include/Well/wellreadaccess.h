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

    virtual bool	getInfo() const				= 0;
    virtual bool	getTrack() const			= 0;
    virtual bool	getLogs(bool needjustinfo=false) const	= 0;
    virtual bool	getMarkers() const			= 0;
    virtual bool	getD2T() const				= 0;
    virtual bool	getCSMdl() const			= 0;
    virtual bool	getDispProps() const			= 0;
    virtual bool	getLog(const char* lognm) const		= 0;
 //   virtual bool	getLogInfo() const			= 0;
    virtual void	getLogNames(BufferStringSet&) const	= 0;
    virtual void	getLogInfo(ObjectSet<IOPar>&) const	= 0;

    virtual const uiString& errMsg() const			= 0;

    Data&		data()					{ return wd_; }
    const Data&		data() const				{ return wd_; }

    bool		getAll(bool stoponerr) const;

protected:

    Data&		wd_;

    bool		addToLogSet(Log*, bool needjustinfo=false) const;
    bool		updateDTModel(D2TModel&,bool ischeckshot,
					uiString& errmsg) const;
    static float	getZFac(const IOPar&);
			    //!< see cc file for explanation

};

}; // namespace Well
