#ifndef iostrm_H
#define iostrm_H

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		2-8-1995
 RCS:		$Id$
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
    virtual bool	isBad() const;

    virtual void	copyFrom(const IOObj*);
    virtual const char*	fullUserExpr(bool forread=true) const;
    virtual const char*	connType() const;
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

    FileSpec&		fileSpec()			{ return fs_; }
    const FileSpec&	fileSpec() const		{ return fs_; }
    void		setExt( const char* ext )	{ extension_ = ext; }
    void		genFileName();

    int			nrFiles() const			{ return fs_.nrFiles();}
    bool		isMulti() const			{ return nrFiles()>1; }

protected:

    virtual bool	getFrom(ascistream&);
    virtual bool	putTo(ascostream&) const;

    FileSpec		fs_;
    mutable int		curfidx_;
    BufferString	extension_;

    StreamProvider*	getStreamProv(bool,bool f=true) const;
    bool		implDoAll(bool,bool yn=true) const;

public:

    virtual void	setDirName(const char*);

};


#endif

