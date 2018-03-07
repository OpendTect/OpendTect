#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "wellcommon.h"
#include "uistring.h"
#include "od_iosfwd.h"
class IOObj;

namespace Well
{
class WriteAccess;

/*!\brief Writes Well::Data to any data storage.

  It is essential that creating a writer does not imply writing any actual data.

 */

mExpClass(Well) Writer
{ mODTextTranslationClass(Well::Writer)
public:

			Writer(const DBKey&,const Data&);
			Writer(const IOObj&,const Data&);
			~Writer();
    bool		isUsable() const	{ return wa_; }

    bool		put() const;		//!< Just write all

    bool		putInfoAndTrack() const;//!< Write Info and track
    bool		putLogs() const;	//!< Write logs only
    bool		putMarkers() const;	//!< Write Markers only
    bool		putD2T() const;		//!< Write D2T model only
    bool		putCSMdl() const;	//!< Write Check shot model only
    bool		putDispProps() const;	//!< Write display pars only

    const uiString&	errMsg() const		{ return errmsg_; }
    const Data*		data() const;

    bool		isFunctional() const;

    static bool		isFunctional(const DBKey&);
    static bool		isFunctional(const IOObj&);

protected:

    WriteAccess*	wa_;
    mutable uiString	errmsg_;

private:

    void		init(const IOObj&,const Data&);

};


}; // namespace Well
