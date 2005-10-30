#ifndef attribdatacubes_h
#define attribdatacubes_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Apr 2002
 RCS:           $Id: attribdatacubes.h,v 1.5 2005-10-30 22:03:24 cvskris Exp $
________________________________________________________________________

-*/

#include "arrayndimpl.h"
#include "cubesampling.h"
#include "refcount.h"
#include "samplingdata.h"

namespace Attrib
{

/*!\brief A class for holding cubes with attribute data. All the cubes have
the same sampling and size.
 */

class DataCubes
{ mRefCountImpl(DataCubes);
public:
    				DataCubes();

    int				nrCubes() const { return cubes.size(); }
    void			addCube();
    void			removeCube(int);

    void			setSizeAndPos( const CubeSampling& );
    				/*!<\note that all data will be lost. */
    void			setSize( int nrinl, int nrcrl, int nrz );
    				/*!<\note that all data will be lost. */
    void			setValue( int array, int inlidx, int crlidx,
	    				  int zidx, float val );
    bool			getValue( int array, const BinIDValue&,
	    				  float* res ) const;
    bool			includes( const BinIDValue& ) const;
    bool			includes( const BinID& ) const;


    const Array3D<float>&	getCube(int idx) const;
    				/*!<The attrib data. The data is always
				    organized with inl as the slowest dim,
				    and z as the fastest dim. Data can thus
				    be accessed by:
				    \code
				    float val = getCube(idx).get(inlidx,crlidx,
				    				  zidx);
				    \endcode */
    int				getInlSz() const 	{ return inlsz; }
    int				getCrlSz() const	{ return crlsz; }
    int				getZSz() const		{ return zsz; }

    SamplingData<int>		inlsampling;
    SamplingData<int>		crlsampling;
    int				z0;
    double			zstep;
    CubeSampling		cubeSampling() const;
    				/*!<For convenience. The samling is changed
				    by setting inlsampling, crlsampling, z0 and
				    zstep. */

    static int			cInlDim()	{ return 0; }
    static int			cCrlDim()	{ return 1; }
    static int			cZDim()		{ return 2; }

protected:
    ObjectSet<Array3DImpl<float> >	cubes;
    int					inlsz;
    int					crlsz;
    int					zsz;
};


}; //namespace

#endif
