#ifndef iodir_H
#define iodir_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		31-7-1995
 RCS:		$Id: iodir.h,v 1.11 2004-01-08 15:30:04 bert Exp $
________________________________________________________________________

-*/
 
 
#include <ioobj.h>
#include <selector.h>

/*\brief 'Directory' of IOObj objects.

The IODir class is responsible for finding all IOObj's in the system. An IODir
instance will actually load all IOObj's, provides access to keys and allows
searching. It has a key of its own.

Few operation are done through the IODir directly: usually, IOMan will be
the service access point.

*/


class IODir : public UserIDObject
{

    friend class	IOMan;
    friend class	IOObj;
    friend class	IOLink;

public:
			IODir(const char*);
			IODir(const MultiID&);
			~IODir();
    void		reRead();
    bool		bad() const		{ return state_ == Fail; }
    const MultiID&	key() const		{ return key_; }

    const IOObj*	main() const;
    const char*		dirName() const		{ return dirname_; }

    const ObjectSet<IOObj>& getObjs() const { return objs_; }
    int			size() const		  { return objs_.size(); }
    const IOObj*	operator[](const MultiID&) const;
    const IOObj*	operator[]( const char* str ) const;
    const IOObj*	operator[]( int idx ) const	{ return objs_[idx]; }
    const IOObj*	operator[]( IOObj* o ) const	{ return objs_[o]; }

    bool		addObj(IOObj*,bool immediate_store=YES);
			// after call, IOObj is mine!
    bool		commitChanges(const IOObj*);
			// after call, pointer may dangle!
    bool		permRemove(const MultiID&);
    bool		mkUniqueName(IOObj*);

    static IOObj*	getObj(const MultiID&);
    static IOObj*	getMain(const char*);

			// Use this if you know there's no contingency
			// Therefore, only in special-purpose programs
    bool		doWrite() const;

private:

    ObjectSet<IOObj>	objs_;
    FileNameString	dirname_;
    int			state_;
    int			curid_;
    MultiID		key_;

			IODir();
    static bool		create(const char* dirnm,const MultiID&,IOObj* mainobj);
    static IOObj*	doRead(const char*,IODir*,int id=-1);
    bool		build();
    bool		wrOmf(const char*) const;

    MultiID		newKey() const;

    static IOObj*	readOmf(const char*,const char*,IODir*,int,bool&);
};


#endif
