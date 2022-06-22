#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Feb 2007
________________________________________________________________________

*/


#include "seismod.h"
#include "seisbuf.h"
#include "arraynd.h"
#include "datapackbase.h"
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

    void		getAuxInfo(Seis::GeomType,int,IOPar&) const;

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
					   int compnr=0);
			//!< buf becomes mine
			SeisTrcBufDataPack(const SeisTrcBuf&,Seis::GeomType,
					   SeisTrcInfo::Fld,const char* categry,
					   int compnr=0);
			//!< buf stays yours (and must remain alive!)
			SeisTrcBufDataPack(const SeisTrcBufDataPack&);

    void		setBuffer(SeisTrcBuf*,Seis::GeomType,SeisTrcInfo::Fld,
				  int icomp=0,bool manage_buf=true);

    bool		getTrcKeyZSampling(TrcKeyZSampling&) const;

    const char*		dimName(bool) const override;
    Coord3		getCoord(int,int) const override;
    void		getAltDim0Keys(BufferStringSet&) const override;
    bool		dimValuesInInt(const char* key) const override;
    double		getAltDim0Value(int,int) const override;
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

    Seis::GeomType		gt_;
    SeisTrcInfo::Fld		posfld_;
    TypeSet<SeisTrcInfo::Fld>	flds_;

};


