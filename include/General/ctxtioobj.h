#ifndef ctxtioobj_H
#define ctxtioobj_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		7-1-1996
 RCS:		$Id: ctxtioobj.h,v 1.7 2001-08-22 15:37:59 bert Exp $
________________________________________________________________________

-*/
 
 
#include <uidobj.h>
#include <multiid.h>
#include <idobj.h>
#include <enums.h>
class Translator;
class IOObj;
class IOPar;

/*!\brief Holds the context for selecting and/or creating IOObjs.

Usually, this objects is obtained by calling the ioContext() method of
a certain Translator group.

*/


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



/*!\brief Holds an IOObjCtxt plus a pointer to an IOObj and/or an IOPar.

Neither the IOObj nor the IOPar are managed by this object. But, when you
use setObj or setPar, the old object pointed to will be deleted. If you
don't want that, you'll have to just assign.

*/



class CtxtIOObj : public UserIDObject
{
public:
			CtxtIOObj( const IOObjContext& ct, IOObj* o=0 )
			: UserIDObject(""), ctxt(ct), ioobj(o), iopar(0)
			{ setLinked(&ctxt); }
			CtxtIOObj( const CtxtIOObj& ct )
			: UserIDObject(""), ctxt(ct.ctxt)
			, ioobj(ct.ioobj), iopar(ct.iopar)
			{ setLinked(&ctxt); }
    void		destroyAll();

    void		setObj(IOObj*); //!< destroys previous
    void		setObj(const MultiID&); //!< destroys previous
    void		setPar(IOPar*); //!< destroys previous
    int			fillObj(const MultiID& idofdir=MultiID(""));
			//!< 0 = fail, 1=existing found, 2=new made

    IOObjContext	ctxt;
    IOObj*		ioobj;
    IOPar*		iopar;

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
    deftransl		= "";
    stdseltype		= None;
}


#endif
