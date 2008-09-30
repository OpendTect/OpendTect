#ifndef segyfiledef_h
#define segyfiledef_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2008
 RCS:		$Id: segyfiledef.h,v 1.5 2008-09-30 16:18:40 cvsbert Exp $
________________________________________________________________________

-*/

#include "ranges.h"
#include "bufstring.h"
#include "position.h"
#include "samplingdata.h"
#include "seistype.h"
class IOPar;
class IOObj;
class DataPointSet;
 

namespace SEGY
{

class TrcFileIdx
{
public:
		TrcFileIdx( int fnr=-1, int tnr=0 )
		    : filenr_(fnr), trcnr_(tnr) {}

    bool	isValid() const			{ return filenr_ >= 0; }
    void	toNextFile()			{ filenr_++; trcnr_ = 0; }

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
    int			nrFiles() const	
    			{ return isMultiFile() ? nrs_.nrSteps()+1 : 1; }
    const char*		getFileName(int nr=0) const;
    IOObj*		getIOObj(bool temporary=true) const;

    void		fillPar(IOPar&) const;
    void		usePar(const IOPar&);

    void		getMultiFromString(const char*);
    static const char*	sKeyFileNrs;

    static void		ensureWellDefined(IOObj&);
    static void		fillParFromIOObj(const IOObj&,IOPar&);
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


/*\brief Data usually obtained by scanning file  */

class FileData
{
public:
    			FileData(const char* fnm,Seis::GeomType);
    			FileData(const FileData&);
			~FileData();

    BufferString	fname_;
    Seis::GeomType	geom_;
    int			trcsz_;
    SamplingData<float>	sampling_;
    int			segyfmt_;
    bool		isrev1_;
    int			nrstanzas_;
    DataPointSet&	data_;

    int			nrTraces() const;
    BinID		binID(int) const;
    Coord		coord(int) const;
    int			trcNr(int) const;
    float		offset(int) const;
    bool		isNull(int) const;
    bool		isUsable(int) const;

};

} // namespace


#endif
