#ifndef bufstringset_h
#define bufstringset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Oct 2003
 Contents:	Set of BufferStrings
 RCS:		$Id: bufstringset.h,v 1.10 2008-02-29 10:30:50 cvsnanne Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "sets.h"

class IOPar;

/*!\brief Set of BufferString objects
 
  The default is that the set owns the strings, in which case the strings
  are automatically deleted on destruction of the set.

 */

class BufferStringSet : public ObjectSet<BufferString>
{
public:
    			BufferStringSet( bool ownr=true ) : owner_(ownr) {}
			BufferStringSet(const char* arr[],int len=-1);
			BufferStringSet(const BufferStringSet&);
	    		~BufferStringSet()	{ if ( owner_ ) deepErase(); }
    void		deepErase()		{ ::deepErase(*this); }
    BufferStringSet&	operator =(const BufferStringSet&);
    bool		operator ==(const BufferStringSet&) const;

    BufferStringSet&	add( const char* s )
    			{ *this += new BufferString(s); return *this; }
    bool		addIfNew(const char*);	//!< returns whether added
    void		add(const BufferStringSet&,bool allowduplicates);
    						//!< makes copies
    BufferString&	get( int idx )		{ return *((*this)[idx]); }
    const BufferString&	get( int idx ) const	{ return *((*this)[idx]); }
    int			indexOf(const char*) const; //!< returns -1 if not found

    inline int		maxLength() const;
    void		sort(BufferStringSet* slave=0);
    int*		getSortIndexes() const; //!< returns new int [size()]

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    // Overriding ObjectSet's methods - different in case of being owner
    virtual void	erase();	// becomes deepErase if owner
    virtual BufferString* remove(int);	// etc.
    virtual void	remove(int,int);

    bool		isOwner() const		{ return owner_; }
    void		setIsOwner( bool yn )	{ owner_ = yn; }

protected:

    bool		owner_;

};


class NamedBufferStringSet : public BufferStringSet
{
public:
    			NamedBufferStringSet( const char* nm=0, bool ownr=true )
			    : BufferStringSet(ownr)
			    , name_(nm)			{}

    const BufferString&	name() const			{ return name_; }
    void		setName( const char* nm )	{ name_ = nm; }

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

protected:

    BufferString	name_;

};


inline int BufferStringSet::maxLength() const
{
    int ret = 0;
    for ( int idx=0; idx<size(); idx++ )
    {
	const int len = get(idx).size();
	if ( len > ret )
	    ret = len;
    }
    return ret;
}


#endif
