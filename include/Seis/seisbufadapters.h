#ifndef seisbufadapters_h
#define seisbufadapters_h

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Feb 2007
 RCS:		$Id: seisbufadapters.h,v 1.13 2011-04-24 10:06:45 cvsbert Exp $
________________________________________________________________________

*/


#include "seisbuf.h"
#include "arraynd.h"
#include "datapackbase.h"
#include "seisinfo.h"


/*!\brief Array2D based on SeisTrcBuf. */

mClass SeisTrcBufArray2D : public Array2D<float>
{
public:

    			SeisTrcBufArray2D(const SeisTrcBuf&);
    			SeisTrcBufArray2D(SeisTrcBuf&,bool mine,int compnr=0);
			~SeisTrcBufArray2D();

    bool		isOK() const		{ return true; }

    const Array2DInfo&	info() const		{ return info_; }
    float*		getData() const		{ return 0; }
    void		set(int,int,float);
    float		get(int,int) const;

    void		getAuxInfo(Seis::GeomType,int,IOPar&) const;

    SeisTrcBuf&		trcBuf()		{ return buf_; }
    const SeisTrcBuf&	trcBuf() const		{ return buf_; }

    void		setComp( int ic )	{ comp_ = ic; }
    int			getComp() const		{ return comp_; }

    void		setBufMine( bool yn )	{ bufmine_ = yn; }

protected:

    SeisTrcBuf&		buf_;
    Array2DInfo&	info_;
    bool		bufmine_;
    int			comp_;

};


/*!\brief FlatDataPack based on SeisTrcBuf. */

mClass SeisTrcBufDataPack : public FlatDataPack
{
public:

    			SeisTrcBufDataPack(SeisTrcBuf*,Seis::GeomType,
					   SeisTrcInfo::Fld,const char* categry,
					   int compnr=0);
			SeisTrcBufDataPack(const SeisTrcBufDataPack&);

    void		setBuffer(SeisTrcBuf*,Seis::GeomType,SeisTrcInfo::Fld,
	    			  int icomp=0);

    bool		getCubeSampling(CubeSampling&) const;

    const char*		dimName(bool) const;
    Coord3		getCoord(int,int) const;
    void		getAltDim0Keys(BufferStringSet&) const;
    double		getAltDim0Value(int,int) const;
    void		getAuxInfo(int,int,IOPar&) const;
    bool		posDataIsCoord() const		{ return false; }

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


#endif
