#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2003
 Contents:	Set of BufferStrings
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"
#include "manobjectset.h"
class GlobExpr;
class uiString;
class uiStringSet;
mFDQtclass( QString )
mFDQtclass( QStringList )

/*!\brief Set of BufferString objects. */

mExpClass(Basic) BufferStringSet : public OD::Set
{ mIsContainer( BufferStringSet, ManagedObjectSet<BufferString>, strs_ )
public:

    typedef ObjectSet<BufferString>	SetType;

			BufferStringSet()	{}
    explicit		BufferStringSet(size_type n,const char* s=nullptr);
    explicit		BufferStringSet(const char* arr[],size_type len=-1);
    explicit		BufferStringSet(const char*);
			BufferStringSet(const char*,const char*);
			BufferStringSet(const char*,const char*,const char*);
    BufferStringSet*	clone() const
			{ return new BufferStringSet(*this); }
    virtual		~BufferStringSet()	{}
    bool		operator ==(const BufferStringSet&) const;
    bool		operator !=(const BufferStringSet&) const;

    inline size_type	size() const		{ return strs_.size(); }
    inline bool		isEmpty() const		{ return strs_.isEmpty(); }
    virtual bool	validIdx( od_int64 i ) const
						{ return strs_.validIdx(i); }
    idx_type		indexOf(const char*,CaseSensitivity s=CaseSensitive
						) const; //!< first match
    idx_type		indexOf(const GlobExpr&) const;	//!< first match
    idx_type		indexOf( const BufferString* b ) const
						{ return strs_.indexOf(b); }
    inline bool		isPresent( const BufferString* b ) const
						{ return strs_.isPresent(b);}
    inline bool		isPresent( const char* s,
				   CaseSensitivity c=CaseSensitive ) const
						{ return indexOf(s,c) >= 0; }
    BufferString&	get( idx_type idx )	{ return *strs_.get(idx); }
    const BufferString&	get( idx_type idx ) const { return *strs_.get(idx); }
    BufferString*	first()			{ return strs_.first(); }
    const BufferString*	first() const		{ return strs_.first(); }
    BufferString*	last()			{ return strs_.last(); }
    const BufferString*	last() const		{ return strs_.last(); }

    inline void		setEmpty()		{ strs_.setEmpty(); }
    virtual void	erase()			{ setEmpty(); }
    bool		remove(const char*);
    void		removeSingle( idx_type i ) { strs_.removeSingle(i); }
    void		removeRange( idx_type i1, idx_type i2 )
						{ strs_.removeRange(i1,i2); }
    void		swap( idx_type i1, idx_type i2 )
						{ strs_.swap( i1, i2 ); }

    BufferStringSet&	add(const char*);
    BufferStringSet&	add(const OD::String&);
    BufferStringSet&	add(const mQtclass(QString)&);
    BufferStringSet&	add(const BufferStringSet&,bool allowduplicates);
    BufferStringSet&	add(const char* arr[],size_type len=-1);
    BufferStringSet&	add( BufferString* bs )	{ strs_.add(bs); return *this; }
    BufferStringSet&	addToAll(const char*,bool infront=false);
    bool		addIfNew(const char*);	//!< returns whether added
    bool		addIfNew(const OD::String&);
    void		append( const BufferStringSet& oth )
			{ strs_.append( oth.strs_ ); }
    BufferStringSet&	addWordsFrom(const char*);

#   define		mODBSSDefMatchSens CaseSensitivity cs=CaseInsensitive
    idx_type		nearestMatch(const char*,mODBSSDefMatchSens) const;
    TypeSet<idx_type>	getMatches(const char* globexpr,
				   mODBSSDefMatchSens) const;
    bool		isSubsetOf(const BufferStringSet&) const;
    size_type		maxLength() const;
    idx_type		firstDuplicateOf(idx_type,mODStringDefSens,
					 idx_type startat=0) const;
    bool		hasUniqueNames(mODStringDefSens) const;
    BufferString	commonStart() const;

    void		sort(bool caseinsens=true,bool asc=true);
    idx_type*		getSortIndexes(bool caseinsns=true,bool asc=true) const;
			//!< returns new int [size()] for you to 'delete []'
			//!< does NOT sort but provides data for useIndexes
    void		useIndexes( const idx_type* idxs )
			{ strs_.useIndexes(idxs); }

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    void		fill(uiStringSet&) const;
    void		use(const uiStringSet&);
    void		fill(mQtclass(QStringList)&) const;
    void		use(const mQtclass(QStringList)&);

    BufferString	cat(const char* sepstr="\n") const;
    void		unCat(const char*,const char* sepstr="\n");

    BufferString	getDispString(size_type maxnritems=-1,
				      bool quoted=true) const;

    // uncommon stuff
    BufferString*	operator[]( idx_type idx )	 { return strs_[idx]; }
    const BufferString*	operator[]( idx_type idx ) const { return strs_[idx]; }
    const SetType&	getStringSet() const		 { return strs_; }
    SetType&		getStringSet()			 { return strs_; }
    uiStringSet		getUiStringSet() const;

    void		setNullAllowed( bool yn=true )
				{ strs_.setNullAllowed( yn ); }
    BufferStringSet&	operator +=( BufferString* bs )	{ return add(bs); }
    BufferStringSet&	set( idx_type idx, BufferString* bs )
				{ strs_.replace(idx,bs); return *this; }
    void		insertAt( BufferString* bs, idx_type idx )
				{ strs_.insertAt(bs,idx); }

    // remainder of OD::Set interface

    virtual od_int64	nrItems() const		{ return size(); }
    virtual void	swapItems( od_int64 i1, od_int64 i2 )
			{ swap( idx_type(i1), idx_type(i2) ); }
    virtual void	reverse()		{ strs_.reverse(); }

};

mDefContainerSwapFunction( Basic, BufferStringSet )

inline BufferString toString( const BufferStringSet& bss )
{ return bss.cat( " " ); }
