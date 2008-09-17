#ifndef segyfiledef_h
#define segyfiledef_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2008
 RCS:		$Id: segyfiledef.h,v 1.2 2008-09-17 15:27:00 cvsbert Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "bufstring.h"
class IOPar;
 

namespace SEGY
{

struct TrcFileIdx
{
		TrcFileIdx( int fnr=-1, int tnr=0 )
		    : filenr_(fnr), trcnr_(tnr) {}

    bool	isValid() const			{ return filenr_ >= 0; }

    int		filenr_;
    int		trcnr_;
};

/*\brief Input and output file(s)  */

class FileSpec
{
public:
    			FileSpec( const char* fnm=0 )
			    : fname_(fnm)
			    , nrs_(mUdf(int),0,1)
			    , zeropad_(0)	{}

    BufferString	fname_;
    StepInterval<int>	nrs_;
    int			zeropad_;	//!< pad zeros to this length

    bool		isMultiFile() const	{ return !mIsUdf(nrs_.start); }

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    void		getMultiFromString(const char*);
    static const char*	sKeyFileNrs;

};


/*\brief Parameters that control the primary read process */

class FilePars
{
public:
    			FilePars( bool forread )
			    : ns_(0)
			    , fmt_(forread?0:1)
			    , byteswapped_(false)
			    , forread_(forread)		{}

    int			ns_;
    int			fmt_;
    bool		byteswapped_;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    static int		nrFmts( bool forread )	{ return forread ? 6 : 5; }
    static const char**	getFmts(bool forread);
    static const char*	nameOfFmt(int fmt,bool forread);
    static int		fmtOf(const char*,bool forread);

protected:

    bool		forread_;

};

} // namespace


#endif
