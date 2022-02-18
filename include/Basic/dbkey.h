#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		August 2020
________________________________________________________________________

-*/


#include "multiid.h"
#include "objectset.h"

class SurveyDiskLocation;
class SurveyInfo;

mExpClass(Basic) DBKey : public MultiID
{
public:
			DBKey()
			{}
			DBKey( const MultiID& mid )
			    : MultiID(mid)
			{}
			DBKey(const MultiID& mid,
			      const SurveyDiskLocation&);
			DBKey(const DBKey&);
			~DBKey();

    DBKey&		operator =(const DBKey&);
    bool		operator ==(const DBKey&) const;
    bool		operator !=(const DBKey&) const;

    bool		isValid() const		{ return !isUdf(); }

    void		setSurveyDiskLocation(const SurveyDiskLocation&);
    bool		hasSurveyLocation() const { return survloc_; }
    const SurveyDiskLocation& surveyDiskLocation() const;
    void		clearSurveyDiskLocation();
    bool		isInCurrentSurvey() const;
    const SurveyInfo&	surveyInfo() const;

    DBKey		getLocal() const;

    bool		fromString(const char*);
    BufferString	toString(bool withsurvloc) const;

protected:

    SurveyDiskLocation*		survloc_ = nullptr;
};


mExpClass(Basic) DBKeySet : public OD::Set
{ mIsContainer( DBKeySet, ObjectSet<DBKey>, dbkys_ )
public:

    inline		DBKeySet()		{}
    inline		DBKeySet( const DBKeySet& oth )
				    { deepAppend( dbkys_, oth.dbkys_ ); }
    explicit		DBKeySet( const DBKey& dbky )
						{ add( dbky ); }
			~DBKeySet()		{ deepErase(dbkys_); }
    DBKeySet*		clone() const override	{ return new DBKeySet(*this); }

    inline DBKeySet&	operator =( const DBKeySet& oth )
			{ deepCopy( dbkys_, oth.dbkys_ ); return *this; }
    DBKeySet&		operator =( const TypeSet<MultiID>& oth );
    bool		operator ==(const DBKeySet&) const;
    bool		operator !=(const DBKeySet&) const;

    inline size_type	size() const		{ return dbkys_.size(); }
    inline bool		isEmpty() const		{ return dbkys_.isEmpty(); }
    bool		validIdx( od_int64 i ) const override
						{ return dbkys_.validIdx(i); }
    idx_type		indexOf(const DBKey&) const;
    inline bool		isPresent( const DBKey& dbky )
						{ return indexOf(dbky) >= 0;}
    DBKey&		get( idx_type idx )	{ return *dbkys_.get(idx); }
    const DBKey&	get( idx_type idx ) const { return *dbkys_.get(idx); }
    DBKey&		operator [](idx_type idx) { return *dbkys_[idx]; }
    const DBKey&	operator [](idx_type idx) const { return *dbkys_[idx]; }
    DBKey&		first()			{ return *dbkys_.first(); }
    const DBKey&	first() const		{ return *dbkys_.first(); }
    DBKey&		last()			{ return *dbkys_.last(); }
    const DBKey&	last() const		{ return *dbkys_.last(); }

    DBKeySet&		add( DBKey* ky )	{ dbkys_.add(ky); return *this;}
    DBKeySet&		add( const DBKey& ky )	{ return add(new DBKey(ky)); }
    bool		addIfNew(const DBKey&);
    void		append(const DBKeySet&,bool allowduplicates=true);
    void		insert(idx_type,const DBKey&);
    void		useIndexes( const idx_type* idxs )
						{ dbkys_.useIndexes(idxs); }

    void		setEmpty()		{ deepErase( dbkys_ ); }
    void		erase() override	{ setEmpty(); }
    DBKeySet&		removeSingle(idx_type);
    DBKeySet&		removeRange(idx_type,idx_type);
    DBKeySet&		remove(const DBKey&);

    void		swap( idx_type i1, idx_type i2 )
						{ dbkys_.swap( i1, i2 ); }

    inline DBKeySet&	operator +=( DBKey* k )		{ return add(k); }
    inline DBKeySet&	operator +=( const DBKey& k )	{ return add(k); }
    inline DBKeySet&	operator -=( const DBKey& k )	{ return remove(k); }

    void		addTo(BufferStringSet&) const;

    // remainder of OD::Set interface

    od_int64		nrItems() const override	{ return size(); }
    void		swapItems( od_int64 i1, od_int64 i2 ) override
			{ swap( (idx_type)i1, (idx_type)i2 ); }
    void		reverse() override		{ dbkys_.reverse(); }

};

mDefContainerSwapFunction( Basic, DBKeySet )
