#ifndef ioobj_H
#define ioobj_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		31-7-1995
 RCS:		$Id: ioobj.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________

@$*/
 
 
#include <conn.h>
#include <uidobj.h>
class ascistream;
class ascostream;
class Translator;
class IOPar;


class IOObj : public DefObject
	    , public UnitIDObject
{	      isUidAbstractDefObject(IOObj)

    friend class	IODir;
    friend class	IOLink;

public:
    virtual		~IOObj();
    virtual bool	bad() const			= 0;
    virtual bool	isLink() const			{ return NO; }
    virtual bool	multiConn() const		{ return NO; }

    virtual void	copyFrom(const IOObj*)		= 0;

    virtual Conn*	conn(Conn::State) const		= 0;
    virtual bool	hasConn( const ClassDef& cd ) const
			{ return &cd == connclassdef_; }
    virtual Conn*	nextConn(Conn::State) const	{ return 0; }
    virtual int		connNr() const			{ return 0; }
    virtual void	skipConn() const		{}
    virtual bool	isPercConn() const		{ return 0; }

    virtual const UnitID& parentId() const		= 0;
    virtual void	setParentId(const char*)	= 0;
    virtual const char*	translator() const		= 0;
    virtual void	setTranslator(const char*)	= 0;
    virtual const char*	group() const			= 0;
    virtual void	setGroup(const char*)		= 0;
    virtual const char*	fullUserExpr(bool forread) const= 0;

    virtual bool	implExists(bool forread) const	= 0;
    virtual bool	implRemovable() const		= 0;
    virtual bool	implRemove() const		= 0;
    virtual bool	removeQuery() const		{ return NO; }
    virtual void	genDefaultImpl()		{}

    virtual int		setName(const char*);
    virtual const char*	dirName() const;
    IOObj*		getParent() const;
    IOObj*		cloneStandAlone() const;
    bool		isStandAlone() const	{ return dirname_ ? YES : NO; }
    void		setStandAlone(const char* dirnm);
    IOPar*		trOpts() const			{ return opts; }
    void		mkOpts();

    static IOObj*	produce(const char*,const char* nm=0,const char* uid=0,
				bool initdefaults=YES);

    Translator*		getTranslator() const;

protected:
    FileNameString*	dirname_;
    const ClassDef*	connclassdef_;
    IOLink*		mylink_;

			IOObj(const char* nm=0,const char* uid=0);
			IOObj(IOObj*,const char* uid=0);
    static IOObj*	get(ascistream&,const char*,const char*);
    int			put(ascostream&) const;
    virtual int		getFrom(ascistream&)		= 0;
    virtual int		putTo(ascostream&) const	= 0;

private:

    IOPar*		opts;
    int			myId() const;

};


bool equalIOObj(const UnitID&,const UnitID&);
bool areEqual(const IOObj*,const IOObj*);


#endif
