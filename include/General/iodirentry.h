#ifndef iodirentry_h
#define iodirentry_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: iodirentry.h,v 1.5 2003-10-15 15:15:53 bert Exp $
________________________________________________________________________

-*/
 
#include "multiid.h"
#include "uidobjset.h"
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
    bool		beingsorted;

};


/*!\brief list of dir entries. */

class IODirEntryList : public UserIDObjectSet<IODirEntry>
{
public:
			IODirEntryList(IODir*,const IOObjContext&);
			IODirEntryList(IODir*,const TranslatorGroup*,bool,
					const char* translator_globexpr=0);
			~IODirEntryList();

    void		fill(IODir*);
    void		setSelected(const MultiID&);
    void		curRemoved();
    IOObj*		selected();
    bool		mustChDir();
    bool		canChDir();
    void		sort();

    MultiID		lastiokey;
    IOObjContext&	ctxt;

};


#endif
