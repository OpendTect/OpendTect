#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
________________________________________________________________________

-*/

#include "generalmod.h"
#include "dbkey.h"
#include "objectset.h"
class IOObj;
class IODir;
class IOObjContext;
class TranslatorGroup;


/*!\brief list of IODir entries, sorted by name, conforming to a context.
    Can be Filtered using GlobExpr. */

mExpClass(General) IODirEntryList
{
public:

    typedef ObjectSet<IOObj>::size_type	size_type;
    typedef size_type			IdxType;

			IODirEntryList(const IOObjContext&);
					//!< empty; needs to be filled
			IODirEntryList(const IODir&,const IOObjContext&);
			IODirEntryList(const IODir&,const TranslatorGroup*,
					const char* translator_globexpr=0);
			~IODirEntryList();
    const char*		name() const	{ return name_; }
    size_type		size() const	{ return entries_.size(); }
    bool		isEmpty() const	{ return entries_.isEmpty(); }

    void		fill(const IODir&,const char* nmfiltglobexpr=0);
    IdxType		indexOf(const char*) const;

    const IOObj&	ioobj( IdxType idx ) const { return *entries_[idx]; }
    DBKey		key(IdxType) const;
    BufferString	name(IdxType) const;
    BufferString	dispName(IdxType) const;
    BufferString	iconName(IdxType) const;

protected:

    ObjectSet<IOObj>	entries_;
    IOObjContext&	ctxt_;
    BufferString	name_;

    void		sort();

};
