#ifndef cbvsio_h
#define cbvsio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H.Bril
 Date:		12-3-2001
 Contents:	Common Binary Volume Storage format io
 RCS:		$Id: cbvsio.h,v 1.15 2009-07-22 16:01:15 cvsbert Exp $
________________________________________________________________________

-*/

#include "position.h"
#include "bufstringset.h"


/*!\brief Base class for CBVS reader and writer

CBVS storage assumes inline-sorting of data. X-line sorting is simply not
supported.

*/

mClass CBVSIO
{
public:

			CBVSIO()
			: errmsg_(0), strmclosed_(false), nrxlines_(1)
			, nrcomps_(0), cnrbytes_(0)	{}
    virtual		~CBVSIO()			{ delete [] cnrbytes_; }

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

    mutable BinID	curbinid_;

};


/*!\brief Base class for CBVS read and write manager

*/

mClass CBVSIOMgr
{
public:

			CBVSIOMgr( const char* basefname )
			: curnr_(0)
			, basefname_(basefname)	{}
    virtual		~CBVSIOMgr();

    inline bool		failed() const		{ return errMsg(); }
    inline const char*	errMsg() const
			{ return errmsg_.isEmpty() ? errMsg_() : errmsg_.buf();}

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

protected:

    BufferString	basefname_;
    BufferString	errmsg_;
    BufferStringSet	fnames_;
    int			curnr_;

    virtual const char*	errMsg_() const		= 0;

    class AuxInlInf
    {
    public:
			AuxInlInf( int i ) : inl(i), cumnrxlines(0)	{}

	int		inl;
	int		cumnrxlines;
	TypeSet<int>	xlines;
    };

};

//! Common implementation macro
#define mGetAuxFromStrm(auxinf,buf,memb,strm) \
    strm.read( buf, sizeof(auxinf.memb) ); \
    auxinf.memb = finterp.get( buf, 0 )

//! Common implementation macro
#define mGetCoordAuxFromStrm(auxinf,buf,strm) \
    strm.read( buf, 2*sizeof(auxinf.coord.x) ); \
    auxinf.coord.x = dinterp.get( buf, 0 ); \
    auxinf.coord.y = dinterp.get( buf, 1 )

//! Common implementation macro
#define mAuxSetting(ptr,n) (*ptr & (unsigned char)n)


#endif
