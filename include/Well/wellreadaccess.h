#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "wellmod.h"
#include "gendefs.h"
#include "uistrings.h"
class BufferStringSet;


namespace Well
{
class D2TModel;
class Data;
class Log;
class Track;

/*!\brief Base class for object reading data from data store into Well::Data */

mExpClass(Well) ReadAccess
{
public:

    virtual		~ReadAccess();

    virtual bool	getInfo() const			= 0;
    virtual bool	getTrack() const		= 0;
    virtual bool	getLogs(bool needjustinfo=false) const	  = 0;
    virtual bool	getMarkers() const		= 0;
    virtual bool	getD2T() const			= 0;
    virtual bool	getCSMdl() const		= 0; //!< Checkshot mdl
    virtual bool	getDispProps() const		= 0;
    virtual bool	getLog(const char* lognm) const	= 0;
    virtual void	getLogInfo(BufferStringSet& lognms) const = 0;
    virtual bool	getDefLogs() const		{ return false; }

    virtual const uiString& errMsg() const		= 0;

    Data&		data()				{ return wd_; }
    const Data&		data() const			{ return wd_; }

protected:
			ReadAccess(Data&);

    Data&		wd_;

    bool		addToLogSet(Log*, bool needjustinfo=false) const;
    bool		updateDTModel(D2TModel*,bool ischeckshot,
					uiString& errmsg) const;
    void		adjustTrackIfNecessary(bool frommarkers=false) const;

    mDeprecated("use updateDTModel with uiString")
    bool		updateDTModel(D2TModel*,bool ischeckshot,
					BufferString& errmsg) const;
			//!< D2TModel will become mine and may even be deleted

    mDeprecatedDef
    bool		updateDTModel(D2TModel*,const Track&,float,bool) const;

public:
    mDeprecated("Use other get functions, or Well::Reader::get")
    virtual bool	get() const			= 0;
};

} // namespace Well
