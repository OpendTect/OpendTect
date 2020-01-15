#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format io
________________________________________________________________________

-*/

#include "generalmod.h"
#include "bufstringset.h"
#include "cubedata.h"
#include "position.h"
#include "uistrings.h"


/*!\brief Base class for CBVS reader and writer

CBVS storage assumes inline-sorting of data. X-line sorting is simply not
supported.

*/

mExpClass(General) CBVSIO
{
public:

			CBVSIO()
			: errmsg_(toUiString("")),
			  strmclosed_(false), nrxlines_(1),
			  nrcomps_(0), cnrbytes_(0)	{}
    virtual		~CBVSIO();

    bool		failed() const	    { return !errmsg_.isEmpty(); }
    const uiString	errMsg() const	    { return errmsg_; }

    virtual void	close()			= 0;
    int			nrComponents() const		{ return nrcomps_; }
    const BinID&	binID() const			{ return curbinid_; }
    void		setErrMsg( const uiString& s )	{ errmsg_ = s; }

    static const int	integersize;
    static const int	version;
    static const int	headstartbytes;

    enum CoordPol	{ InAux=0, InTrailer=1, NotStored=2 };

protected:

    uiString		errmsg_;
    int*		cnrbytes_;
    int			nrcomps_;
    bool		strmclosed_;
    int			nrxlines_;
    CoordPol		coordpol_;
    TypeSet<Coord>	trailercoords_;
    PosInfo::SortedCubeData	lds_;

    mutable BinID	curbinid_;

};


/*!\brief Base class for CBVS read and write manager

*/

mExpClass(General) CBVSIOMgr
{ mODTextTranslationClass(CBVSIOMgr)
public:

			CBVSIOMgr( const char* basefname )
			: curnr_(0)
			, basefname_(basefname)	{}
    virtual		~CBVSIOMgr();

    inline bool		failed() const		{ return !errmsg_.isEmpty(); }
    inline uiString	errMsg() const		{ return errmsg_;}

    virtual void	close()		= 0;

    virtual int		nrComponents() const	= 0;
    virtual const BinID& binID() const		= 0;

    inline BufferString	getFileName( int nr ) const
			{ return getFileName(basefname_,nr); }

    static BufferString	baseFileName(const char*);
    static BufferString	getFileName(const char*,int);
			//!< returns aux file name for negative nr
    static int		getFileNr(const char*);
			//!< returns 0 or number behind '^'

protected:

    BufferString	basefname_;
    uiString		errmsg_;
    BufferStringSet	fnames_;
    int			curnr_;

    virtual uiString	gtErrMsg() const				= 0;

    mClass(General) AuxInlInf
    {
    public:
			AuxInlInf( int i ) : inl(i), cumnrxlines(0)	{}

	int		inl;
	int		cumnrxlines;
	TypeSet<int>	xlines;
    };

};
