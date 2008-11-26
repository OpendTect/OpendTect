#ifndef segyfiledata_h
#define segyfiledata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2008
 RCS:		$Id: segyfiledata.h,v 1.7 2008-11-26 12:50:46 cvsbert Exp $
________________________________________________________________________

-*/

#include "bufstring.h"
#include "position.h"
#include "seisposkey.h"
#include "samplingdata.h"
#include "iopar.h"

class ascostream;
 

namespace SEGY
{

struct TraceInfo
{
			TraceInfo( Seis::GeomType gt=Seis::Vol )
			    : pos_(gt), usable_(true)	{}
    virtual		~TraceInfo()			{}
    virtual TraceInfo*	clone() const		{ return new TraceInfo(*this); }
    bool		operator ==( const TraceInfo& ti ) const
			{ return pos_ == ti.pos_; }

    Seis::PosKey	pos_;
    bool		usable_;

    inline BinID	binID() const		{ return pos_.binID(); }
    inline int		trcNr() const		{ return pos_.trcNr(); }
    inline float	offset() const		{ return pos_.offset(); }
    inline bool		isUsable() const	{ return usable_; }

    virtual bool	isRich() const		{ return false; }
    virtual Coord	coord() const		{ return Coord(0,0); }
    virtual float	azimuth() const		{ return 0; }
    virtual bool	isNull() const		{ return false; }
};


struct RichTraceInfo : public TraceInfo
{
			RichTraceInfo( Seis::GeomType gt=Seis::Vol )
			    : TraceInfo(gt), azimuth_(0), null_(false)	{}
    virtual TraceInfo*	clone() const	{ return new RichTraceInfo(*this); }
    bool		operator ==( const RichTraceInfo& ti ) const
			{ return pos_ == ti.pos_; }
    bool		operator ==( const TraceInfo& ti ) const
			{ return pos_ == ti.pos_; }

    virtual bool	isRich() const		{ return true; }
    virtual Coord	coord() const		{ return coord_; }
    virtual float	azimuth() const		{ return azimuth_; }
    virtual bool	isNull() const		{ return null_; }

    Coord		coord_;
    float		azimuth_;
    bool		null_;
};


/*\brief Data usually obtained by scanning a SEG-Y file. */

class FileData : public ObjectSet<TraceInfo>
{
public:

    			FileData(const char* fnm,Seis::GeomType);
			FileData( const FileData& fd )	{ *this = fd; }
			~FileData()			{ deepErase(*this); }
    FileData&		operator =(const FileData&);

    BufferString	fname_;
    Seis::GeomType	geom_;
    int			trcsz_;
    SamplingData<float>	sampling_;
    int			segyfmt_;
    bool		isrev1_;
    int			nrstanzas_;

#define mSEGYFileDataDefFn(ret,nm) \
    inline ret		nm( int idx ) const	{ return (*this)[idx]->nm(); }
			mSEGYFileDataDefFn(BinID,binID)
			mSEGYFileDataDefFn(Coord,coord)
			mSEGYFileDataDefFn(int,trcNr)
			mSEGYFileDataDefFn(float,offset)
			mSEGYFileDataDefFn(float,azimuth)
			mSEGYFileDataDefFn(bool,isNull)
			mSEGYFileDataDefFn(bool,isUsable)
#undef mSEGYFileDataDefFn

    bool		isRich() const;
    int			nrNullTraces() const;
    int			nrUsableTraces() const;

    void		getReport(IOPar&) const;
    bool		getFrom(ascistream&);
    bool		putTo(ascostream&) const;

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
