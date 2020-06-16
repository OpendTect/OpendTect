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
class GlobExpr;
class uiString;
class uiStringSet;
mFDQtclass( QString )
mFDQtclass( QStringList )

/*!
\brief Set of BufferString objects.
*/

mExpClass(Basic) BufferStringSet : public ManagedObjectSet<BufferString>
{
public:

			BufferStringSet(int n=0,const char* s=0);
			BufferStringSet(const char* arr[],int len=-1);
    bool		operator ==(const BufferStringSet&) const;

    BufferString&	get( int idx )		{ return *((*this)[idx]); }
    const BufferString&	get( int idx ) const	{ return *((*this)[idx]); }
    int			indexOf(const char*) const;	//!< first match
    int			indexOf(const GlobExpr&) const;	//!< first match
    int			indexOf( const BufferString* b ) const
				{ return ObjectSet<BufferString>::indexOf(b); }
    inline bool		isPresent( const BufferString* b ) const
				{ return ObjectSet<BufferString>::isPresent(b);}
    inline bool		isPresent( const char* s ) const
				{ return indexOf(s) >= 0; }
    int			nearestMatch(const char*,bool caseinsens=true) const;
    bool		isSubsetOf(const BufferStringSet&) const;

    BufferStringSet&	add(const char*);
    BufferStringSet&	add(const OD::String&);
    BufferStringSet&	add(const mQtclass(QString)&);
    BufferStringSet&	add(const BufferStringSet&,bool allowduplicates);
    BufferStringSet&	add(const char* arr[],int len=-1);
    BufferStringSet&	addToAll(const char*,bool infront=false);
    bool		addIfNew(const char*);	//!< returns whether added
    bool		addIfNew(const OD::String&);

    int			maxLength() const;
    void		sort(bool caseinsens=true,bool asc=true);
    BufferString	commonStart() const;

    int*		getSortIndexes(bool caseinsns=true,bool asc=true) const;
			//!< returns new int [size()] for you to 'delete []'
			//!< does NOT sort but provides data for useIndexes
    void		useIndexes(const int*);

    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);

    void		fill(uiStringSet&) const;
    void		use(const uiStringSet&);
    void		fill(mQtclass(QStringList)&) const;
    void		use(const mQtclass(QStringList)&);

    BufferString	cat(const char* sepstr="\n") const;
    void		unCat(const char*,const char* sepstr="\n");

    BufferString	getDispString(int maxnritems=-1,bool quoted=true) const;

};


#endif
