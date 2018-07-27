#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2018
________________________________________________________________________

-*/

#include "dbkey.h"
#include "surveydisklocation.h"


/*!\brief DBKey valid in multi-survey contexts */

mExpClass(Basic) FullDBKey : public DBKey
{
public:

			FullDBKey()				{}
			FullDBKey( const DBKey& oth )		{ *this = oth; }
			FullDBKey( const FullDBKey& oth )	{ *this = oth; }
    explicit		FullDBKey( const SurveyDiskLocation& sdl )
				: survloc_(sdl)			{}
			FullDBKey( const DBKey& dbky,
				       const SurveyDiskLocation& sdl )
				: DBKey(dbky), survloc_(sdl)	{}
			FullDBKey( const SurveyDiskLocation& sdl,
				       const DBKey& dbky )
				: DBKey(dbky), survloc_(sdl)	{}
    FullDBKey&		operator =(const DBKey&);
    FullDBKey&		operator =(const FullDBKey&);
    bool		operator ==(const FullDBKey&) const;
    bool		operator !=(const FullDBKey&) const;
    bool		operator ==(const DBKey&) const;
    bool		operator !=(const DBKey&) const;

    virtual DBKey*	clone() const	{ return new FullDBKey(*this); }

    static FullDBKey	getFromStr(const char*);
    virtual BufferString toString() const;
    virtual void	fromString(const char*);

    DBKey		getKey() const	{ return *this; }
    void		setKey( const DBKey& ky )
			{ DBKey::operator =( ky ); }
    virtual const SurveyDiskLocation& surveyDiskLocation() const
			{ return survloc_; }
    void		setSurveyDiskLocation( const SurveyDiskLocation& sdl )
			{ survloc_ = sdl; }

    virtual bool	isInCurrentSurvey() const
			{ return survloc_.isCurrentSurvey(); }
    BufferString	surveyName() const;
    BufferString	fullPath() const	{ return survloc_.fullPath(); }
    void		hasSoftPath()		{ survloc_.hasSoftPath(); }
    void		ensureHardPath()	{ survloc_.ensureHardPath(); }
    void		softenPath()		{ survloc_.softenPath(); }

protected:

    SurveyDiskLocation	survloc_;

};
