#ifndef attribdatacubes_h
#define attribdatacubes_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Apr 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "trckeyzsampling.h"
#include "refcount.h"
#include "samplingdata.h"
#include "bindatadesc.h"
#include "typeset.h"

class BinIDValue;

template <class T> class Array3D;
template <class T> class Array3DImpl;

namespace Attrib
{

/*!
\brief A class for holding cubes with attribute data. All the cubes have
the same sampling and size.
*/

mExpClass(AttributeEngine) DataCubes
{ mRefCountImpl(DataCubes);
public:
				DataCubes();

    int				nrCubes() const { return cubes_.size(); }
    bool			validCubeIdx(int) const;

    bool			addCube(const BinDataDesc* bdd=0);
    bool			addCube(float,const BinDataDesc* bdd=0);
				//!<Adds the cube and inits it to the given val.
    bool			addCube(Array3DImpl<float>&,bool manage=false);
				//!<Does not delete it when manage=false
    void			removeCube(int);

    bool			setSizeAndPos(const TrcKeyZSampling&);
				/*!<\note that all data will be lost. */
    bool			setSize(int nrinl,int nrcrl,int nrz);
				/*!<\note that all data will be lost. */
    void			setValue(int array,int inlidx,int crlidx,
					 int zidx,float val);
    void			setValue(int array,float val);
    bool			getValue(int array,const BinIDValue&,
					 float* res,bool interpolate) const;
    bool			includes(const BinIDValue&) const;
    bool			includes(const BinID&) const;
    bool			includes(const TrcKeyZSampling&) const;


    const Array3D<float>&	getCube(int idx) const;
				/*!<The attrib data. The data is always
				    organized with inl as the slowest dim,
				    and z as the fastest dim. Data can thus
				    be accessed by:
				    \code
				    float val = getCube(idx).get(inlidx,crlidx,
								  zidx);
				    \endcode */
    Array3D<float>&		getCube(int idx);
				/*!<The attrib data. The data is always
				    organized with inl as the slowest dim,
				    and z as the fastest dim. Data can thus
				    be accessed by:
				    \code
				    float val = getCube(idx).get(inlidx,crlidx,
								  zidx);
				    \endcode */
    void			setCube(int idx,const Array3D<float>&);
    int				getInlSz() const	{ return inlsz_; }
    int				getCrlSz() const	{ return crlsz_; }
    int				getZSz() const		{ return zsz_; }

    SamplingData<int>		inlsampling_;
    SamplingData<int>		crlsampling_;
    float			z0_;
    double			zstep_;
    TrcKeyZSampling		cubeSampling() const;
				/*!<For convenience. The samling is changed
				    by setting inlsampling, crlsampling, z0 and
				    zstep. */

    static int			cInlDim()	{ return 0; }
    static int			cCrlDim()	{ return 1; }
    static int			cZDim()		{ return 2; }

protected:
    ObjectSet<Array3DImpl<float> >	cubes_;
    BoolTypeSet				manage_;
    int					inlsz_;
    int					crlsz_;
    int					zsz_;
};

} // namespace Attrib

#endif
