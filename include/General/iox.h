#ifndef iox_h
#define iox_h

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		25-7-1997
 Contents:	IOObj on other IOObj
 RCS:		$Id: iox.h,v 1.1.1.1 1999-09-03 10:11:41 dgb Exp $
________________________________________________________________________


@$*/

#include <ioobject.h>


/*$@ IOX
 is a X-Group entry in the omf.
@$*/
class IOX : public IOObject
{	    isUidConcreteDefObject(IOX)

    friend class	dIOX;

public:
			IOX(const char* nm=0,const char* id=0,bool =0);
    virtual		~IOX();
    bool		bad() const;

    void		setUid(const UnitID&);
    void		copyFrom(const IOObj*);
    const char*		fullUserExpr(bool) const;
    void		genDefaultImpl()		{}
    bool		hasConn(const ClassDef&) const;

    bool		implExists(bool) const;
    bool		implRemovable() const;
    bool		implRemove() const;

    Conn*		conn(Conn::State) const;
    IOObj*		getIOObj() const;

protected:

    UnitID		uid;

    int			getFrom(ascistream&);
    int			putTo(ascostream&) const;

};


#endif
