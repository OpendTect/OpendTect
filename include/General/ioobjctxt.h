#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
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
    bool		allowmissing_; //what to do if a require_ key is absent

    BufferString	allowtransls_;	//!< FileMultiString of glob expressions
    bool		allownonuserselectable_; //!< allow 'alien' like SEG-Y

    bool		isGood(const IOObj&,bool forread=true) const;
    void		clear();

    static bool		isAllowedTranslator(const char* tnm,const char* allowd);

};


/*!\brief Holds the context for selecting and/or creating IOObjs.

  Usually, this object is obtained by calling the ioContext() method of a
  certain TranslatorGroup. A Translator Group corresponds to an actual data
  type ('3D Horizon', 'Pre-Stack 3D seismics', ...).

  * stdseltype_: corresponds to the subdirectory in the data store.
	None for 'custom' data directories.
  * trgroup_:  needed because one subdirectory in the data store usually
	contains a number of data types.
  * dirid_: no matter what stdseltype_ says, this will be the DBDir ID used
	for finding and creating IOObj's.
  * forread_: in many cases, it necessary to know the purpose: select
	something existing or make a new object
  * toselect_: constraints on the keys in the IOObjs pars()
  * destpolicy_: Some objects are 'publishable' outside the survey. To make
	this happen for 3D seismic cubes, a new seismic format was created that
	stores the full relevant part of the survey setup in the .info file.
  * deftransl_: a translator is what you could call a 'data format handler'. If
	filled, will overrule the 'normal' default translator.

*/

mExpClass(General) IOObjContext : public NamedObject
{
public:

    typedef DBKey::GroupNrType	DBGroupNrType;
    typedef DBKey::DirID	DBDirID;

    enum StdSelType	{ Seis=0, Surf, Loc, Feat, WllInf, NLA, Misc, Attr, Mdl,
			  Geom, None };
			mDeclareEnumUtils(StdSelType);
    enum DestPolicy	{ SurveyOnly, AllowShared, PreferShared };
			//!< keeps 'allow shared' and 'want shared' in one flag

			IOObjContext(const TranslatorGroup*,
				     const char* prefname=0);
			IOObjContext(const IOObjContext&);

    IOObjContext&	operator =(const IOObjContext&);

    //! intrinsics
    StdSelType		stdseltype_;
    const TranslatorGroup* trgroup_;	//!< Mandatory, must never be 0

    //! this selection only
    bool		forread_;
    DBDirID		dirid_;		//!< If set, overrules the 'standard'
    BufferString	deftransl_;	//!< Translator to use for new entry
    IOObjSelConstraints toselect_;
    DestPolicy		destpolicy_;

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
    static BufferString		getDataDirName(StdSelType,
						bool subdirnmonly=false);

    FixedString		objectTypeName() const;
    uiString		uiObjectTypeName(int n=1) const;
    FixedString		translatorGroupName() const;
    uiString		translatorTypeName(int n=1) const;
    inline bool		hasStdSelDirID() const { return stdseltype_ != None; }
    DBDirID		getSelDirID() const;
    IOStream*		crDefaultWriteObj(const Translator&,
					  const DBKey&) const;
    void		fillTrGroup() const;
			//!< Uses stdseltype_ to make a trgroup_
			//!< Should never be necessary
    void		fixTranslator( const char* trusrnm )
			{ deftransl_ = toselect_.allowtransls_ = trusrnm; }

    int			nrMatches() const;
    inline bool		haveMatches() const { return nrMatches() > 0; }

    mDeprecated DBDirID	getSelKey() const   { return getSelDirID(); }
};
