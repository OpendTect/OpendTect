#ifndef ctxtioobj_H
#define ctxtioobj_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		7-1-1996
 RCS:		$Id: ctxtioobj.h,v 1.3 2001-02-13 17:15:57 bert Exp $
________________________________________________________________________

-*/
 
 
#include <uidobj.h>
#include <multiid.h>
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
    bool		crlink;
    bool		needparent;
    int			parentlevel;
    const Translator*	partrgroup;	// If !0, parent needed for create
    MultiID		selkey;
    bool		multi;		// If true, multi allowed

    // this selection only
    bool		forread;
    bool		maychdir;
    bool		maydooper;
    ClassID		ioobjclassid;	// If specified, only this type
    const char*		deftransl;

    // this object only
    MultiID		parentkey;


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
    int			fillObj(const MultiID& idofdir=MultiID(""));
			//!< 0 = fail, 1=existing found, 2=new made

    IOObjContext	ctxt;
    IOObj*		ioobj;

};



inline void IOObjContext::init()
{
    newonlevel		= 0;
    crlink		= false;
    needparent		= false;
    parentlevel		= 0;
    partrgroup		= 0;
    multi		= false;
    forread		= true;
    maychdir		= true;
    maydooper		= true;
    ioobjclassid	= 0;
    deftransl		= 0;
}


#endif
