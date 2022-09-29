#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"
#include "manobjectset.h"
class GlobExpr;
class od_ostream;
class uiString;
class uiStringSet;
class QString;
template <class T> class QList;

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
    BufferStringSet*	clone() const override
			{ return new BufferStringSet(*this); }
    virtual		~BufferStringSet()	{}
    bool		operator ==(const BufferStringSet&) const;
    bool		operator !=(const BufferStringSet&) const;

    inline size_type	size() const		{ return strs_.size(); }
    inline bool		isEmpty() const		{ return strs_.isEmpty(); }
    bool		validIdx( od_int64 i ) const override
						{ return strs_.validIdx(i); }
    idx_type		indexOf(const char*,
				OD::CaseSensitivity s=OD::CaseSensitive) const;
			//!< first match
    idx_type		indexOf(const GlobExpr&) const;	//!< first match
    idx_type		indexOf( const BufferString* b ) const
						{ return strs_.indexOf(b); }
    inline bool		isPresent( const BufferString* b ) const
						{ return strs_.isPresent(b);}
    inline bool		isPresent( const char* s,
				OD::CaseSensitivity c=OD::CaseSensitive) const
						{ return indexOf(s,c) >= 0; }
    BufferString&	get( idx_type idx )	{ return *strs_.get(idx); }
    const BufferString&	get( idx_type idx ) const { return *strs_.get(idx); }
    BufferString*	first()			{ return strs_.first(); }
    const BufferString*	first() const		{ return strs_.first(); }
    BufferString*	last()			{ return strs_.last(); }
    const BufferString*	last() const		{ return strs_.last(); }

    inline void		setEmpty()		{ strs_.setEmpty(); }
    void		erase() override	{ setEmpty(); }
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

    idx_type		nearestMatch(const char*,
			    OD::CaseSensitivity cs=OD::CaseInsensitive) const;
    idx_type		nearestMatch(const char*,bool caseinsens) const;
    TypeSet<idx_type>	getMatches(const char* globexpr,
			    OD::CaseSensitivity cs=OD::CaseInsensitive) const;
    bool		isSubsetOf(const BufferStringSet&) const;
    size_type		maxLength() const;
    idx_type		firstDuplicateOf(idx_type,
				OD::CaseSensitivity cs=OD::CaseSensitive,
				idx_type startat=0) const;
    bool		hasUniqueNames(
				OD::CaseSensitivity cs=OD::CaseSensitive) const;
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
    void		fill(QList<QString>&) const;
    void		use(const QList<QString>&);

    BufferString	cat(const char* sepstr="\n") const;
    void		unCat(const char*,const char* sepstr="\n");

    BufferString	getDispString(size_type maxnritems=-1,
				      bool quoted=true) const;

    void		removeSingle( idx_type i, bool ) { removeSingle(i); }

    // uncommon stuff
    BufferString*	operator[]( idx_type idx )	 { return strs_[idx]; }
    const BufferString*	operator[]( idx_type idx ) const { return strs_[idx]; }
    const SetType&	getStringSet() const		 { return strs_; }
    SetType&		getStringSet()			 { return strs_; }
    uiStringSet		getUiStringSet() const;

    void		setNullAllowed( bool yn=true )
				{ strs_.setNullAllowed( yn ); }
    BufferStringSet&	operator +=( BufferString* bs )	{ return add(bs); }
    BufferStringSet&	replace( idx_type idx, BufferString* bs )
			{ set( idx, bs ); return *this; }
    BufferStringSet&	set( idx_type idx, BufferString* bs )
				{ strs_.replace(idx,bs); return *this; }
    void		insertAt( BufferString* bs, idx_type idx )
				{ strs_.insertAt(bs,idx); }

    // remainder of OD::Set interface

    od_int64		nrItems() const override	{ return size(); }
    void		swapItems( od_int64 i1, od_int64 i2 ) override
			{ swap( idx_type(i1), idx_type(i2) ); }
    void		reverse() override		{ strs_.reverse(); }

public:

    mDeprecated("Use a set")
    BufferStringSet&	operator=(const char* arr[]);
    mDeprecated("Use equal")
    BufferStringSet&	copy(const BufferStringSet&);

    inline void		allowNull(bool yn=true)	{ strs_.setNullAllowed(yn); }

};

mDefContainerSwapFunction( Basic, BufferStringSet )

inline BufferString toString( const BufferStringSet& bss )
{ return bss.cat( " " ); }


mDeprecated("Use setEmpty")
mGlobal(Basic) void deepErase(BufferStringSet&);
mDeprecated("Use equal")
mGlobal(Basic) void deepCopy(BufferStringSet&,const BufferStringSet&);
mDeprecated("Use sort")
mGlobal(Basic) void sort(BufferStringSet&);
mDeprecated("Use getStringSet()")
mGlobal(Basic) const BufferString* find(const BufferStringSet&,const char*);


mExpClass(Basic) StringPairSet
{ mIsContainer( StringPairSet, ManagedObjectSet<StringPair>, entries_ )
public:

			StringPairSet()		{}
    virtual		~StringPairSet()	{}

    inline int		size() const		{ return entries_.size(); }
    inline bool		isEmpty() const		{ return entries_.isEmpty(); }
    inline void		setEmpty()		{ entries_.setEmpty(); }
    bool		validIdx( int i ) const
						{ return entries_.validIdx(i); }

    StringPair&	get( int idx )	{ return *entries_.get(idx); }
    const StringPair&	get( int idx ) const { return *entries_.get(idx); }

    StringPairSet&	add(const char*,const char*);
    StringPairSet&	add(const char*,int);
    StringPairSet&	add(const char*,const OD::String&);
    StringPairSet&	add(const OD::String&,const OD::String&);
    StringPairSet&	add(const StringPair&);
    StringPairSet&	add(const StringPairSet&);

    bool		remove(const char* first);
    void		removeSingle( int i ) { entries_.removeSingle(i); }

    int			indexOf(const char* first) const;
    			//!< first match
    inline bool		isPresent(const char* first) const
						{ return indexOf(first) >= 0; }

    void		dumpPretty(od_ostream&) const;
    void		dumpPretty(BufferString&) const;
};

