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
#include "cubesampling.h"
#include "refcount.h"
#include "samplingdata.h"
#include "bindatadesc.h"


template <class T> class Array3D;
template <class T> class Array3DImpl;

namespace Attrib
{

/*!
  \ingroup AttributeEngine
  \brief A class for holding cubes with attribute data. All the cubes have
  the same sampling and size.
*/

mClass(AttributeEngine) DataCubes
{ mRefCountImpl(DataCubes);
public:
    				DataCubes();

    int				nrCubes() const { return cubes_.size(); }
    bool			addCube(const BinDataDesc* bdd=0);
    bool			addCube(float,const BinDataDesc* bdd=0);
    				//!<Adds the cube and inits it to the given val.
    void			removeCube(int);

    bool			setSizeAndPos(const CubeSampling&);
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
    bool			includes(const CubeSampling&) const;


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
    int				getInlSz() const 	{ return inlsz_; }
    int				getCrlSz() const	{ return crlsz_; }
    int				getZSz() const		{ return zsz_; }

    SamplingData<int>		inlsampling_;
    SamplingData<int>		crlsampling_;
    int				z0_;
    double			zstep_;
    CubeSampling		cubeSampling() const;
    				/*!<For convenience. The samling is changed
				    by setting inlsampling, crlsampling, z0 and
				    zstep. */

    static int			cInlDim()	{ return 0; }
    static int			cCrlDim()	{ return 1; }
    static int			cZDim()		{ return 2; }

protected:
    ObjectSet<Array3DImpl<float> >	cubes_;
    int					inlsz_;
    int					crlsz_;
    int					zsz_;
};


}; //namespace

#endif

