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

class TranslatorGroup;
class IOStream;

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

    BufferString	allowtransls_;	//!< FileMultiString of glob expressions
    bool		allownonuserselectable_; //!< allow 'alien' like SEG-Y

    bool		isGood(const IOObj&,bool forread=true) const;
    void		clear();

    static bool		isAllowedTranslator(const char* tnm,const char* allowd);

};


/*!
\brief Holds the context for selecting and/or creating IOObjs.

  Usually, this object is obtained by calling the ioContext() method of a
  certain TranslatorGroup.

  Note, that if the StdSelType is set to None, you must provide the selkey or
  we'll be blobbing stuff in the root of the survey.
*/

mExpClass(General) IOObjContext : public NamedObject
{
public:

    enum StdSelType	{ Seis=0, Surf, Loc, Feat, WllInf, NLA, Misc, Attr, Mdl,
			  Geom, None };
			mDeclareEnumUtils(StdSelType);

			IOObjContext(const TranslatorGroup*,
				     const char* prefname=0);
			IOObjContext(const IOObjContext&);

    IOObjContext&	operator =(const IOObjContext&);

    //! intrinsics
    StdSelType		stdseltype_;
    const TranslatorGroup* trgroup_;	//!< Mandatory, must never be 0
    int			newonlevel_;	//!< level 0 is survey dir
    bool		multi_;		//!< If true, multi allowed

    //! this selection only
    bool		forread_;
    MultiID		selkey_;	//!< If set, overrules the 'standard'
    bool		maydooper_;	//!< Will we allow add/remove etc?
    BufferString	deftransl_;	//!< Translator to use for new entry
    IOObjSelConstraints toselect_;

    bool		validIOObj(const IOObj&) const;

    struct StdDirData
    {
					StdDirData(const char*,const char*,
						   const char*);

	const char*			id_;
	const char*			dirnm_;
	const char*			desc_;
					//!< Can be converted to StdSelType

	mDeprecated const char*&	id;
	mDeprecated const char*&	dirnm;
	mDeprecated const char*&	desc;
    };

    static int			totalNrStdDirs();
    static const StdDirData*	getStdDirData(StdSelType);
    static BufferString		getDataDirName(StdSelType);
				//!< Including legacy names - smart

    const char*		objectTypeName() const;
    inline bool		hasStdSelKey() const	{ return stdseltype_ != None; }
    MultiID		getSelKey() const;
    IOStream*		crDefaultWriteObj(const Translator&,
					  const MultiID&) const;
    void		fillTrGroup() const;
			//!< Uses stdseltype_ to make a trgroup_
			//!< Should never be necessary
    void		fixTranslator( const char* trusrnm )
			{ deftransl_ = toselect_.allowtransls_ = trusrnm; }

    mDeprecated StdSelType&		stdseltype;
    mDeprecated const TranslatorGroup*& trgroup;
    mDeprecated int&			newonlevel;
    mDeprecated bool&			multi;
    mDeprecated bool&			forread;
    mDeprecated MultiID&		selkey;
    mDeprecated bool&			maydooper;
    mDeprecated BufferString&		deftransl;
    mDeprecated IOObjSelConstraints&	toselect;
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
    mStartAllowDeprecatedSection
			CtxtIOObj( const IOObjContext& ct, IOObj* o=0 )
			    : NamedObject(""), ctxt_(ct), ioobj_(o), iopar_(0)
			    , ctxt(ctxt_), ioobj(ioobj_), iopar(iopar_)
			{ setLinkedTo(&ctxt_); }
			CtxtIOObj( const CtxtIOObj& ct )
			    : NamedObject(""), ctxt_(ct.ctxt_)
			    , ioobj_(ct.ioobj_?ct.ioobj_->clone():0)
			    , iopar_(ct.iopar_?new IOPar(*ct.iopar_):0)
			    , ctxt(ctxt_), ioobj(ioobj_), iopar(iopar_)
			{ setLinkedTo(&ctxt_); }
    mStopAllowDeprecatedSection
    void		destroyAll();

    void		setObj(IOObj*); //!< destroys previous
    void		setObj(const MultiID&); //!< destroys previous
    void		setPar(IOPar*); //!< destroys previous
    int			fillObj(bool mktmpifnew=false,int translidxfornew=-1);
			//!< If ioobj not valid, fills using ctxt.name()
			//!< return 0=fail, 1=existing found, 2=new made
    void		fillIfOnlyOne();
				//!< replaces ioobj if there's only one
				//!< That one must match the preconditions
    void		fillDefault(bool alsoifonlyone=true);
				//!< gets Default.xx or does fillIfOnlyOne()
    void		fillDefaultWithKey(const char*,bool alsoifonlyone=true);
				//!< With alternate key

    IOObjContext		ctxt_;
    IOObj*			ioobj_;
    IOPar*			iopar_;

    //Legacy
    mDeprecated IOObjContext&	ctxt;
    mDeprecated IOObj*&		ioobj;
    mDeprecated IOPar*&		iopar;
};


#endif


