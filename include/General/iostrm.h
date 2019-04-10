#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		2-8-1995
________________________________________________________________________


-*/

#include "generalmod.h"
#include "streamconn.h"
#include "filemultispec.h"
#include "ioobj.h"
class StreamProvider;


/*\brief is a file entry in the omf. */


mExpClass(General) IOStream : public IOObj
{
public:
			IOStream(const char* nm=0,DBKey id=DBKey::getInvalid(),
				 bool gendefimpl=false);
			IOStream(const IOStream&);
    virtual bool	isBad() const;

    virtual void	copyFrom(const IOObj&);
    virtual const char*	fullUserExpr(bool forread=true) const;
    virtual DBKey	key() const;
    virtual bool	isInCurrentSurvey() const;
    virtual BufferString mainFileName() const;
    virtual const char*	connType() const;
    virtual bool	isStream() const	{ return true; }
    virtual IOStream*	asStream()		{ return this; }
    virtual const IOStream* asStream() const	{ return this; }
    virtual Conn*	getConn(bool) const;

    virtual bool	implExists(bool forread) const;
    virtual bool	implReadOnly() const;
    virtual bool	implRemove() const;
    virtual bool	implSetReadOnly(bool) const;
    virtual bool	implRename(const char*,const CallBack* cb=0);
    virtual bool	implManagesObjects() const	{ return false; }

    bool		isMultiConn() const	{ return isMulti(); }
    int			curConnIdx() const	{ return curfidx_; }
    void		resetConnIdx() const	{ curfidx_ = 0; }
    int			connIdxFor(int nr) const;
    bool		toNextConnIdx() const
			{ curfidx_++; return curfidx_ < nrFiles(); }
    void		setConnIdx( int idx ) const
			{ curfidx_ = idx; }

    File::MultiSpec&	fileSpec()			{ return fs_; }
    const File::MultiSpec& fileSpec() const		{ return fs_; }
    void		setExt( const char* ext )	{ extension_ = ext; }
    void		genFileName();
    bool		locateInSharedDir(const char* dirnm);
			//!< will not move files, just changes the entry

    int			nrFiles() const			{ return fs_.nrFiles();}
    bool		isMulti() const			{ return nrFiles()>1; }

protected:

    virtual bool	getFrom(ascistream&);
    virtual bool	putTo(ascostream&) const;
    virtual bool	isEqTo(const IOObj&) const;

    File::MultiSpec	fs_;
    mutable int		curfidx_;
    BufferString	extension_;
    BufferString	specfname_;

    StreamProvider*	getStreamProv(bool,bool f=true) const;
    bool		implDoAll(bool,bool yn=true) const;

public:

    virtual void	setDirName(const char*);
    virtual void	setAbsDirectory(const char*);

};
