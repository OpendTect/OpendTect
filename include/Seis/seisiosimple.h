#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

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
namespace Survey { class Geometry2D; }


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

	bool		isasc_ = true;
	Seis::GeomType	geom_;
	Pos::GeomID	geomid_;

	bool		havepos_ = true;
	bool		isxy_ = false;

	bool		havenr_ = false;
	bool		haverefnr_ = false;
	SamplingData<int> nrdef_;

	bool		havesd_ = false;
	SamplingData<float> sd_;
	int		nrsamples_;
	int		compidx_ = 0;

			// PS only
	bool		haveoffs_ = false;
	SamplingData<float> offsdef_;
	int		nroffsperpos_;
	bool		haveazim_ = false;

			// 3D only
	SamplingData<int> inldef_;
	SamplingData<int> crldef_;
	int		nrcrlperinl_;

			// 2D only
	Coord		startpos_;
	Coord		steppos_;
	BufferString	linename_;

	Scaler*		scaler_ = nullptr;
	SeisResampler*	resampler_ = nullptr;
	bool		remnull_ = false;
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

    int			nextStep() override;
    uiString		uiMessage() const override;
    od_int64		nrDone() const override;
    od_int64		totalNr() const override;
    uiString		uiNrDoneText() const override;

protected:

    Data		data_;
    bool		isimp_;
    bool		isps_;

    SeisTrc&		trc_;
    od_stream*		strm_ = nullptr;
    SeisTrcReader*	rdr_ = nullptr;
    SeisTrcWriter*	wrr_ = nullptr;
    SeisImporter*	importer_ = nullptr;
    bool		firsttrc_ = true;
    int			nrdone_ = 0;
    int			offsnr_ = 0;
    int			prevnr_;
    BinID		prevbid_;
    uiString		errmsg_;
    const bool		zistm_;

    const Survey::Geometry2D*	geom2d_ = nullptr;

    void		startImpRead();
    int			readImpTrc(SeisTrc&);
    int			readExpTrc();
    int			writeExpTrc();
    od_istream&		iStream();
    od_ostream&		oStream();

    friend class	SeisIOSimpleImportReader;

};
