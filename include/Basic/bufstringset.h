#ifndef bufstringset_h
#define bufstringset_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Oct 2003
 Contents:	Set of BufferStrings
 RCS:		$Id: bufstringset.h,v 1.12 2009-02-13 13:31:14 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "manobjectset.h"

mClass IOPar;

/*!\brief Set of BufferString objects
 
  The default is that the set owns the strings, in which case the strings
  are automatically deleted on destruction of the set.

 */

mClass BufferStringSet : public ManagedObjectSet<BufferString>
{
public:
    			BufferStringSet();
			BufferStringSet(const char* arr[],int len=-1);
    BufferStringSet&	operator =(const BufferStringSet&);
    bool		operator ==(const BufferStringSet&) const;

    BufferString&	get( int idx )		{ return *((*this)[idx]); }
    const BufferString&	get( int idx ) const	{ return *((*this)[idx]); }
    int			indexOf(const char*) const;

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
