#ifndef seisbufadapters_h
#define seisbufadapters_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Feb 2007
 RCS:		$Id: seisbufadapters.h,v 1.5 2007-10-03 12:53:55 cvskris Exp $
________________________________________________________________________

*/


#include "seisbuf.h"
#include "arraynd.h"
#include "datapackbase.h"
#include "seisinfo.h"


/*!\brief Array2D based on SeisTrcBuf. */

class SeisTrcBufArray2D : public Array2D<float>
{
public:

    			SeisTrcBufArray2D(const SeisTrcBuf&,int compnr=0);
    			SeisTrcBufArray2D(SeisTrcBuf&,bool mine,int compnr=0);
			~SeisTrcBufArray2D();

    bool		isOK()			{ return true; }

    const mPolyArray2DInfoTp&	info() const	{ return info_; }
    float*		getData() const		{ return 0; }
    void		set(int,int,float);
    float		get(int,int) const;

    void		getAuxInfo(Seis::GeomType,int,IOPar&) const;

    SeisTrcBuf&		trcBuf()		{ return buf_; }
    const SeisTrcBuf&	trcBuf() const		{ return buf_; }

protected:

    SeisTrcBuf&		buf_;
    mPolyArray2DInfoTp&	info_;
    bool		bufmine_;
    int			comp_;

};


/*!\brief FlatDataPack based on SeisTrcBuf. */

class SeisTrcBufDataPack : public FlatDataPack
{
public:

    			SeisTrcBufDataPack(SeisTrcBuf*,Seis::GeomType,
					   SeisTrcInfo::Fld,const char* categry,
					   int compnr=0);

    void		setBuffer(SeisTrcBuf*,Seis::GeomType,SeisTrcInfo::Fld,
	    			  int icomp=0);

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
