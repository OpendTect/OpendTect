#ifndef ctxtioobj_H
#define ctxtioobj_H

/*@+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		7-1-1996
 RCS:		$Id: ctxtioobj.h,v 1.2 2000-05-29 10:33:54 bert Exp $
________________________________________________________________________

@$*/
 
 
#include <uidobj.h>
#include <unitid.h>
#include <idobj.h>
class Translator;
class IOObj;

class IOObjContext : public UserIDObject
{
public:
			// defaults: see init() below
			IOObjContext(const Translator*,const char* prefname=0);
			IOObjContext(const IOObjContext&);
    IOObjContext&	operator=(const IOObjContext&);

    // intrinsics
    const Translator*	trgroup;	// Mandatory, must never be 0
    int			newonlevel;
    int			crlink;
    int			needparent;
    int			parentlevel;
    const Translator*	partrgroup;	// If !0, parent needed for create
    UnitID		selid;
    int			multi;		// If YES, multi allowed

    // this selection only
    int			forread;
    int			maychdir;
    int			maydooper;
    ClassID		ioobjclassid;	// If specified, only this type
    const char*		deftransl;

    // this object only
    UnitID		parentid;


private:

    inline void		init();

};


class CtxtIOObj : public UserIDObject
{
public:
			CtxtIOObj( const IOObjContext& ct, IOObj* o=0 )
			: UserIDObject(""), ctxt(ct), ioobj(o)
			{ setLinked(&ctxt); }
			CtxtIOObj( const CtxtIOObj& ct )
			: UserIDObject(""), ctxt(ct.ctxt), ioobj(ct.ioobj)
			{ setLinked(&ctxt); }

    void		setObj(IOObj*);
    int			fillObj(UnitID idofdir="");

    IOObjContext	ctxt;
    IOObj*		ioobj;

};



inline void IOObjContext::init()
{
    newonlevel		= 0;
    crlink		= NO;
    needparent		= NO;
    parentlevel		= 0;
    partrgroup		= 0;
    multi		= NO;
    forread		= YES;
    maychdir		= YES;
    maydooper		= YES;
    ioobjclassid	= 0;
    deftransl		= 0;
}


#endif
