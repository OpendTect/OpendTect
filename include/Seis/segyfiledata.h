#ifndef segyfiledata_h
#define segyfiledata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2008
 RCS:		$Id: segyfiledata.h,v 1.2 2008-11-13 11:33:21 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "position.h"
#include "seistype.h"
#include "samplingdata.h"
class IOPar;
class DataPointSet;
 

namespace SEGY
{

struct TraceInfo
{
		TraceInfo()
		    : nr_(0), offset_(0)
		    , isnull_(false), isusable_(true)		{}

    int		nr_;
    BinID	binid_;
    Coord	coord_;
    float	offset_;
    bool	isnull_;
    bool	isusable_;
};


/*\brief Data usually obtained by scanning a SEG-Y file.

  The data is stored in a DataPointSet, and we have to take measures against
  the SI() not being well defined. Thus, we add 2 extra columns for residual
  X and Y offset.
 
 */

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

    int			nrTraces() const;

    TraceInfo		traceData(int) const;
    BinID		binID(int) const;
    Coord		coord(int) const;
    int			trcNr(int) const;
    float		offset(int) const;
    bool		isNull(int) const;
    bool		isUsable(int) const;

    void		add(const TraceInfo&);
    void		addEnded(); //!< causes dataChange() for DataPointSet

    void		getReport(IOPar&) const;

protected:

    DataPointSet&	data_;
};


class FileDataSet : public ObjectSet<FileData>
{
public:

    struct TrcIdx
    {
			TrcIdx( int fnr=-1, int tnr=0 )
			    : filenr_(fnr), trcnr_(tnr)	{}
	bool		isValid() const			{ return filenr_>=0; }
	void		toNextFile()			{ filenr_++; trcnr_=0; }

	int		filenr_;
	int		trcnr_;
    };

    			FileDataSet()			{}
    			FileDataSet( const FileDataSet& fds )
			    				{ *this = fds; }
    			~FileDataSet()			{ deepErase(*this); }
    FileDataSet&	operator =(const FileDataSet&);

    bool		toNext(TrcIdx&,bool allownull=true,
	    			bool allownotusable=false) const;

    void		getReport(IOPar&) const;

};

} // namespace


#endif
