#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "basicmod.h"
#include "bufstringset.h"


/*!
\brief File pathname tools.

  This class splits a full filename (with path) into the separate parts:
  directories and possibly a filename (the last part does not have to be a
  file name). That makes it easy to change the file name, or the path only,
  or the extension, add directories, etc.

  To get the result after manipulation, use the fullPath().

  fileName() returns the last part, pathOnly() returns all but the last part.
  setFileName() replaces the last part, setPath() replaces all but the last
  part.
*/

mExpClass(Basic) FilePath
{
public:
    enum Style		{ Local, Unix, Windows };

			FilePath(const char* fullinp=0);
			FilePath(const char* p1,const char* p2,const char* p3=0,
				 const char* p4=0,const char* p5=0);
			FilePath( const FilePath& fp )	{ *this = fp; }
			FilePath(const FilePath&,const char* p2,
				 const char* p3=0,const char* p4=0,
				 const char* p5=0);
			~FilePath();

    FilePath&		operator =(const FilePath&);
    FilePath&		operator =(const char* fullinp);
    bool		operator ==(const FilePath&) const;
    bool		operator ==(const char* fnm) const;
    bool		operator !=(const FilePath&) const;
    bool		operator !=(const char* fnm) const;
    bool		isEmpty() const;

    FilePath&		set(const char* fullinp);
    FilePath&		add(const char*);	//!< at end
    FilePath&		insert(const char*);	//!< after prefix at start
    FilePath&		setFileName(const char*); //!< pass null to remove level
    FilePath&		setPath(const char*);	//!< including prefix
    FilePath&		setExtension(const char*,bool replace=true);
						//!< !replace => add
    bool		exists() const;

    bool		isAbsolute() const;
    bool		isURI() const;
    bool		isSubDirOf(const FilePath&,FilePath* reldir = 0) const;
			/*!<If reldir is set, it will be filled with the
			    relative path. */
    bool		makeRelativeTo(const FilePath&);
    bool		makeAbsolute(const FilePath&);
    bool		makeCanonical();

    BufferString	fullPath(Style s=Local,bool cleanup=true) const;
    const char*		prefix() const;
    const char*		postfix() const;
    const char*		domain() const;
    void		setDomain(const char*);
    int			nrLevels() const;
    const char*		extension() const;	//!< may return null

    BufferString	pathOnly(Style s=Local,bool cleanup=true) const;
    const OD::String&	fileName() const;
    BufferString	baseName() const; //!<return name of file w/o path & ext
    BufferString	winDrive() const;

    const OD::String&	dir(int nr=-1) const;
			//!< nr < 0 returns last dir name
    BufferString	dirUpTo(int) const;
			//!< nr < 0 returns last dir, including prefix
    BufferString	fileFrom(int level,Style s=Local) const;
    BufferString	partitionName() const;
    BufferString	rootPath() const;

    static BufferString	getTempDir();
    static BufferString	getTempFileName(const char* typ,const char* ext);
    static BufferString getTempFullPath(const char* typ,const char* ext);
    static BufferString getTimeStampFileName(const char* ext);
    mDeprecated("Use getTempFullPath instead")
    static BufferString	getTempName( const char* ext=0 )
			{ return getTempFullPath(0,ext); }
    static BufferString	mkCleanPath(const char* path,Style fmt);

    const char*		dirSep() const;
    static const char*	dirSep(Style);
    static const char*	sPrefSep;
    static const char*	uriProtocolSeparator()		{ return "://"; }

    static BufferString getLongPath(const char* shortpath,
				    BufferString* error=nullptr);
    static BufferString getShortPath(const char* longpath,
				    BufferString* error=nullptr);

protected:

    bool		isabs_				= true;
    bool		isuri_				= false;
    BufferString	prefix_;
    BufferString	domain_;
    BufferString	postfix_;	//!< after '?'

    BufferStringSet	lvls_;

    void		addPart(const char*);
    void		compress(int sl=0);
    void		trueDirIfLink();

public:
    mDeprecated("Use getLongPath")
    static BufferString getFullLongPath(const FilePath&);
};


mGlobal(Basic) inline const char* sSeismicSubDir()	{ return "Seismics"; }
mGlobal(Basic) inline const char* sWellSubDir()		{ return "WellInfo"; }
mGlobal(Basic) inline const char* sSurfaceSubDir()	{ return "Surfaces"; }

mGlobal(Basic) inline const char* sParFileExtension()	{ return "par"; }
mGlobal(Basic) inline const char* sInfoFileExtension()	{ return "info"; }
mGlobal(Basic) inline const char* sProcFileExtension()	{ return "proc"; }
mGlobal(Basic) inline const char* sStatsFileExtension()	{ return "stats"; }
