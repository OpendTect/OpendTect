#ifndef segyfiledata_h
#define segyfiledata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2008
 RCS:		$Id: segyfiledata.h,v 1.3 2008-11-17 15:50:12 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "position.h"
#include "seistype.h"
#include "samplingdata.h"
#include "iopar.h"
 

namespace SEGY
{

struct TraceInfo
{
		TraceInfo()
		    : nr_(0), offset_(0)
		    , null_(false), usable_(true)		{}
    bool	operator ==( const TraceInfo& ti ) const
			{ return nr_ == ti.nr_ && binid_ == ti.binid_
			    			&& offset_ == ti.offset_; }

    int		nr_;
    BinID	binid_;
    Coord	coord_;
    float	offset_;
    bool	null_;
    bool	usable_;
};


/*\brief Data usually obtained by scanning a SEG-Y file. */

class FileData : public TypeSet<TraceInfo>
{
public:

    			FileData(const char* fnm,Seis::GeomType);

    BufferString	fname_;
    Seis::GeomType	geom_;
    int			trcsz_;
    SamplingData<float>	sampling_;
    int			segyfmt_;
    bool		isrev1_;
    int			nrstanzas_;

    inline BinID	binID( int i ) const	{ return (*this)[i].binid_; }
    inline Coord	coord( int i ) const	{ return (*this)[i].coord_; }
    inline int		trcNr( int i ) const	{ return (*this)[i].nr_; }
    inline float	offset( int i ) const	{ return (*this)[i].offset_; }
    inline bool		isNull( int i ) const	{ return (*this)[i].null_; }
    inline bool		isUsable( int i ) const	{ return (*this)[i].usable_; }

    int			nrNullTraces() const;
    int			nrUsableTraces() const;

    void		getReport(IOPar&) const;

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

    			FileDataSet( const IOPar& iop )	{ pars_ = iop; }
    			FileDataSet( const FileDataSet& fds )
			    				{ *this = fds; }
    			~FileDataSet()			{ deepErase(*this); }
    FileDataSet&	operator =(const FileDataSet&);

    bool		toNext(TrcIdx&,bool allownull=true,
	    			bool allownotusable=false) const;

    const IOPar&	pars() const			{ return pars_; }

protected:

    IOPar		pars_;

};

} // namespace


#endif
