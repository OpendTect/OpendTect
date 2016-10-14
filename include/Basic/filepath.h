#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
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

    enum Style		{ Local, Unix, Windows };

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

    bool		isAbsolute() const;
    bool		isSubDirOf(const Path&,Path* reldir = 0) const;
			/*!<If reldir is set, it will be filled with the
			    relative path. */
    bool		makeRelativeTo(const Path&);
    bool		makeCanonical();

    BufferString	fullPath(Style s=Local,bool cleanup=true) const;
    const char*		prefix() const;
    int			nrLevels() const;
    const char*		extension() const;	//!< may return null

    const OD::String&	fileName() const;
    BufferString	baseName() const; //!< name of file w/o path or ext
    BufferString	pathOnly() const;
    BufferString	winDrive() const;

    const OD::String&	dir(int nr=-1) const;
			//!< nr < 0 returns last dir name
    BufferString	dirUpTo(int) const;
			//!< nr < 0 returns last dir, including prefix

    static BufferString	getTempDir();
    static BufferString	getTempName(const char* ext=0);
    static BufferString	mkCleanPath(const char* path, Style fmt);
    static BufferString getTimeStampFileName(const char*ext=0);

    const char*		dirSep() const;
    static const char*	dirSep(Style);
    static const char*	sPrefSep;

protected:

    bool		isuri_;
    bool		isabs_;
    BufferString	prefix_;
    BufferStringSet	lvls_;

    void		addPart(const char*);
    void		compress(int sl=0);
    void		conv2TrueDirIfLink();
};

} // namespace File;

mDeprecated typedef File::Path FilePath;
