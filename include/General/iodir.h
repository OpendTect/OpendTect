#ifndef iodir_H
#define iodir_H

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		31-7-1995
________________________________________________________________________

-*/


#include "generalmod.h"

#include "multiid.h"
#include "objectset.h"
#include "namedobj.h"
#include "od_iosfwd.h"
#include "threadlock.h"
#include "uistring.h"
class IOObj;
class IOObjContext;
class IOObjSelConstraints;


/*\brief 'Directory' of IOObj objects.

The IODir class is responsible for finding all IOObj's in the system. An IODir
instance will actually load all IOObj's, provides access to keys and allows
searching. It has a key of its own.

Few operation are done through the IODir directly: usually, IOMan will be
the service access point.

*/


mExpClass(General) IODir : public NamedObject
{ mODTextTranslationClass(IODir);
public:

    typedef ObjectSet<IOObj>::size_type	size_type;
    typedef MultiID::SubID		SubID;

			IODir(const char*);
			IODir(const MultiID&);
			IODir(const IODir&);
			~IODir();
    IODir&		operator =(const IODir&);

    bool		isBad() const;
    const MultiID&	key() const		{ return key_; }
    const char*		dirName() const		{ return dirname_; }
    const IOObj*	main() const;

    size_type		size() const;
    size_type		isEmpty() const		{ return size() < 1; }
    const IOObj*	get(size_type) const;
    const ObjectSet<IOObj>& getObjs() const	{ return objs_; }
			//!< Use only when MT is not an issue

    bool		isPresent(const MultiID&) const;
    size_type		indexOf(const MultiID&) const;
    const IOObj*	get(const MultiID&) const;
    const IOObj*	get(const char* nm,const char* trgrpnm=0) const;
				//!< Without trgrpnm, just returns first

    bool		commitChanges(const IOObj*);
    bool		permRemove(const MultiID&);
    bool		ensureUniqueName(IOObj&) const;

    static MultiID	getNewTmpKey(const IOObjContext&);
    static IOObj*	getObj(const MultiID&,uiString& errmsg);
    static IOObj*	getMain(const char*,uiString& errmsg);
    MultiID		newTmpKey() const;

    uiString		errMsg() const		{ return errmsg_; }

private:

    bool		isok_;
    ObjectSet<IOObj>	objs_;
    const BufferString	dirname_;
    const MultiID	key_;
    mutable SubID	curid_;
    mutable SubID	curtmpid_;
    mutable uiString	errmsg_;
    mutable Threads::Lock lock_;

			IODir();

			// No locks, lock if necessary
    void		doReRead();
    static IOObj*	doRead(const char*,IODir*,uiString& errmsg,SubID,
	    			bool incoldtmps=false);
    static IOObj*	readOmf(od_istream&,const char*,IODir*,SubID,bool);

    static void		setDirName(IOObj&,const char*);

    void		init(const MultiID&,bool);
    bool		build(bool);
    bool		doAddObj(IOObj*,bool);
    bool		doWrite() const;
    bool		wrOmf(od_ostream&) const;
    const IOObj*	doGet(const MultiID&) const;
    const IOObj*	doGet(const char*,const char*) const;
    size_type		gtIdxOf(const MultiID&) const;
    bool		doEnsureUniqueName(IOObj&) const;

    MultiID		gtNewKey(const size_type&) const;
    MultiID		newKey() const;		//!< locked, as it's 'public'

    friend class	IOMan;
    friend class	IOObj;

public:

    // 'Usually not needed' section

    void		reRead();
				// Done a lot already
    bool		writeNow() const;
				// write is automatic after commit or remove

    bool		addObj(IOObj*,bool immediate_store=true);
				// usually done by IOM()
				//!< after call, IOObj is mine
    static void		getTmpIOObjs(const MultiID&,ObjectSet<IOObj>&,
	    				const IOObjSelConstraints* c=0);

};


#endif
