#ifndef ctxtioobj_H
#define ctxtioobj_H

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	A.H. Bril
 Date:		7-1-1996
 RCS:		$Id: ctxtioobj.h,v 1.18 2003-10-17 14:19:01 bert Exp $
________________________________________________________________________

-*/
 
 
#include "uidobj.h"
#include "multiid.h"
#include "enums.h"
class IOObj;
class IOPar;
class TranslatorGroup;

/*!\brief Holds the context for selecting and/or creating IOObjs.

Usually, this objects is obtained by calling the ioContext() method of
a certain Translator group.

*/


class IOObjContext : public UserIDObject
{
public:

    enum StdSelType	{ Seis=0, Surf, Loc, Feat, WllInf, NN, Misc, Attr, Mdl,
			  None };
			DeclareEnumUtils(StdSelType)


			IOObjContext(const TranslatorGroup*,
				     const char* prefname=0);
			//!< defaults: see init() below
			IOObjContext(const IOObjContext&);
			IOObjContext(const IOPar&);
    IOObjContext&	operator=(const IOObjContext&);

    //! intrinsics
    StdSelType		stdseltype;
    const TranslatorGroup* trgroup;	//!< Mandatory, must never be 0
    int			newonlevel;	//!< level 0 is survey dir
    bool		crlink;		//!< Create subdir with new entry
    bool		needparent;	//!< Does object have a 'parent'
    int			parentlevel;	//!< On what level can parent be found
    const TranslatorGroup* partrgroup;	//!< If !0, parent needed for create
    bool		multi;		//!< If true, multi allowed
    bool		maychdir;	//!< If not, only select from curdir

    //! this selection only
    bool		forread;
    MultiID		parentkey;	//!< If set, require this parent
    bool		maydooper;	//!< Will we allow add/remove etc?
    BufferString	deftransl;	//!< Translator to use for new entry
    BufferString	trglobexpr;	//!< Only display when matches globexpr
    BufferString	ioparkeyval[2];	//!< Allow only/not with this key-value
    bool		includekeyval;	//!< Is keyval only or not allowed

    bool		validIOObj(const IOObj&) const;

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
    static BufferString		getDataDirName(StdSelType);
    				//!< Including legacy names - smart

    inline bool		hasStdSelType() const	{ return stdseltype != None; }
    MultiID		stdSelKey() const
			{ return MultiID(getStdDirData(stdseltype)->id); }
    void		fillTrGroup();
    			//!< Uses stdseltype to make a trgroup
    			//!< SHould never be necessary

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
    includekeyval	= true;
}


#endif
