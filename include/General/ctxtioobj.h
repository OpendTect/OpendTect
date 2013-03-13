#ifndef ctxtioobj_h
#define ctxtioobj_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		7-1-1996
 RCS:		$Id$
________________________________________________________________________

-*/
 
#include "generalmod.h"
#include "namedobj.h"
#include "multiid.h"
#include "enums.h"
#include "ioobj.h"
#include "iopar.h"

class IOPar;
class TranslatorGroup;

/*!
\brief Holds constraints on IOObj selection.
*/

mExpClass(General) IOObjSelConstraints
{
public:
    
    			IOObjSelConstraints();
    			IOObjSelConstraints(const IOObjSelConstraints&);
    			~IOObjSelConstraints();
    IOObjSelConstraints& operator =(const IOObjSelConstraints&);

    IOPar&		require_;
    IOPar&		dontallow_;

    BufferString	allowtransls_;		//!< Glob expression
    bool		allownonreaddefault_;	//!< allow 'alien' like SEG-Y

    bool		isGood(const IOObj&) const;
    void		clear();
};


/*!
\brief Holds the context for selecting and/or creating IOObjs.

  Usually, this objects is obtained by calling the ioContext() method of a
  certain TranslatorGroup.
  
  Note, that if the StdSelType is set to None, you must provide the selkey or
  we'll be blobbing stuff in the root of the survey.
*/

mExpClass(General) IOObjContext : public NamedObject
{
public:

    enum StdSelType	{ Seis=0, Surf, Loc, Feat, WllInf, NLA, Misc, Attr, Mdl,
			  Geom, None };
			DeclareEnumUtils(StdSelType)

			IOObjContext(const TranslatorGroup*,
				     const char* prefname=0);
			IOObjContext(const IOObjContext&);
    IOObjContext&	operator =(const IOObjContext&);

    //! intrinsics
    StdSelType		stdseltype;
    const TranslatorGroup* trgroup;	//!< Mandatory, must never be 0
    int			newonlevel;	//!< level 0 is survey dir
    bool		multi;		//!< If true, multi allowed

    //! this selection only
    bool		forread;
    MultiID		selkey;		//!< If set, overrules the 'standard'
    bool		maydooper;	//!< Will we allow add/remove etc?
    BufferString	deftransl;	//!< Translator to use for new entry
    IOObjSelConstraints	toselect;

    bool		validIOObj(const IOObj&) const;

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

    inline bool		hasStdSelKey() const	{ return stdseltype != None; }
    MultiID		getSelKey() const;
    void		fillTrGroup();
    			//!< Uses stdseltype to make a trgroup
    			//!< Should never be necessary
};


/*!
\brief Holds an IOObjCtxt plus a pointer to an IOObj and/or an IOPar.

  Neither the IOObj nor the IOPar are managed by this object. But, when you
  use setObj or setPar, the old object pointed to will be deleted. If you don't
  want that, you'll have to just assign.
*/

mExpClass(General) CtxtIOObj : public NamedObject
{
public:
			CtxtIOObj( const IOObjContext& ct, IOObj* o=0 )
			: NamedObject(""), ctxt(ct), ioobj(o), iopar(0)
			{ setLinked(&ctxt); }
			CtxtIOObj( const CtxtIOObj& ct )
			: NamedObject(""), ctxt(ct.ctxt)
			, ioobj(ct.ioobj?ct.ioobj->clone():0)
			, iopar(ct.iopar?new IOPar(*ct.iopar):0)
			{ setLinked(&ctxt); }
    void		destroyAll();

    void		setObj(IOObj*); //!< destroys previous
    void		setObj(const MultiID&); //!< destroys previous
    void		setPar(IOPar*); //!< destroys previous
    int			fillObj(bool mktmpifnew=false);
			//!< If ioobj not valid, fills using ctxt.name()
			//!< 0 = fail, 1=existing found, 2=new made
    void		fillIfOnlyOne();
				//!< replaces ioobj if there's only one
				//!< That one must match the preconditions
    void		fillDefault(bool alsoifonlyone=true);
				//!< gets Default.xx or does fillIfOnlyOne()
    void		fillDefaultWithKey(const char*,bool alsoifonlyone=true);
				//!< With alternate key

    IOObjContext	ctxt;
    IOObj*		ioobj;
    IOPar*		iopar;

};


#endif

