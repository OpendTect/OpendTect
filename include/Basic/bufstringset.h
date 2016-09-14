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
class QString;

/*!\brief Set of BufferString objects. */

mExpClass(Basic) BufferStringSet
{
public:

    typedef ObjectSet<BufferString>	SetType;
    typedef SetType::size_type		size_type;

			BufferStringSet(size_type n=0,const char* s=0);
			BufferStringSet(const char* arr[],size_type len=-1);
    virtual		~BufferStringSet()	{}
    bool		operator ==(const BufferStringSet&) const;

    inline size_type	size() const		{ return strs_.size(); }
    inline bool		isEmpty() const		{ return strs_.isEmpty(); }
    inline bool		validIdx( size_type i ) const
						{ return strs_.validIdx(i); }
    size_type		indexOf(const char*) const;	//!< first match
    size_type		indexOf(const GlobExpr&) const;	//!< first match
    size_type		indexOf( const BufferString* b ) const
						{ return strs_.indexOf(b); }
    inline bool		isPresent( const BufferString* b ) const
						{ return strs_.isPresent(b);}
    inline bool		isPresent( const char* s ) const
						{ return indexOf(s) >= 0; }
    BufferString&	get( size_type idx )	{ return *strs_.get(idx); }
    const BufferString&	get( size_type idx ) const { return *strs_.get(idx); }
    BufferString*	first()			{ return strs_.first(); }
    const BufferString*	first() const		{ return strs_.first(); }
    BufferString*	last()			{ return strs_.last(); }
    const BufferString*	last() const		{ return strs_.last(); }

    inline void		setEmpty()		{ strs_.setEmpty(); }
    inline void		erase()			{ setEmpty(); }
    void		removeSingle( size_type i ) { strs_.removeSingle(i); }
    void		removeRange( size_type i1, size_type i2 )
						{ strs_.removeRange(i1,i2); }

    BufferStringSet&	add(const char*);
    BufferStringSet&	add(const OD::String&);
    BufferStringSet&	add(const QString&);
    BufferStringSet&	add(const BufferStringSet&,bool allowduplicates);
    BufferStringSet&	add(const char* arr[],size_type len=-1);
    BufferStringSet&	add( BufferString* bs )	{ strs_.add(bs); return *this; }
    BufferStringSet&	addToAll(const char*,bool infront=false);
    bool		addIfNew(const char*);	//!< returns whether added
    bool		addIfNew(const OD::String&);
    void		append( const BufferStringSet& oth )
			{ strs_.append( oth.strs_ ); }

    size_type		nearestMatch(const char*,bool caseinsens=true) const;
    bool		isSubsetOf(const BufferStringSet&) const;
    size_type		maxLength() const;

    void		sort(bool caseinsens=true,bool asc=true);
    size_type*		getSortIndexes(bool caseinsns=true,bool asc=true) const;
			//!< returns new int [size()] for you to 'delete []'
			//!< does NOT sort but provides data for useIndexes
    void		useIndexes(const size_type*);

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    void		fill(uiStringSet&) const;
    void		use(const uiStringSet&);

    BufferString	cat(const char* sepstr="\n") const;
    void		unCat(const char*,const char* sepstr="\n");

    BufferString	getDispString(size_type maxnritems=-1,
				      bool quoted=true) const;

    // uncommon stuff
    BufferString*	operator[]( size_type idx )	  { return strs_[idx]; }
    const BufferString*	operator[]( size_type idx ) const { return strs_[idx]; }
    const SetType&	getStringSet() const	    { return strs_; }
    SetType&		getStringSet()		    { return strs_; }
    void		allowNull( bool yn=true )   { strs_.allowNull( yn ); }
    BufferStringSet&	operator +=( BufferString* bs )	{ return add(bs); }
    BufferStringSet&	set( size_type idx, BufferString* bs )
				{ strs_.replace(idx,bs); return *this; }
    void		insertAt( BufferString* bs, size_type idx )
				{ strs_.insertAt(bs,idx); }

protected:

    ManagedObjectSet<BufferString>  strs_;

};


