#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Sep 2016
________________________________________________________________________

-*/


#include "generalmod.h"
#include "sharedobject.h"
#include "monitoriter.h"
#include "dbkey.h"
#include "objectset.h"
#include "od_iosfwd.h"
#include "threadlock.h"
#include "uistring.h"
#include "ioobjctxt.h"


/*\brief 'Directory' of IOObj objects.

The DBDir class is responsible for finding all IOObj's in one of the
subdirectories of an OpendTect data tree. A DBDir instance will actually load
all IOObj's, provides access to keys and allows searching.

*/


mExpClass(General) DBDir : public SharedObject
{ mODTextTranslationClass(DBDir);
public:

    typedef ObjectSet<IOObj>::size_type	size_type;
    typedef size_type			IdxType;
    typedef DBKey::ObjNrType		ObjNrType;
    typedef DBKey::DirID		DirID;
    typedef DBKey::ObjID		ObjID;

			DBDir(const char* dirname);
			DBDir(DirID);
			DBDir(IOObjContext::StdSelType);
			~DBDir();
			mDeclMonitorableAssignment(DBDir);

    bool		isBad() const;
    bool		isOutdated() const;
    DirID		dirID() const		{ return dirid_; }
    const char*		dirName() const		{ return dirname_; }
    od_int64		readTime() const	{ return readtime_; }
    uiString		errMsg() const		{ return errmsg_; }

    size_type		size() const;
    bool		isEmpty() const		{ return size() < 1; }

    bool		isPresent(ObjID) const;
    bool		isPresent(const DBKey&) const;
    IdxType		indexOf(ObjID) const;

    IOObj*		getEntry(ObjID) const;
    IOObj*		getEntryByName(const char* nm,
					const char* trgrpnm=0) const;
    IOObj*		getEntryByIdx(IdxType) const;

    bool		commitChanges(const IOObj&);
    bool		permRemove(ObjID);
    DBKey		newTmpKey() const;

    static ChangeType	cEntryChanged()		{ return 2; }
    static ChangeType	cEntryAdded()		{ return 3; }
    static ChangeType	cEntryToBeRemoved()	{ return 4; }

private:

    bool		isok_;
    ObjectSet<IOObj>	objs_;
    const DirID		dirid_;
    const BufferString	dirname_;
    od_int64		readtime_;
    mutable ObjNrType	curnr_;
    mutable ObjNrType	curtmpnr_;
    mutable uiString	errmsg_;

			DBDir();

    void		fromDirID(DirID,bool);
    bool		readFromFile(bool);
    bool		readOmf(od_istream&,bool);
    bool		writeToFile() const;
    bool		wrOmf(od_ostream&) const;
    const IOObj*	gtObjByName(const char*,const char*) const;
    IdxType		gtIdx(ObjID) const;
    bool		gtIsOutdated() const;
    bool		setObj(IOObj*,bool writeafter);
    bool		ensureUniqueName(IOObj&) const;

    DBKey		gtNewKey(const ObjNrType&) const;

    friend class	DBDirIter;

    void		setObjDirName(IOObj&);

public:

    // 'Usually not needed' section

    bool		reRead(bool force) const;
				// Done a lot already

    static void		getTmpIOObjs(DirID,ObjectSet<IOObj>&,
					const IOObjSelConstraints* c=0);

};



mExpClass(General) DBDirIter : public MonitorableIter<DBDir::size_type>
{
public:

    typedef DBDir::ObjID ObjID;

			DBDirIter(const DBDir&);
			DBDirIter(const DBDirIter&);

    virtual size_type	size() const	    { return dbDir().size(); }
    const DBDir&	dbDir() const;

    const IOObj&	ioObj() const;
    ObjID		objID() const;
    DBKey		key() const;

private:

    DBDirIter&		operator =(const DBDirIter&);

};
