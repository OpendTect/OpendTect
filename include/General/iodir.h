#ifndef iodir_H
#define iodir_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		31-7-1995
 RCS:		$Id: iodir.h,v 1.2 1999-10-18 14:03:07 dgb Exp $
________________________________________________________________________

@$*/
 
/*@+
@$*/
 
#include <ioobj.h>
#include <selector.h>
class ostream;
class AliasObjectSet;
class AliasObject;


class IODir : public UnitIDObject
{

    friend class	IOMan;
    friend class	IOObj;
    friend class	IOLink;
    friend class	IOObjSelector;
    friend class	dIoSelect;

public:
			IODir(const char*);
			IODir(const UnitID&);
			~IODir();
    void		reRead();
    bool		bad() const		{ return state_ == Fail; }

    const IOObj*	main() const;
    const char*		dirName() const		{ return dirname_; }

    const UserIDObjectSet<IOObj>& getObjs() const { return objs_; }
    int			size() const		  { return objs_.size(); }
    const IOObj*	operator[](const UnitID&) const;
    const IOObj*	operator[]( int idx ) const	{ return objs_[idx]; }
    const IOObj*	operator[]( IOObj* o ) const	{ return objs_[o]; }
    const IOObj*	operator[]( const char* str ) const
							{ return objs_[str]; }

    bool		addObj(IOObj*,bool immediate_store=YES);
			// after call, IOObj is mine!
    bool		commitChanges(const IOObj*);
			// after call, pointer may dangle!
    bool		permRemove(const UnitID&);
    bool		mkUniqueName(IOObj*);

    void		setCurrent( const IOObj* obj )
			{ objs_.setCurrent(obj); }
    const IOObj*	current() const
			{ return objs_.current(); }

    static IOObj*	getObj(const UnitID&);
    static IOObj*	getMain(const char*);

			// Use this if you know there's no contingency
			// Therefore, only in special-purpose programs
    bool		doWrite() const;

private:

    UserIDObjectSet<IOObj> objs_;
    FileNameString	dirname_;
    int			state_;
    int			curid_;

			IODir();
    static bool		create(const char* dirnm,const UnitID&,IOObj* mainobj);
    static IOObj*	doRead(const char*,IODir*,int id=-1);
    bool		build();

    IOObj*		operator[]( int idx )		{ return objs_[idx]; }
    IOObj*		operator[]( IOObj* o )		{ return objs_[o]; }
    IOObj*		operator[]( const char* str )	{ return objs_[str]; }
    void		operator +=( IOObj* obj )	{ objs_ += obj; }
    void		operator -=( IOObj* obj )	{ objs_ -= obj; }

    UnitID		newId() const;

    static IOObj*	readOmf(const char*,const char*,IODir*,int,bool&);
};


#endif
