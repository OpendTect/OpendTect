#ifndef ctxtioobj_H
#define ctxtioobj_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		7-1-1996
 RCS:		$Id: ctxtioobj.h,v 1.4 2001-04-13 11:52:28 bert Exp $
________________________________________________________________________

-*/
 
 
#include <uidobj.h>
#include <multiid.h>
#include <idobj.h>
#include <enums.h>
class Translator;
class IOObj;
class IOPar;


class IOObjContext : public UserIDObject
{
public:

    enum StdSelType	{ Seis=0, Grd, Wvlt, Feat, Log, NN, Misc, Attr, None };
			DeclareEnumUtils(StdSelType)


			IOObjContext(const Translator*,const char* prefname=0);
			//!< defaults: see init() below
			IOObjContext(const IOObjContext&);
			IOObjContext(const IOPar&);
    IOObjContext&	operator=(const IOObjContext&);

    //! intrinsics
    const Translator*	trgroup;	//!< Mandatory, must never be 0
    int			newonlevel;
    bool		crlink;
    bool		needparent;
    int			parentlevel;
    const Translator*	partrgroup;	//!< If !0, parent needed for create
    StdSelType		stdseltype;
    bool		multi;		//!< If true, multi allowed
    bool		maychdir;

    //! this selection only
    bool		forread;
    bool		maydooper;
    BufferString	deftransl;
    MultiID		parentkey;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    struct StdDirData
    {
	const char*	id;
	const char*	dirnm;
	const char*	desc; //!< Can be converted to StdSelType
    };

    static int			totalNrStdDirs();
    static const StdDirData*	getStdDirData(StdSelType);

    inline bool		hasStdSelType() const	{ return stdseltype != None; }
    MultiID		stdSelKey() const
			{ return MultiID(getStdDirData(stdseltype)->id); }

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
    deftransl		= 0;
    stdseltype		= None;
}


#endif
