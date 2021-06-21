#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Mar 2004
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstringset.h"


namespace File
{

/*!
\brief File pathname tools.

  This class splits a file or directory name into the separate parts:
  directories and possibly a filename (the last part does not have to be a
  file name). That makes it easy to change the file name, or the path only,
  or the extension, add directories, etc.

  To get the result after manipulation, use the fullPath().

  fileName() returns the last part, pathOnly() returns all but the last part.
  setFileName() replaces the last part, setPath() replaces all but the last
  part.
*/

mExpClass(Basic) Path
{
public:

			Path(const char* fullinp=0);
			Path(const char* p1,const char* p2,const char* p3=0,
				 const char* p4=0,const char* p5=0);
			Path( const Path& fp )	{ *this = fp; }
			Path(const Path&,const char* p2,
				 const char* p3=0,const char* p4=0,
				 const char* p5=0);

    Path&		operator =(const Path&);
    Path&		operator =(const char* fullinp);
    bool		operator ==(const Path&) const;
    bool		operator ==(const char* fnm) const;
    bool		operator !=(const Path&) const;
    bool		operator !=(const char* fnm) const;
    bool		isEmpty() const		{ return lvls_.isEmpty(); }

    Path&		set(const char* fullinp);
    Path&		add(const char*);	//!< at end
    Path&		insert(const char*);	//!< after prefix at start
    Path&		setFileName(const char*); //!< pass null to remove level
    Path&		setPath(const char*);	//!< including prefix
    Path&		setExtension(const char*,bool replace=true);
						//!< !replace => bluntly add

    bool		exists() const;
    bool		isAbsolute() const	{ return isabs_; }
    bool		isURI() const		{ return !domain_.isEmpty(); }
    bool		isSubDirOf(const Path&,Path* reldir = 0) const;
			/*!<If reldir is set, it will be filled with the
			    relative path. */
    bool		makeRelativeTo(const Path&);
    bool		makeCanonical(); // i.e. follow links, expand '..'

    BufferString	fullPath() const	{ return fullPath( Local ); }
    const char*		prefix() const		{ return prefix_; }
    const char*		postfix() const		{ return postfix_; }
    const char*		domain() const		{ return domain_; }
    int			nrLevels() const	{ return lvls_.size(); }
    const char*		extension() const;	//!< may return null

    const OD::String&	fileName() const;
    BufferString	pathOnly() const;
    BufferString	winDrive() const;
    BufferString	baseName() const;	//!< file w/o path or ext

    const OD::String&	dir(int nr=-1) const;
			//!< nr < 0 returns last dir name
    BufferString	dirUpTo(int) const;
			//!< nr < 0 returns last dir, including prefix
    BufferString	fileFrom(int) const;

    static BufferString	getTempDir();
    static BufferString	getTempFileName(const char* typ,const char* ext);
    static BufferString	getTempFullPath(const char* typ,const char* ext);
    static BufferString getTimeStampFileName(const char* ext);
    mDeprecated static BufferString getTempName( const char* ext=0 )
			{ return getTempFullPath(0,ext); }

    const char*		dirSep() const;
    static const char*	sPrefSep;
    static const char*	uriProtocolSeparator()		{ return "://"; }

    static BufferString getFullLongPath(const File::Path&);

protected:

    bool		isabs_;
    BufferString	prefix_;
    BufferString	domain_;
    BufferString	postfix_;	    //!< after '?'

    BufferStringSet	lvls_;

    void		addPart(const char*);
    void		compress(int sl=0);
    void		conv2TrueDirIfLink();

public:

			// Use if you understand it
    enum Style		{ Local, Unix, Windows };
    static BufferString	mkCleanPath(const char* path,Style fmt);
    static const char*	dirSep(Style);
    BufferString	fullPath(Style,bool cleanup=true) const;
    BufferString	pathOnly(Style) const;

};

} // namespace File;


mGlobal(Basic) inline const char* sSeismicSubDir()	{ return "Seismics"; }
mGlobal(Basic) inline const char* sWellSubDir()		{ return "WellInfo"; }
mGlobal(Basic) inline const char* sSurfaceSubDir()	{ return "Surfaces"; }

mGlobal(Basic) inline const char* sParFileExtension()	{ return "par"; }
mGlobal(Basic) inline const char* sInfoFileExtension()	{ return "info"; }
mGlobal(Basic) inline const char* sProcFileExtension()	{ return "proc"; }
mGlobal(Basic) inline const char* sStatsFileExtension()	{ return "stats"; }


mDeprecated typedef File::Path FilePath;
