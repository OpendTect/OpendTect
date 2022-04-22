#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		2-8-1995
________________________________________________________________________


-*/

#include "generalmod.h"
#include "streamconn.h"
#include "filespec.h"
#include "ioobj.h"
class StreamProvider;


/*\brief is a file entry in the omf. */


mExpClass(General) IOStream : public IOObj
{
public:
			IOStream(const char* nm=0,const char* id=0,
				 bool =false);
			IOStream(const IOStream&);
    bool		isBad() const override;

    void		copyFrom(const IOObj*) override;
    const char*		fullUserExpr(bool forread=true) const override;
    BufferString	mainFileName() const override;
    const char*		connType() const override;
    Conn*		getConn(bool) const override;

    bool		implExists(bool forread) const override;
    bool		implReadOnly() const override;
    bool		implRemove() const override;
    bool		implSetReadOnly(bool) const override;
    bool		implRename(const char*,const CallBack* cb=0) override;
    bool		implManagesObjects() const override
			    { return false; }

    bool		isMultiConn() const	{ return isMulti(); }
    int			curConnIdx() const	{ return curfidx_; }
    void		resetConnIdx() const	{ curfidx_ = 0; }
    int			connIdxFor(int nr) const;
    bool		toNextConnIdx() const
			{ curfidx_++; return curfidx_ < nrFiles(); }
    void		setConnIdx( int idx ) const
			{ curfidx_ = idx; }

    FileSpec&		fileSpec()			{ return fs_; }
    const FileSpec&	fileSpec() const		{ return fs_; }
    void		setExt( const char* ext )	{ extension_ = ext; }
    void		genFileName();

    int			nrFiles() const			{ return fs_.nrFiles();}
    bool		isMulti() const			{ return nrFiles()>1; }

protected:

    bool		getFrom(ascistream&) override;
    bool		putTo(ascostream&) const override;

    FileSpec		fs_;
    mutable int		curfidx_;
    BufferString	extension_;
    BufferString	specfname_;

    StreamProvider*	getStreamProv(bool,bool f=true) const;
    bool		implDoAll(bool,bool yn=true) const;

public:

    void		setDirName(const char*) override;
    virtual void	setAbsDirectory(const char*);

};


