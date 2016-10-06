#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Aug 1996 / Sep 2016
________________________________________________________________________

-*/

#include "generalmod.h"
#include "namedobj.h"
#include "dbkey.h"
#include "enums.h"
#include "iopar.h"

class IOObj;
class IOStream;
class CtxtIOObj;
class Translator;
class TranslatorGroup;

/*!\brief Holds constraints on IOObj selection. */

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


/*!\brief Holds the context for selecting and/or creating IOObjs.

  Usually, this object is obtained by calling the ioContext() method of a
  certain TranslatorGroup.

  Note, that if the StdSelType is set to None, you must provide the groupnr_ or
  we'll be blobbing stuff in the root of the survey.
*/

mExpClass(General) IOObjContext : public NamedObject
{
public:

    typedef DBKey::GroupNrType	DBGroupNrType;
    typedef DBKey::DirID	DBDirID;

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
    bool		multi_;		//!< If true, multi allowed

    //! this selection only
    bool		forread_;
    DBDirID		dirid_;		//!< If set, overrules the 'standard'
    BufferString	deftransl_;	//!< Translator to use for new entry
    IOObjSelConstraints toselect_;

    bool		validIOObj(const IOObj&) const;

    struct StdDirData
    {
			StdDirData(DBGroupNrType,const char*,const char*);

	DBDirID		id_;
	const char*	dirnm_;
	const char*	desc_;
					//!< Can be converted to StdSelType
    };

    static int			totalNrStdDirs();
    static const StdDirData*	getStdDirData(StdSelType);
    static BufferString		getDataDirName(StdSelType);
				//!< Including legacy names - smart

    FixedString		objectTypeName() const;
    FixedString		translatorGroupName() const;
    inline bool		hasStdSelDirID() const { return stdseltype_ != None; }
    DBDirID		getSelDirID() const;
    IOStream*		crDefaultWriteObj(const Translator&,
					  const DBKey&) const;
    void		fillTrGroup() const;
			//!< Uses stdseltype_ to make a trgroup_
			//!< Should never be necessary
    void		fixTranslator( const char* trusrnm )
			{ deftransl_ = toselect_.allowtransls_ = trusrnm; }

    mDeprecated StdSelType&		stdseltype;
    mDeprecated const TranslatorGroup*& trgroup;
    mDeprecated bool&			multi;
    mDeprecated bool&			forread;
    mDeprecated BufferString&		deftransl;
    mDeprecated IOObjSelConstraints&	toselect;
    mDeprecated DBDirID			getSelKey() const
					{ return getSelDirID(); }
};
