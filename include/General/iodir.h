#ifndef iodir_H
#define iodir_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		31-7-1995
 RCS:		$Id: iodir.h,v 1.5 2001-07-13 22:04:11 bert Exp $
________________________________________________________________________

@$*/
 
 
#include <ioobj.h>
#include <selector.h>
class AliasObjectSet;
class AliasObject;


class IODir : public UserIDObject
{

    friend class	IOMan;
    friend class	IOObj;
    friend class	IOLink;
    friend class	IOObjSelector;
    friend class	dIoSelect;

public:
			IODir(const char*);
			IODir(const MultiID&);
			~IODir();
    void		reRead();
    bool		bad() const		{ return state_ == Fail; }
    const MultiID&	key() const		{ return key_; }

    const IOObj*	main() const;
    const char*		dirName() const		{ return dirname_; }

    const UserIDObjectSet<IOObj>& getObjs() const { return objs_; }
    int			size() const		  { return objs_.size(); }
    const IOObj*	operator[](const MultiID&) const;
    const IOObj*	operator[]( int idx ) const	{ return objs_[idx]; }
    const IOObj*	operator[]( IOObj* o ) const	{ return objs_[o]; }
    const IOObj*	operator[]( const char* str ) const
							{ return objs_[str]; }

    bool		addObj(IOObj*,bool immediate_store=YES);
			// after call, IOObj is mine!
    bool		commitChanges(const IOObj*);
			// after call, pointer may dangle!
    bool		permRemove(const MultiID&);
    bool		mkUniqueName(IOObj*);

    void		setCurrent( const IOObj* obj )
			{ objs_.setCurrent(obj); }
    const IOObj*	current() const
			{ return objs_.current(); }

    static IOObj*	getObj(const MultiID&);
    static IOObj*	getMain(const char*);

			// Use this if you know there's no contingency
			// Therefore, only in special-purpose programs
    bool		doWrite() const;

private:

    UserIDObjectSet<IOObj> objs_;
    FileNameString	dirname_;
    int			state_;
    int			curid_;
    MultiID		key_;

			IODir();
    static bool		create(const char* dirnm,const MultiID&,IOObj* mainobj);
    static IOObj*	doRead(const char*,IODir*,int id=-1);
    bool		build();

    IOObj*		findObj( int idx )		{ return objs_[idx]; }
    IOObj*		findObj( IOObj* o )		{ return objs_[o]; }
    IOObj*		findObj( const char* str )	{ return objs_[str]; }
    void		operator +=( IOObj* obj )	{ objs_ += obj; }
    void		operator -=( IOObj* obj )	{ objs_ -= obj; }

    MultiID		newKey() const;

    static IOObj*	readOmf(const char*,const char*,IODir*,int,bool&);
};


#endif
