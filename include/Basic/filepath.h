#ifndef filepath_h
#define filepath_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Mar 2004
 RCS:		$Id: filepath.h,v 1.8 2008-12-18 05:23:26 cvsranojay Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"


/*!\brief File pathname tools */

mClass FilePath
{
public:
    enum Style		{ Local, Unix, Windows };

    			FilePath( const char* fnm=0 );
			FilePath( const FilePath& fp );

    FilePath&		operator =( const FilePath& fp );
    FilePath&		operator =( const char* fnm );
    bool		operator ==( const FilePath& fp ) const;
    bool		operator ==( const char* fnm ) const;
    bool		operator !=( const FilePath& fp ) const;
    bool		operator !=( const char* fnm ) const;

    FilePath&		set(const char*);
    FilePath&		add(const char*);	//!< at end
    FilePath&		insert(const char*);	//!< after prefix
    void		setFileName(const char*); //!< pass null to remove level
    void		setPath(const char*);	//!< including prefix
    void		setExtension(const char*,bool replace=true);
    						//!< !replace => add

    bool		isAbsolute() const;

    BufferString	fullPath( Style f = Local, bool cleanup=true ) const;
    const char*		prefix() const;
    int			nrLevels() const;
    const char*		extension() const;	//!< may return null

    const BufferString& fileName() const;
    BufferString	pathOnly() const;

    const BufferString&	dir(int nr=-1) const;
    			//!< nr < 0 returns last dir name
    BufferString	dirUpTo(int) const;
    			//!< nr < 0 returns last dir, including prefix

    static BufferString	getTempDir();
    static BufferString	getTempName(const char* ext=0);
    static BufferString	mkCleanPath(const char* path, Style fmt);

    static const char*	dirSep( Style stl );
    static const char*	sPrefSep;

protected:

    bool		isabs_;
    BufferString	prefix_;
    BufferStringSet	lvls_;

    void		addPart(const char*);
    void		compress(int sl=0);
};


#endif
