#ifndef extremefinder_h
#define extremefinder_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: extremefinder.h,v 1.2 2003-11-07 12:21:50 bert Exp $
________________________________________________________________________


-*/

#include "basictask.h"
#include "sets.h"

template<class T> class Interval;
template<class T> class MathFunctionND;
template<class T> class MathFunction;

/*!\brief Finds extreme values in MathFunctions

Implementations is inspired from Numerical Recipies, where it is called
"Brent's Method in one dimension"

\note The implementation is not tested (yet) 030512.
*/

class ExtremeFinder1D : public BasicTask
{
public:
    			ExtremeFinder1D( const MathFunction<float>&,
					 bool max, int itermax, double tol,
					 const Interval<double>& interval );
			/*!<\param func		The function, f(x) where the
						extreme value shoud be found
			    \param max		Specifies wether a min or max
			     			value should be found
			    \param itermax	Maximum number of iterations
			    \param tol		Tolerance on the function
			     			variable (x)
			    \param interval	The interval of x that will be
			    			searched
			*/

    virtual		~ExtremeFinder1D();
    double		extremePos() const;
    			/*!<\returns The x value of the extreme value */
    float		extremeVal() const;
    			/*!<\returns The extreme value */

    int			nrIter() const;
    			/*!<\returns The number of iterations */

    int			nextStep();
    			/*<!\retval 0	Finished
			    \retval 1	More to do
			    \retval -1	Error (no extreme found or
			   		itermax reached The extreme value)
			*/


protected:
    double 		ax,bx,cx;
    double		e, d;
    double		a, b;
    double		u, w, v, x;
    float		fw, fv, fx;

    int				iter;
    const double 		tol;
    const MathFunction<float>&	func;
    const bool			max;
    const int			itermax;
};


/*!\brief Finds the nearest local extreme position in ND's

Implementations is inspired from Numerical Recipies, where the method is
called "Powell's Quadratically Convergent Method"

\note The implementation is not tested (yet) 030512.
*/

class ExtremeFinderND : public BasicTask
{

public:

    			ExtremeFinderND( const MathFunctionND<float>& func,
					 bool max, int itermax );
			/*!<\param func		The function to be searched
			    \param max		Specifies wether a min or max
			    			should be searched for
			    \param itermax	The maximum number of iterations
			*/
    virtual		~ExtremeFinderND();

    template<class IDXABL>	
    void		setStartPos( const IDXABL& sp )
    			{
			    for ( int idx=0; idx<n; idx++ )
			    	p[idx] = sp[idx];
			}

    int			nextStep();
    			/*<!\retval 0	Finished
			    \retval 1	More to do
			    \retval -1	Error (no extreme found or
			   		itermax reached The extreme value)
			*/

    int			nrIter() const;
    			/*!<\return	The number of iterations */

    float 		extremeVal() { return fret; }
    			/*!<\return	The extreme value */
    const double*	extremePos() { return p; }
    			/*!<\return	A pointer to the extreme positions */

    template<class IDXABL>	
    void		extremePos( IDXABL& sp ) const
    			{
			    for ( int idx=0; idx<n; idx++ )
			    	sp[idx] = p[idx];
			}
    			/*!<\brief Sets the sp variable to the extreme position
			*/
private:

    float		linextreme(double*);

    double*		p;
    ObjectSet<float>	xi;
    float		ftol;
    int			n;
    int			iter;
    float		fret;

    float*		pt;

    const bool 				max;
    const int				itermax;
    const MathFunctionND<float>&	func;
};

#endif

