#ifndef transform_h
#define transform_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id: transform.h,v 1.11 2010-04-30 20:36:25 cvskris Exp $
________________________________________________________________________


-*/

#include "complex"
#include "sets.h"

class ArrayNDInfo;
template <class T> class ArrayND;

typedef std::complex<float> float_complex;

/*!\brief A TransformND is an object that can perform an transform of any kind.
\par
Usage:
1. Set size, and direction.
2. Run init().
3. Use and be happy. If size or direction is changed, init() must be rerun.

\note
1) It is not guarantied that all sizes are supported, nor that all kinds
of data (real & complex) or that the transform is bidirectional (reversible).
This must be queried before usage.

\par
2) That input and output sizes must be the one that was specified on startup.
This is _not_ checked at runtime. Also note that both input and output should
be able to give a pointer to its C-structured data. This is not neccesarly
checked at runtime.

*/

mClass TransformND
{
public:
    virtual bool		setInputInfo( const ArrayNDInfo& )	= 0;
    virtual const ArrayNDInfo&	getInputInfo() const			= 0;
    				/*!< Don't run before setInputInfo() has been
				     run. */
    virtual const ArrayNDInfo&	getOutputInfo() const { return getInputInfo(); }
    				/*!< Don't run before setInputInfo() has been
				     run. */

    virtual bool	real2real() const				= 0;
    virtual bool	real2complex() const				= 0;
    virtual bool	complex2real() const				= 0;
    virtual bool	complex2complex() const				= 0;

			//! Says whether the transform can be run in both dirs
    virtual bool	bidirectional() const				= 0;
    virtual bool	setDir(bool forward)				= 0;
    virtual bool	getDir() const					= 0;

    virtual bool	init() { return true; }

    virtual bool	isPossible( const ArrayNDInfo& ) const;
    virtual void	getNearBigPsblSz(const ArrayNDInfo&,
					   ArrayNDInfo& ) const;

    virtual bool	isFast( const ArrayNDInfo& ) const;
    virtual void	getNearBigFastSz(const ArrayNDInfo&,
					   ArrayNDInfo& ) const;

    virtual bool	transform(const ArrayND<float>&,
			    ArrayND<float>&) const	   { return false; }
    virtual bool	transform(const ArrayND<float>&,
			    ArrayND<float_complex>&) const { return false; }
    virtual bool	transform(const ArrayND<float_complex>&,
				  ArrayND<float_complex>&) const {return false;}
    virtual bool	transform(const ArrayND<float_complex>&,
				  ArrayND<float>&) const   { return false; }

protected:
    virtual bool	isPossible(int) const				= 0;
    virtual bool	isFast(int) const				= 0;
    virtual int		getNearBigFastSz(int) const;
    virtual int		getNearBigPsblSz(int) const;
};

/*!\brief
GenericTransformND is a subclass of TransformND that supports orthogonal
multidimensional transforms.
\par
It lets any 1D orthogonal transform
(GenericTransformND::GenericTransform1D) be extended to ND. Most transform
(i.e. all that don't have an own library as fftw can be implemented as a
subclass of GenericTransformND.
*/

mClass GenericTransformND : public TransformND
{
public:
    bool			setInputInfo( const ArrayNDInfo& );
    const ArrayNDInfo&		getInputInfo() const;

    bool			setDir( bool forward );
    bool			getDir() const;

    bool			init();

    bool			transform( const ArrayND<float>&,
					   ArrayND<float>& ) const;
    bool			transform(const ArrayND<float_complex>&,
				          ArrayND<float_complex>& ) const;

				GenericTransformND();
				~GenericTransformND();
mProtected:

    class			Transform1D;
    virtual Transform1D*	createTransform() const		= 0;

    void			transformND( const float*, float*,
					     int ndim) const;
    void			transformND( const float_complex*,
					     float_complex*,
					     int ndim) const;

    bool				forward;

    ObjectSet<Transform1D>		transforms;
    ObjectSet<Transform1D>		owntransforms;
    ArrayNDInfo*			info;

    
    mClass Transform1D
    {
    public:
	virtual void		setSize(int) 				= 0;
	virtual int		getSize() const				= 0;
	virtual void		setDir(bool forward)			= 0;
	virtual	bool		getDir() const				= 0;

	virtual bool		init()					= 0;
				//! \param space gives the number of samples
				//! between the tranformed samples. I.e.
				//! every Nth sample is transformed.
	virtual void		transform1D( const float_complex*,
					     float_complex*,
					     int space) const		= 0;

				//! \param space gives the number of samples
				//! between the tranformed samples. I.e.
				//! every Nth sample is transformed.
	virtual void		transform1D( const float*, float*,
					     int space) const		= 0;

    };
};

#endif
