#ifndef iodirentry_h
#define iodirentry_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: iodirentry.h,v 1.6 2003-10-17 14:19:01 bert Exp $
________________________________________________________________________

-*/
 
#include "multiid.h"
#include "uidobj.h"
#include "sets.h"
class IOObj;
class IODir;
class IOObjContext;
class TranslatorGroup;

/*!\brief needed for manipulation. Used by user interface IOObj management. */

class IODirEntry : public UserIDObject
{
public:
			IODirEntry(IOObj*,int,bool);
    const UserIDString&	name() const;

    IOObj*		ioobj;
    static bool		beingsorted;

};


/*!\brief list of dir entries. */

class IODirEntryList : public ObjectSet<IODirEntry>
		     , public UserIDObject
{
public:
			IODirEntryList(IODir*,const IOObjContext&);
			IODirEntryList(IODir*,const TranslatorGroup*,bool,
					const char* translator_globexpr=0);
			~IODirEntryList();

    void		fill(IODir*);
    void		setSelected(const MultiID&);
    bool		mustChDir();
    bool		canChDir();
    void		sort();
    void		curRemoved();
    void		setCurrent( int idx )	{ cur_ = idx; }
    IODirEntry*		current() const	
    			{ return cur_ < 0 || cur_ >= size() ? 0
			    	: (*(IODirEntryList*)this)[cur_]; }
    IOObj*		selected()
			{ return current() ? current()->ioobj : 0 ; }
    int			indexOf(const char*) const;

    MultiID		lastiokey;
    IOObjContext&	ctxt;

protected:

    int			cur_;

};


#endif
