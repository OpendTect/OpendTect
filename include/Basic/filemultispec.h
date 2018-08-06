#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2015
 RCS:		$Id: $
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstringset.h"
#include "ranges.h"


namespace File
{

/*!\brief Specification for one or more files.

  If the paths are not absolute, they will be considered relative to the
  current survey directory. If you work with relative pathnames, you can
  specify a subdir-from-survey-dir.

 */

mExpClass(Basic) MultiSpec
{
public:

			MultiSpec(const char* fnm=0);
			MultiSpec(const IOPar&);
    bool		operator ==( const MultiSpec& oth ) const
			{ return fnames_ == oth.fnames_
			      && nrs_ == oth.nrs_
			      && zeropad_ == oth.zeropad_
			      && survsubdir_ == oth.survsubdir_; }

    BufferStringSet	fnames_;
    StepInterval<int>	nrs_;
    int			zeropad_;	//!< left-pad the nrs_ to this length
    BufferString	survsubdir_;	//!< For example sSeismicSubDir()

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

    static void		makePathsRelative(IOPar&,const char* todir=0);
			//< default is survey directory

    void		getMultiFromString(const char*);

protected:

    BufferString	usrstr_;

};

} // namespace File
