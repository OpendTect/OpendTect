#ifndef filepath_h
#define filepath_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Mar 2004
 RCS:		$Id: filepath.h,v 1.2 2004-11-02 16:07:31 arend Exp $
________________________________________________________________________

-*/

#include "bufstringset.h"


/*!\brief File pathname tools */

class FilePath
{
public:
    enum Style		{ Local, Unix, Windows };

    			FilePath( const char* fnm=0 )
				{ set( fnm ); }
			FilePath( const FilePath& fp )
				{ *this = fp; }
    inline FilePath&	operator =( const FilePath& fp )
				{ lvls_ = fp.lvls_; prefix_ = fp.prefix_;
				  isabs_ = fp.isabs_; return *this; }
    inline FilePath&	operator =( const char* fnm )
				{ return (*this = FilePath(fnm)); }
    inline bool		operator ==( const FilePath& fp ) const
				{ return   lvls_ == fp.lvls_
					&& prefix_ == fp.prefix_
					&& isabs_ == fp.isabs_; }
    inline bool		operator ==( const char* fnm ) const
				{ return *this == FilePath(fnm); }
    inline bool		operator !=( const FilePath& fp ) const
				{ return !(*this == fp); }
    inline bool		operator !=( const char* fnm ) const
				{ return !(*this == FilePath(fnm)); }

    FilePath&		set(const char*);
    FilePath&		add(const char*);	//!< at end
    FilePath&		insert(const char*);	//!< after prefix
    void		setFileName(const char*); //!< pass null to remove level
    void		setPath(const char*);	//!< including prefix
    void		setExtension(const char*,bool replace=true);
    						//!< !replace => add

    inline bool		isAbsolute() const	{ return isabs_; }

    inline BufferString	fullPath() const	{ return dirUpTo(-1); }
    inline BufferString	fullPathStyled( Style f ) const
			    { return mkCleanPath(fullPath(),f); }
    inline const char*	prefix() const		{ return prefix_.buf(); }
    inline int		nrLevels() const	{ return lvls_.size(); }

    inline const BufferString& fileName() const
				{ return dir(-1); }
    inline BufferString	pathOnly() const
				{ return dirUpTo(lvls_.size()-2); }

    const BufferString&	dir(int nr=-1) const;
    			//!< nr < 0 returns last dir name
    BufferString	dirUpTo(int) const;
    			//!< nr < 0 returns last dir, including prefix

    static BufferString	getTempName(const char* ext=0);
    static BufferString	mkCleanPath(const char* path, Style fmt);
    static const char*	sPrefSep;
    static const char*	sDirSep;

protected:

    bool		isabs_;
    BufferString	prefix_;
    BufferStringSet	lvls_;

    void		addPart(const char*);
    void		compress(int sl=0);

};


#endif
