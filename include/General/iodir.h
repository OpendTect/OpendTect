#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "multiid.h"
#include "objectset.h"
#include "namedobj.h"
#include "od_iosfwd.h"

#include <unordered_map>

class DBKey;
class IOObj;


/*\brief 'Directory' of IOObj objects.

The IODir class is responsible for finding all IOObj's in the system. An IODir
instance will actually load all IOObj's, provides access to keys and allows
searching. It has a key of its own.

Few operation are done through the IODir directly: usually, IOMan will be
the service access point.

*/


mExpClass(General) IODir : public NamedCallBacker
{
public:
			IODir(const char*);
			IODir(const MultiID&);
			~IODir();

    void		reRead();
    bool		isBad() const		{ return !isok_; }
    const MultiID&	key() const		{ return key_; }

    const IOObj*	main() const;
    const char*		dirName() const		{ return dirname_; }

    int			size() const		{ return objs_.size(); }
    int			isEmpty() const		{ return objs_.isEmpty(); }
    const IOObj*	get( int idx ) const	{ return objs_[idx]; }
    const ObjectSet<IOObj>& getObjs() const	{ return objs_; }

    bool		isPresent(const MultiID&) const;
    int			indexOf(const MultiID&) const;
    const IOObj*	get(const MultiID&) const;
    const IOObj*	get(const char* objnm,
			    const char* trgrpnm=nullptr) const;
			// Without trgrpnm, just returns first

    bool		addObj(IOObj*,bool immediate_store=true);
			    //!< after call, IOObj is mine
    bool		addObjects(ObjectSet<IOObj>&,bool immediate_store=true);
    bool		commitChanges(const IOObj*);
			    //!< after call, assume pointer will be invalid
    bool		commitChanges(const ObjectSet<const IOObj>&);
    bool		permRemove(const MultiID&);
    bool		permRemove(const TypeSet<MultiID>&);
    bool		ensureUniqueName(IOObj&);

    static IOObj*	getObj(const DBKey&);
    static IOObj*	getObj(const MultiID&);
    static IOObj*	getMain(const char*);

			// Use this if you know there's no contingency
			// Therefore, only in special-purpose programs
    bool		doWrite() const;

    bool		hasObjectsWithGroup(const char* trgrpnm) const;
    MultiID		getNewKey() const;

private:

    ObjectSet<IOObj>	objs_;
    BufferString	dirname_;
    MultiID		key_;
    bool		isok_		= false;
    mutable int		curid_		= 0;
    mutable od_int64	lastmodtime_	= 0;

			IODir();
    static IOObj*	doRead(const char*,IODir*,int id=-1);
    static void		setDirName(IOObj&,const char*);
    static IOObj*	readOmf(od_istream&,const char*,IODir*,int);
    static IOObj*	getIOObj(const char* dirnm,const MultiID&);

    bool		build();
    bool		wrOmf(od_ostream&) const;
    IOObj*		get( int idx )		{ return objs_[idx]; }
    IOObj*		get(const MultiID&);
    void		addObjectNoWrite(IOObj*);
    void		addObjNoChecks(IOObj*);

    MultiID		newKey() const;

    friend class	IOMan;
    friend class	IOObj;

    std::unordered_map<int,int>		objidmap_;

public:

    void		update();
};
