#ifndef samplfunc_h
#define samplfunc_h

#include <mathfunc.h>
#include <simpnumer.h>

class SampledFunction : public MathFunction
{
public:
    virtual float		operator[](int)	const			= 0;
    virtual void		set( int, float )			= 0;

    virtual float		getDx() const				= 0;
    virtual float		getX0() const				= 0;

    virtual int			size() const				= 0;

    float			getIndex(float x) const
				    { return (x-getX0()) / getDx(); }

    int				getNearestIndex(float x) const
				    { return mNINT(getIndex( x )); }

    float			getValue(double x) const
				{ return interpolateSampled( *this, size(), 
				     getIndex(x), false, 0); }

};

template <class T>
class SampledFunctionImpl : public SampledFunction
{
public:
			SampledFunctionImpl( const T& idxabl_, int sz_,
			    float x0_=0, float dx_=1 )
			    : idxabl( idxabl_ )
			    , sz( sz_ )
			    , x0( x0_ )
			    , dx( dx_ )
			{}

    float		operator[](int idx) const { return idxabl[idx]; }
    void		set( int idx, float val) { idxabl[idx] = val; }

    float		getDx() const { return dx; }
    float		getX0() const { return x0; }

    int			size() const { return sz; }

protected:
    const T&		idxabl;
    int			sz;
    int			firstidx;

    float		dx;
    float		x0;
};

#endif
