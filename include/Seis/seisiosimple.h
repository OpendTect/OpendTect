#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2003
 * SVN      : $Id$
-*/

#include "seismod.h"
#include "samplingdata.h"
#include "multiid.h"
#include "position.h"
#include "executor.h"
#include "seistype.h"
#include "od_iosfwd.h"
#include "uistring.h"
#include "coordsystem.h"

class Scaler;
class SeisTrc;
class LineKey;
class SeisImporter;
class SeisTrcReader;
class SeisTrcWriter;
class SeisResampler;
namespace Seis { class SelData; }


mExpClass(Seis) SeisIOSimple : public Executor
{ mODTextTranslationClass(SeisIOSimple);
public:

    mExpClass(Seis) Data
    {
    public:
			Data(const char*,Seis::GeomType);
			Data(const Data&);
			~Data();
	Data&		operator =(const Data&);

	BufferString	fname_;
	MultiID		seiskey_;

	bool		isasc_;
	Seis::GeomType	geom_;

	bool		havepos_;
	bool		isxy_;

	bool		havenr_;
	SamplingData<int> nrdef_;

	bool		haverefnr_;

	bool		havesd_;
	SamplingData<float> sd_;
	int		nrsamples_;
	int		compidx_;

			// PS only
	bool		haveoffs_;
	SamplingData<float> offsdef_;
	int		nroffsperpos_;
	bool		haveazim_;

			// 3D only
	SamplingData<int> inldef_;
	SamplingData<int> crldef_;
	int		nrcrlperinl_;

			// 2D only
	Coord		startpos_;
	Coord		steppos_;
	LineKey&	linekey_;

	Scaler*		scaler_;
	SeisResampler*	resampler_;
	bool		remnull_;
	IOPar&		subselpars_;

	void		clear(bool survchanged);
	void		setScaler(Scaler*);
			//!< passed obj will be cloned
	void		setResampler(SeisResampler*);
			//!< passed obj will become mine
	ConstRefMan<Coords::CoordSystem> const getCoordSys()
							{ return coordsys_; }
	void			   setCoordSys(const Coords::CoordSystem*crs)
				    { coordsys_ = crs; }
    protected:
	ConstRefMan<Coords::CoordSystem> coordsys_;
    };

			SeisIOSimple(const Data&,bool imp);
			~SeisIOSimple();

    int			nextStep();
    uiString		uiMessage() const;
    od_int64		nrDone() const;
    od_int64		totalNr() const;
    uiString		uiNrDoneText() const;

protected:

    Data		data_;
    bool		isimp_;
    bool		isps_;

    SeisTrc&		trc_;
    od_stream*		strm_;
    SeisTrcReader*	rdr_;
    SeisTrcWriter*	wrr_;
    SeisImporter*	importer_;
    bool		firsttrc_;
    int			nrdone_;
    int			offsnr_;
    int			prevnr_;
    BinID		prevbid_;
    uiString		errmsg_;
    const bool		zistm_;

    void		startImpRead();
    int			readImpTrc(SeisTrc&);
    int			readExpTrc();
    int			writeExpTrc();
    od_istream&		iStream();
    od_ostream&		oStream();

    friend class	SeisIOSimpleImportReader;

};


