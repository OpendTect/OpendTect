#ifndef iox_h
#define iox_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H.Bril
 Date:		25-7-1997
 Contents:	IOObj on other IOObj
 RCS:		$Id: iox.h,v 1.8 2003-10-15 15:15:53 bert Exp $
________________________________________________________________________

-*/

#include <ioobject.h>


/*\brief is a X-Group entry in the omf, e.g. Seismic data based upon a
Feature Set. */

class IOX : public IOObject
{	    isUidConcreteDefObject(IOX)
public:
			IOX(const char* nm=0,const char* ky=0,bool =0);
    virtual		~IOX();
    bool		bad() const;

    void		copyFrom(const IOObj*);
    const char*		fullUserExpr(bool) const;
    void		genDefaultImpl()		{}

    bool		implExists(bool) const;

    const char*		connType() const;
    Conn*		getConn(Conn::State) const;
    bool		slowOpen() const;
    IOObj*		getIOObj() const;

    const MultiID&	ownKey() const			{ return ownkey_; }
    void		setOwnKey(const MultiID&);

protected:

    MultiID		ownkey_;

    int			getFrom(ascistream&);
    int			putTo(ascostream&) const;

};


#endif
