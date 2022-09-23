#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstringset.h"
#include "ranges.h"


/*!\brief Specification for one or more files.

  If the paths are not absolute, they will be considered relative to the
  current survey directory. If you work with relative pathnames, you can
  specify a subdir-from-survey-dir.

 */

mExpClass(Basic) FileSpec
{
public:

			FileSpec(const char* fnm=0);
			FileSpec(const IOPar&);
			virtual ~FileSpec();

    BufferStringSet	fnames_;
    StepInterval<int>	nrs_;
    int			zeropad_;	//!< left-pad the nrs_ to this length
    BufferString	survsubdir_;	//!< For example "Seismics"

    bool		isEmpty() const
			{ return fnames_.isEmpty() || fnames_.get(0).isEmpty();}
    bool		isMulti() const		{ return nrFiles() > 1; }
    bool		isRangeMulti() const;

    int			nrFiles() const;
    const char*		fileName(int nr=0) const;
    const char*		absFileName(int nr=0) const; //!< adds path if necessary
    const char*		dirName() const;	//!< only the dir name
    const char*		fullDirName() const;	//!< full name of dir

    const char*		dispName() const;	//!< for titles etc
    const char*		usrStr() const;		//!< a user-typed filename
    void		setUsrStr( const char* str ) { usrstr_ = str; }

    void		setEmpty()
			{ fnames_.setEmpty(); mSetUdf(nrs_.start); }
    void		setFileName( const char* nm )
			{ setEmpty(); if ( nm && *nm ) fnames_.add(nm);}
    void		ensureBaseDir(const char* dirnm);
    void		makeAbsoluteIfRelative(const char* dirnm);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		getReport(IOPar&) const;
    static const char*	sKeyFileNrs();

    void		makePathsRelative(const char* todir=0);
			//< default is survey directory
    static void		makePathsRelative(IOPar&,const char* todir=0);
			//< default is survey directory

    void		getMultiFromString(const char*);

protected:

    BufferString	usrstr_;

};
