#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Sep 2008
________________________________________________________________________

-*/

#include "seismod.h"
#include "filespec.h"
#include "coord.h"
#include "samplingdata.h"
#include "seistype.h"
#include "segythdef.h"
#include "binid.h"
#include "coordsystem.h"
#include "survinfo.h"

class IOObj;
class Scaler;
class SeisTrcInfo;


namespace SEGY
{
class TrcHeader;


/*\brief Definition of input / output file(s)  */

mExpClass(Seis) FileSpec : public ::FileSpec
{
public:

			FileSpec( const char* fnm=0 )
			    : ::FileSpec(fnm)		{}
			FileSpec( const IOPar& iop )
			    : ::FileSpec(iop)		{}

    IOObj*		getIOObj(bool temporary) const;

    static void		fillParFromIOObj(const IOObj&,IOPar&);

};


/*\brief Parameters that control the primary read/write process */

mExpClass(Seis) FilePars
{
public:
			FilePars( bool forread=true )
			    : fmt_(forread?0:1)
			    , forread_(forread)
			    , coordsys_(SI().getCoordSystem()) {}

    int			ns_ = 0;
    int			fmt_;
    int			byteswap_ = 0;	//!, 0=no 1=data only 2=all 3=only hdrs

    bool		swapHdrs() const	{ return byteswap_ > 1; }
    bool		swapData() const
			{ return byteswap_ == 1 || byteswap_ == 2; }
    void		setSwap( bool hdr, bool data )
			{ byteswap_ = hdr ? (data?2:3) : (data?1:0); }

    static int		nrFmts( bool forread )	{ return forread ? 6 : 5; }
    static const char**	getFmts(bool forread);
    static const char*	nameOfFmt(int fmt,bool forread);
    static int		fmtOf(const char*,bool forread);

    static const char*	sKeyForceRev0();
    static const char*	sKeyRevision();
    static const char*	sKeyNrSamples();
    static const char*	sKeyNumberFormat();
    static const char*	sKeyByteSwap();

    void		setForRead(bool);

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		getReport(IOPar&,bool isrev0) const;
    void		setCoordSys(const Coords::CoordSystem* crs)
							    { coordsys_ = crs; }
    ConstRefMan<Coords::CoordSystem> getCoordSys() const { return coordsys_; }
protected:

    bool		forread_;
    ConstRefMan<Coords::CoordSystem> coordsys_;

};


/*\brief Options that control the actual read process */

mExpClass(Seis) FileReadOpts
{
public:

			FileReadOpts(Seis::GeomType gt=Seis::Vol);

    Seis::GeomType	geomType() const	{ return geom_; }
    void		setGeomType(Seis::GeomType);

    enum ICvsXYType	{ Both=0, ICOnly=1, XYOnly=2 };
    enum PSDefType	{ InFile=0, SrcRcvCoords=1, UsrDef=2 };
    enum CoordDefType	{ Present=0, ReadFile=1, Generate=2 };

    bool		forread_;
    TrcHeaderDef	thdef_;
    float		coordscale_;
    float		timeshift_;
    float		sampleintv_;
    ICvsXYType		icdef_;

    bool		havetrcnrs_;
    SamplingData<int>	trcnrdef_;

    PSDefType		psdef_;
    SamplingData<float>	offsdef_;

    CoordDefType	coorddef_;
    Coord		startcoord_;
    Coord		stepcoord_;
    BufferString	coordfnm_;

    void		scaleCoord(Coord&,const Scaler* s=0) const;
    float		timeShift(float) const;
    float		sampleIntv(float) const;

    static const char*	sKeyCoordScale();
    static const char*	sKeyTimeShift();
    static const char*	sKeySampleIntv();
    static const char*	sKeyICOpt();
    static const char*	sKeyHaveTrcNrs();
    static const char*	sKeyTrcNrDef();
    static const char*	sKeyPSOpt();
    static const char*	sKeyOffsDef();
    static const char*	sKeyCoordOpt();
    static const char*	sKeyCoordStart();
    static const char*	sKeyCoordStep();
    static const char*	sKeyCoordFileName();

    void		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
    void		getReport(IOPar&,bool isrev0) const;
    static void		shallowClear(IOPar&);

protected:

    Seis::GeomType	geom_;

};


/*\brief Determines offset as specified by user */

mExpClass(Seis) OffsetCalculator
{
public:
			OffsetCalculator()
			    : type_(FileReadOpts::InFile)
			    , def_(0.f,1.f)
			    , is2d_(false)
			    , coordscale_(1.0f)		{ reset(); }
    void		set(const FileReadOpts&);

    void		reset();

    FileReadOpts::PSDefType type_;
    SamplingData<float>	def_;
    bool		is2d_;
    float		coordscale_;

    void		setOffset(SeisTrcInfo&,const TrcHeader&) const;

protected:

    mutable float	curoffs_;
    mutable BinID	prevbid_;

};



} // namespace SEGY


