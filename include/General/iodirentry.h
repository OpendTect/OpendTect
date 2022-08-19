#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "multiid.h"
#include "namedobj.h"
#include "objectset.h"

class BufferStringSet;
class IODir;
class IOObj;
class IOObjContext;
class TranslatorGroup;

/*!\brief needed for manipulation. Used by user interface IOObj management. */

mExpClass(General) IODirEntry : public NamedObject
{
public:
			IODirEntry(const IOObj*);
    const IOObj*	ioobj_;
};


/*!\brief list of dir entries. */

mExpClass(General) IODirEntryList : public ObjectSet<IODirEntry>
{
public:
			IODirEntryList(const IODir&,const IOObjContext&);
			//!<IODir is expected to remain alive
			IODirEntryList(const IODir&,const TranslatorGroup*,
					bool maychgdir,
					const char* translator_globexpr=0);
			//!<IODir is expected to remain alive
			~IODirEntryList();
    const char*		name() const	{ return name_; }

    void		fill(const IODir&,const char* nmfiltglobexpr=0);
			//!<IODir is expected to remain alive
    void		setSelected(const MultiID&);
    void		sort();
    void		setCurrent( int idx )	{ cur_ = idx; }
    const IODirEntry*	current() const
    			{ return cur_ < 0 || cur_ >= size() ? 0
			    	: (*(IODirEntryList*)this)[cur_]; }
    const IOObj*	selected() const
			{ return current() ? current()->ioobj_ : 0 ; }
    void		removeWithTranslator(const char*);
    int			indexOf(const char*) const;
    int			indexOf( const IODirEntry* e ) const override
			{ return ObjectSet<IODirEntry>::indexOf(e); }
    void		getIOObjNames(BufferStringSet&) const;
    BufferStringSet	getValuesFor( const char* key ) const;
	//!<key can be either the translator group name or an omf metadata key

    MultiID		lastiokey;
    IOObjContext&	ctxt;

protected:

    int			cur_;
    bool		maycd_;
    BufferString	name_;

};
