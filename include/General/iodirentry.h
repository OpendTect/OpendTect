#ifndef iodirentry_h
#define iodirentry_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Bril
 Date:          April 2001
 RCS:           $Id: iodirentry.h,v 1.1 2001-04-27 16:48:51 bert Exp $
________________________________________________________________________

-*/
 
#include <multiid.h>
#include <selector.h>
#include <uidobjset.h>
class Translator;
class IOObj;
class IODir;

/*!\brief needed for manipulation. Used by user interface IOObj management.

*/

class IODirEntry : public UserIDObject
{
public:
			IODirEntry(IOObj*,int,bool);
    const UserIDString&	name() const;

    IOObj*		ioobj;
    bool		beingsorted;

};


class IODirEntryList : public UserIDObjectSet<IODirEntry>
{
public:
			IODirEntryList(IODir*,const Translator*,bool);
			~IODirEntryList();

    void		fill(IODir*);
    void		setSelected(const MultiID&);
    void		curRemoved();
    IOObj*		selected();
    bool		mustChDir();
    bool		canChDir();
    void		sort();

    MultiID			lastiokey;
    ObjectTypeSelectionFun	trsel;
    bool			maychgdir;

};


#endif
