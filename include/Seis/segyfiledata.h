#ifndef segyfiledata_h
#define segyfiledata_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bert
 Date:		Sep 2008
 RCS:		$Id: segyfiledata.h,v 1.6 2008-11-25 11:37:46 cvsbert Exp $
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
			    : pos_(gt), null_(false), usable_(true)	{}
    bool	operator ==( const TraceInfo& ti ) const
			{ return pos_ == ti.pos_; }

    Seis::PosKey	pos_;
    Coord		coord_;
    bool		null_;
    bool		usable_;

    inline BinID	binID() const		{ return pos_.binID(); }
    inline int		trcNr() const		{ return pos_.trcNr(); }
    inline float	offset() const		{ return pos_.offset(); }
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

    inline BinID	binID( int i ) const	{ return (*this)[i].binID(); }
    inline Coord	coord( int i ) const	{ return (*this)[i].coord_; }
    inline int		trcNr( int i ) const	{ return (*this)[i].trcNr(); }
    inline float	offset( int i ) const	{ return (*this)[i].offset(); }
    inline bool		isNull( int i ) const	{ return (*this)[i].null_; }
    inline bool		isUsable( int i ) const	{ return (*this)[i].usable_; }

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
