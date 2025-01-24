#pragma once
/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "seismod.h"

#include "arraynd.h"
#include "datapackbase.h"
#include "odcommonenums.h"
#include "seisbuf.h"
#include "seisinfo.h"

class TrcKeyZSampling;


/*!\brief Array2D based on SeisTrcBuf. */

mExpClass(Seis) SeisTrcBufArray2D : public Array2D<float>
{
public:

			SeisTrcBufArray2D(SeisTrcBuf*,bool mine,int compnr);
			SeisTrcBufArray2D(const SeisTrcBuf*,int compnr);
			~SeisTrcBufArray2D();

    bool		isOK() const override		{ return true; }

    const Array2DInfo&	info() const override		{ return *info_; }
    void		set(int,int,float) override;
    float		get(int,int) const override;

    SeisTrcBuf&		trcBuf()		{ return *buf_; }
    const SeisTrcBuf&	trcBuf() const		{ return *buf_; }

    void		setComp( int ic )	{ comp_ = ic; }
    int			getComp() const		{ return comp_; }

    bool		bufIsMine() const	{ return bufmine_; }
    void		setBufMine( bool yn )	{ bufmine_ = yn; }

protected:

    const float*	getData_() const override	{ return nullptr; }

    SeisTrcBuf*		buf_;
    Array2DInfo*	info_;
    bool		bufmine_;
    int			comp_;

};


/*!\brief FlatDataPack based on SeisTrcBuf. */

mExpClass(Seis) SeisTrcBufDataPack : public FlatDataPack
{
public:
			SeisTrcBufDataPack(SeisTrcBuf*,Seis::GeomType,
					   SeisTrcInfo::Fld,const char* categry,
					   const ZDomain::Info&,int compnr=0);
			//!< buf becomes mine
			SeisTrcBufDataPack(const SeisTrcBuf&,Seis::GeomType,
					   SeisTrcInfo::Fld,const char* categry,
					   const ZDomain::Info&,int compnr=0);
			//!< buf stays yours (and must remain alive!)
			SeisTrcBufDataPack(const SeisTrcBufDataPack&);

    SeisTrcBufDataPack& setBuffer(SeisTrcBuf*,Seis::GeomType,SeisTrcInfo::Fld,
				  int icomp=0,bool manage_buf=true);
    SeisTrcBufDataPack& setOffsetType(Seis::OffsetType);
    SeisTrcBufDataPack& setAzimuthAngleType(OD::AngleType);

    bool		getTrcKeyZSampling(TrcKeyZSampling&) const;
    Seis::OffsetType	offsetType() const;
    OD::AngleType	azimuthAngleType() const;

    uiString		dimName(bool) const override;
    uiString		dimUnitLbl(bool dim0,bool display,bool abbreviated=true,
				   bool withparentheses=true) const override;
    const UnitOfMeasure* dimUnit(bool dim0,bool display) const override;

    TrcKey		getTrcKey(int itrc,int isamp) const override;
    Coord3		getCoord(int itrc,int isamp) const override;
    double		getZ(int itrc,int isamp) const override;
    void		getAltDimKeys(uiStringSet&,bool dim0) const override;
    void		getAltDimKeysUnitLbls(uiStringSet&,bool dim0,
				    bool abbreviated=true,
				    bool withparentheses=true) const override;
    double		getAltDimValue(int key,bool dim0,int i0) const override;
    bool		dimValuesInInt(const uiString& key,
				       bool dim0) const override;
    void		getAuxInfo(int,int,IOPar&) const override;
    bool		posDataIsCoord() const override { return false; }

    SeisTrcBufArray2D&	trcBufArr2D()
			{ return *((SeisTrcBufArray2D*)arr2d_); }
    const SeisTrcBufArray2D& trcBufArr2D() const
			{ return *((SeisTrcBufArray2D*)arr2d_); }
    SeisTrcBuf&		trcBuf()
			{ return trcBufArr2D().trcBuf(); }
    const SeisTrcBuf&	trcBuf() const
			{ return trcBufArr2D().trcBuf(); }

protected:
			~SeisTrcBufDataPack();

    Seis::GeomType		gt_;
    Seis::OffsetType		offsettype_;
    OD::AngleType		azimuthangletype_ = OD::AngleType::Degrees;
    SeisTrcInfo::Fld		posfld_;
    TypeSet<SeisTrcInfo::Fld>	flds_;

};
