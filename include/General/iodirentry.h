#ifndef iodirentry_h
#define iodirentry_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: iodirentry.h,v 1.14 2010-12-13 12:34:11 cvsbert Exp $
________________________________________________________________________

-*/
 
#include "multiid.h"
#include "namedobj.h"
#include "objectset.h"
class IOObj;
class IODir;
class IOObjContext;
class TranslatorGroup;

/*!\brief needed for manipulation. Used by user interface IOObj management. */

mClass IODirEntry : public NamedObject
{
public:
			IODirEntry(IOObj*);
    IOObj*		ioobj;

};


/*!\brief list of dir entries. */

mClass IODirEntryList : public ObjectSet<IODirEntry>
{
public:
			IODirEntryList(IODir*,const IOObjContext&);
			IODirEntryList(IODir*,const TranslatorGroup*,
					bool maychgdir,
					const char* translator_globexpr=0);
			~IODirEntryList();
    const char*		name() const	{ return name_; }

    void		fill(IODir*,const char* nmfiltglobexpr=0);
    void		setSelected(const MultiID&);
    void		sort();
    void		setCurrent( int idx )	{ cur_ = idx; }
    IODirEntry*		current() const	
    			{ return cur_ < 0 || cur_ >= size() ? 0
			    	: (*(IODirEntryList*)this)[cur_]; }
    IOObj*		selected()
			{ return current() ? current()->ioobj : 0 ; }
    int			indexOf(const char*) const;
    void		removeWithTranslator(const char*);

    MultiID		lastiokey;
    IOObjContext&	ctxt;

protected:

    int			cur_;
    bool		maycd_;
    BufferString	name_;

};


#endif
