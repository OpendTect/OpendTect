#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		31-7-1995
________________________________________________________________________

-*/


#include "generalmod.h"
#include "sharedobject.h"
#include "monitoriter.h"
#include "dbkey.h"
#include "objectset.h"
#include "od_iosfwd.h"
#include "uistring.h"
#include "ioobjctxt.h"


/*\brief 'Directory' of IOObj objects.

The IODir class is responsible for finding all IOObj's in the system. An IODir
instance will actually load all IOObj's, provides access to keys and allows
searching. It has a key of its own.

Few operation are done through the IODir directly: usually, IOMan will be
the service access point.

*/


mExpClass(General) IODir : public SharedObject
{ mODTextTranslationClass(IODir);
public:

    typedef ObjectSet<IOObj>::size_type	size_type;
    typedef size_type			IdxType;
    typedef DBKey::ObjNrType		ObjNrType;
    typedef DBKey::DirID		DirID;
    typedef DBKey::ObjID		ObjID;

			IODir(const char* dirname);
			IODir(DirID);
			IODir(IOObjContext::StdSelType);
			~IODir();
			mDeclMonitorableAssignment(IODir);

    bool		isBad() const;
    bool		isOutdated() const;
    DirID		dirID() const		{ return dirid_; }
    const char*		dirName() const		{ return dirname_; }
    od_int64		readTime() const	{ return readtime_; }

    size_type		size() const;
    bool		isEmpty() const		{ return size() < 1; }

    bool		isPresent(const DBKey&) const;
    IdxType		indexOf(const DBKey&) const;


    IOObj*		getEntry(const DBKey&) const;
    IOObj*		getEntryByName(const char* nm,
					const char* trgrpnm=0) const;
    IOObj*		getEntryByIdx(IdxType) const;

    bool		commitChanges(const IOObj*);
    bool		permRemove(const DBKey&);
    bool		ensureUniqueName(IOObj&) const;

    static DBKey	getNewTmpKey(const IOObjContext&);
    static IOObj*	getObj(const DBKey&,uiString& errmsg);
    // static IOObj*	getMain(const char*,uiString& errmsg);
    DBKey		newTmpKey() const;

    uiString		errMsg() const		{ return errmsg_; }

private:

    bool		isok_;
    ObjectSet<IOObj>	objs_;
    const DirID		dirid_;
    const BufferString	dirname_;
    od_int64		readtime_;
    mutable ObjNrType	curnr_;
    mutable ObjNrType	curtmpnr_;
    mutable uiString	errmsg_;

			IODir();

			// No locks, lock if necessary
    bool		doReRead(bool force=false);
    static IOObj*	doRead(const char*,IODir*,uiString& errmsg,ObjNrType,
				bool incoldtmps=false);
    static IOObj*	readOmf(od_istream&,const char*,IODir*,ObjNrType,bool);
    static void		setDirNameFor(IOObj&,const char*);

    void		init(DirID,bool);
    bool		build(bool);
    bool		doAddObj(IOObj*,bool);
    bool		doWrite() const;
    bool		wrOmf(od_ostream&) const;
    const IOObj*	gtObj(const DBKey&) const;
    const IOObj*	gtObjByName(const char*,const char*) const;
    IdxType		gtIdxOf(const DBKey&) const;
    bool		gtIsOutdated() const;
    bool		doEnsureUniqueName(IOObj&) const;
    const IOObj*	main() const;

    DBKey		gtNewKey(const ObjNrType&) const;
    DBKey		newKey() const;		//!< locked, as it's 'public'

    friend class	IOMan;
    friend class	IOObj;
    friend class	IODirIter;
    friend class	IODirEntryList;

public:

    // 'Usually not needed' section

    void		reRead();
				// Done a lot already
    bool		writeNow() const;
				// write is automatic after commit or remove

    bool		addObj(IOObj*,bool immediate_store=true);
				// usually done by IOM()
				//!< after call, IOObj is mine
    static void		getTmpIOObjs(DirID,ObjectSet<IOObj>&,
					const IOObjSelConstraints* c=0);

};



mExpClass(General) IODirIter : public MonitorableIter< IODir::size_type >
{
public:

			IODirIter(const IODir&);
			IODirIter(const IODirIter&);

    virtual size_type	size() const	    { return ioDir().size(); }
    const IODir&	ioDir() const;

    const IOObj&	ioObj() const;
    DBKey		key() const;

private:

    IODirIter&		operator =(const IODirIter&);


};
