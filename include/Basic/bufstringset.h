#ifndef bufstringset_h
#define bufstringset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Oct 2003
 Contents:	Set of BufferStrings
 RCS:		$Id: bufstringset.h,v 1.2 2003-11-07 12:21:50 bert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "sets.h"


/*!\brief Set of BufferString objects
 
  The default is that the set owns the strings, in which case the strings
  are automatically deleted on destruction of the set.

 */

class BufferStringSet : public ObjectSet<BufferString>
{
public:
    			BufferStringSet( bool ownr=true )
			    : owner_(ownr)	{}
			BufferStringSet(const char* arr[],int len=0);
			BufferStringSet(const BufferStringSet&);
	    		~BufferStringSet()	{ if ( owner_ ) deepErase(); }
    void		deepErase()		{ ::deepErase(*this); }
    BufferStringSet&	operator =(const BufferStringSet&);
    bool		operator ==(const BufferStringSet&) const;

    bool		isOwner() const		{ return owner_; }
    void		setIsOwner( bool yn )	{ owner_ = yn; }
    void		add( const char* s )	{ *this += new BufferString(s);}
    bool		addIfNew(const char*);	//!< returns whether added
    BufferString&	get( int idx )		{ return *((*this)[idx]); }
    const BufferString&	get( int idx ) const	{ return *((*this)[idx]); }
    void		sort(bool asc=true);
    int			indexOf(const char*) const;
    						//!< returns -1 if not found

    // Overriding ObjectSet's methods - different in case of being owner
    virtual void	erase();	// becomes deepErase if owner
    virtual void	remove(int);	// etc.
    virtual void	remove(int,int);

protected:

    bool		owner_;

};


class NamedBufferStringSet : public BufferStringSet
{
public:
    			NamedBufferStringSet( const char* nm=0,bool ownr=true )
			    : BufferStringSet(ownr)
			    , name_(nm)			{}

    const BufferString&	name() const			{ return name_; }
    void		setName( const char* nm )	{ name_ = nm; }

protected:

    BufferString	name_;

};


#endif
