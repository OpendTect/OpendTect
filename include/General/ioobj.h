#ifndef ioobj_H
#define ioobj_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		31-7-1995
 RCS:		$Id: ioobj.h,v 1.5 2001-02-13 17:15:57 bert Exp $
________________________________________________________________________

-*/
 
 
#include <conn.h>
#include <multiid.h>
class ascistream;
class ascostream;
class Translator;
class IOPar;


class IOObj : public DefObject
	    , public UserIDObject
{	      isUidAbstractDefObject(IOObj)

    friend class	IODir;
    friend class	IOLink;

public:

    IOObj*		clone() const;
    IOObj*		getParent() const;
    virtual MultiID	key() const			{ return key_; }

    virtual		~IOObj();
    virtual bool	bad() const			= 0;
    virtual bool	isLink() const			{ return false; }
    virtual void	copyFrom(const IOObj*)		= 0;
    virtual bool	hasConnDef( const ClassDef& cd ) const
			{ return &cd == connclassdef_; }

    virtual const ClassDef& connType() const		= 0;
    virtual Conn*	getConn(Conn::State) const	= 0;
    virtual bool	slowOpen() const		{ return false; }

    virtual const MultiID& parentKey() const	= 0;
    virtual void	setParentKey(const char*)	= 0;
    virtual const char*	translator() const		= 0;
    virtual void	setTranslator(const char*)	= 0;
    virtual const char*	group() const			= 0;
    virtual void	setGroup(const char*)		= 0;
    virtual const char*	fullUserExpr(bool forread) const= 0;

    virtual bool	implExists(bool forread) const	= 0;
    virtual bool	implRemovable() const		= 0;
    virtual bool	implRemove() const		= 0;
    virtual bool	removeQuery() const		{ return false; }
    virtual void	genDefaultImpl()		{}

    virtual int		setName(const char*);
    virtual const char*	dirName() const;
    bool		isStandAlone() const { return dirname_ ? true : false; }
    void		setStandAlone(const char* dirnm);
    IOPar*		trOpts() const			{ return opts; }
    void		mkOpts();

    static IOObj*	produce(const char*,const char* nm=0,const char* ky=0,
				bool initdefaults=true);

    Translator*		getTranslator() const;

protected:

    FileNameString*	dirname_;
    const ClassDef*	connclassdef_;
    IOLink*		mylink_;
    MultiID		key_;

			IOObj(const char* nm=0,const char* ky=0);
			IOObj(IOObj*,const char* ky=0);
    static IOObj*	get(ascistream&,const char*,const char*);
    int			put(ascostream&) const;
    virtual int		getFrom(ascistream&)		= 0;
    virtual int		putTo(ascostream&) const	= 0;
    void		setKey( const char* nm )	{ key_ = nm; }

private:

    IOPar*		opts;
    int			myKey() const;

};


bool equalIOObj(const MultiID&,const MultiID&);
bool areEqual(const IOObj*,const IOObj*);


#endif
