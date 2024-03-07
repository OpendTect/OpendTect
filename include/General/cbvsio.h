#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "generalmod.h"
#include "position.h"
#include "posinfo.h"
#include "bufstringset.h"


/*!\brief Base class for CBVS reader and writer

CBVS storage assumes inline-sorting of data. X-line sorting is simply not
supported.

*/

mExpClass(General) CBVSIO
{
public:

			CBVSIO();
    virtual		~CBVSIO();

    bool		failed() const			{ return errmsg_; }
    const char*		errMsg() const			{ return errmsg_; }

    virtual void	close() 			= 0;
    int			nrComponents() const		{ return nrcomps_; }
    const BinID&	binID() const			{ return curbinid_; }
    void		setErrMsg( const char* s )	{ errmsg_ = s; }

    static const int	integersize;
    static const int	version;
    static const int	headstartbytes;

    enum CoordPol	{ InAux=0, InTrailer=1, NotStored=2 };

protected:

    const char*		errmsg_;
    int*		cnrbytes_;
    int			nrcomps_;
    bool		strmclosed_;
    int			nrxlines_;
    CoordPol		coordpol_;
    TypeSet<Coord>	trailercoords_;
    PosInfo::CubeData	lds_;

    mutable BinID	curbinid_;

};


/*!\brief Base class for CBVS read and write manager

*/

mExpClass(General) CBVSIOMgr
{
public:

			CBVSIOMgr(const char* basefname);
    virtual		~CBVSIOMgr();

    inline bool		failed() const		{ return errMsg(); }
    inline const char*	errMsg() const
			{ return errmsg_.isEmpty() ? errMsg_() : errmsg_.str();}

    virtual void	close() 		= 0;

    virtual int		nrComponents() const	= 0;
    virtual const BinID& binID() const		= 0;

    inline BufferString	getFileName( int nr ) const
			{ return getFileName(basefname_,nr); }

    static BufferString	baseFileName(const char*);
    static BufferString	getFileName(const char*,int);
    			//!< returns aux file name for negative nr
    static int		getFileNr(const char*);
    			//!< returns 0 or number behind '^'
    static int		nrFiles(const char*);

protected:

    BufferString	basefname_;
    BufferString	errmsg_;
    BufferStringSet	fnames_;
    int			curnr_;

    virtual const char*	errMsg_() const		= 0;

    mClass(General) AuxInlInf
    {
    public:
			AuxInlInf( int i ) : inl(i), cumnrxlines(0)	{}

	int		inl;
	int		cumnrxlines;
	TypeSet<int>	xlines;
    };

};
