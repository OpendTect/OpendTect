#ifndef bufstringset_h
#define bufstringset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Oct 2003
 Contents:	Set of BufferStrings
 RCS:		$Id: bufstringset.h,v 1.14 2009-04-14 05:06:54 cvsranojay Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "manobjectset.h"

 class IOPar;

/*!\brief Set of BufferString objects */

mClass BufferStringSet : public ManagedObjectSet<BufferString>
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
    int			indexOf(const char*) const;
    inline bool		isPresent( const char* s ) const
    						{ return indexOf(s) >= 0; }

    BufferStringSet&	add(const char*);
    BufferStringSet&	add(const BufferString&);
    BufferStringSet&	add(const BufferStringSet&,bool allowduplicates);
    bool		addIfNew(const char*);	//!< returns whether added
    bool		addIfNew(const BufferString&);

    int			maxLength() const;
    void		sort(BufferStringSet* slave=0);
    int*		getSortIndexes() const; //!< returns new int [size()]

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

};


#endif
