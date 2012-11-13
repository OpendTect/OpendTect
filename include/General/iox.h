#ifndef iox_h
#define iox_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		25-7-1997
 Contents:	IOObj on other IOObj
 RCS:		$Id$
________________________________________________________________________

-*/

#include "generalmod.h"
#include "ioobj.h"


/*\brief is a X-Group entry in the omf, e.g. PS data based on 3D cubes.  */

mClass(General) IOX : public IOObj
{
public:
			IOX(const char* nm=0,const char* ky=0,bool =0);
    virtual		~IOX();
    bool		bad() const;

    void		copyFrom(const IOObj*);
    const char*		fullUserExpr(bool forread=true) const;
    const char*		dirName() const;
    void		genDefaultImpl()		{}

    bool		implExists(bool) const;
    bool		implShouldRemove() const	{ return false; }

    FixedString		connType() const;
    Conn*		getConn(Conn::State) const;
    IOObj*		getIOObj() const;

    const MultiID&	ownKey() const			{ return ownkey_; }
    void		setOwnKey(const MultiID&);

protected:

    MultiID		ownkey_;

    bool		getFrom(ascistream&);
    bool		putTo(ascostream&) const;

    static int		prodid; //!< for factory implementation
};


#endif

