#ifndef bufstringset_h
#define bufstringset_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Oct 2003
 Contents:	Set of BufferStrings
 RCS:		$Id$
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstring.h"
#include "manobjectset.h"
class IOPar;
class GlobExpr;


/*!\brief Set of BufferString objects */

mClass(Basic) BufferStringSet : public ManagedObjectSet<BufferString>
{
public:
    			BufferStringSet();
			BufferStringSet(const char* arr[],int len=-1);
    			BufferStringSet( const BufferStringSet& bss )
			    : ManagedObjectSet<BufferString>(false)
						{ *this = bss; }
    BufferStringSet&	operator =(const BufferStringSet&);
    bool		operator ==(const BufferStringSet&) const;

    BufferString&	get( int idx )		{ return *((*this)[idx]); }
    const BufferString&	get( int idx ) const	{ return *((*this)[idx]); }
    int			indexOf(const char*) const;	//!< first match
    int			indexOf(const GlobExpr&) const;	//!< first match
    int			indexOf( const BufferString* b ) const
				{ return ObjectSet<BufferString>::indexOf(b); }
    inline bool		isPresent( const BufferString* b ) const
				{ return ObjectSet<BufferString>::isPresent(b); }
    inline bool		isPresent( const char* s ) const
				{ return indexOf(s) >= 0; }
    int			nearestMatch(const char*,bool caseinsens=true) const;
			    //!< algo may not be very good, but anyway
			    //!< returns -1 if size is 0
    bool		isSubsetOf(const BufferStringSet&) const;

    BufferStringSet&	add(const char*);
    BufferStringSet&	add(const BufferString&);
    BufferStringSet&	add(const BufferStringSet&,bool allowduplicates);
    bool		addIfNew(const char*);	//!< returns whether added
    bool		addIfNew(const BufferString&);

    int			maxLength() const;
    void		sort(bool caseinsens=true,bool asc=true);
    int*		getSortIndexes(bool caseinsns=true,bool asc=true) const;
    			//!< returns new int [size()] for you to 'delete []'
    			//!< does NOT sort!! you should do useIndexes afterwards
    void		useIndexes(const int*);

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    BufferString	cat(char sepchar='\n') const;
    void		unCat(const char*,char sepchar='\n');

};


#endif

