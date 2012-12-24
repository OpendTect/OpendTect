#ifndef extremefinder_h
#define extremefinder_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "algomod.h"
#include "task.h"
#include "ranges.h"
#include "mathfunc.h"


/*!
\ingroup Algo
\brief Finds extreme values in FloatMathFunctions.

  Implementation of Brent's Method in one dimension.
*/

mClass(Algo) ExtremeFinder1D : public SequentialTask
{
public:
    			ExtremeFinder1D( const FloatMathFunction& func,
					 bool max, int itermax, float tol,
					 const Interval<float>& startinterval,
					 const Interval<float>* limitinterval);
			/*!<\param func		The function, f(x) where the
						extreme value shoud be found
			    \param max		Specifies wether a min or max
			     			value should be found
			    \param itermax	Maximum number of iterations
			    \param tol		Tolerance on the function
			     			variable (x)
			    \param startinterval The interval of x that the
			    			search will be inited by.
			    			Note that the search can go
						outside of this interval.
			    \param limitinterval Set to true if only solutions
			    			within the interval is permitted
			*/

    virtual		~ExtremeFinder1D();

    void		reStart( const Interval<float>& startinterval,
				 const Interval<float>* limitinterval);
    			/*!<
			    \param startinterval The interval of x that the
			    			search will be inited by.
			    			Note that the search can go
						outside of this interval.
			    \param limitinterval Set to true if only solutions
			    			within the interval is permitted
			*/

    float		extremePos() const;
    			/*!<\returns The x value of the extreme value */
    float		extremeVal() const;
    			/*!<\returns The extreme value */

    int			nrIter() const;
    			/*!<\returns The number of iterations */

    int			nextStep();
    			/*!<Will move the current extremePos one step towards
			    the solution.
			    \retval 0	Finished
			    \retval 1	More to do
			    \retval -1	Error (no extreme found or
			   		itermax reached The extreme value)
			*/

protected:

    float 			ax_,bx_,cx_;
    float			e_, d_;
    float			a_, b_;
    float			u_, w_, v_, x_;
    float			fw_, fv_, fx_;

    Interval<float>*		limits_;
    int				iter_;
    const float 		tol_;
    const FloatMathFunction&	func_;
    const bool			max_;
    const int			itermax_;
};


/*!
\ingroup Algo
\brief Bisection Extreme Finder
*/

mClass(Algo) BisectionExtremeFinder1D : public SequentialTask
{
public:
    			BisectionExtremeFinder1D(
					const FloatMathFunction&,
					bool max, int itermax, float tol,
					const Interval<float>& startinterval,
					const Interval<float>* limitinterval);
			/*!<\param func		The function, f(x) where the
						extreme value shoud be found
			    \param max		Specifies wether a min or max
			     			value should be found
			    \param itermax	Maximum number of iterations
			    \param tol		Tolerance on the function
			     			variable (x)
			    \param startinterval The interval of x that the
			    			search will be inited by.
			    			Note that the search can go
						outside of this interval.
			    \param limitinterval Set to true if only solutions
			    			within the interval is permitted
			*/

    virtual		~BisectionExtremeFinder1D();

    void		reStart( const Interval<float>& startinterval,
				 const Interval<float>* limitinterval);
    			/*!<
			    \param startinterval The interval of x that the
			    			search will be inited by.
			    			Note that the search can go
						outside of this interval.
			    \param limitinterval Set to true if only solutions
			    			within the interval is permitted
			*/

    float		extremePos() const;
    			/*!<\returns The x value of the extreme value */
    float		extremeVal() const;
    			/*!<\returns The extreme value */

    int			nrIter() const;
    			/*!<\returns The number of iterations */

    int			nextStep();
    			/*!<Will move the current extremePos one step towards
			    the solution.
			    \retval 0	Finished
			    \retval 1	More to do
			    \retval -1	Error (no extreme found or
			   		itermax reached The extreme value)
			*/

protected:
    Interval<float>*		limits_;
    int				iter;
    const float 		tol;
    const bool			max;
    const int			itermax;

    Interval<float>		current;
    float			startfuncval;
    float			stopfuncval;
    float			centerfuncval;

    bool			isok;
    const FloatMathFunction&	func;
};


/*!
\ingroup Algo
\brief Finds the nearest local extreme position in ND's.  
Implementation of Powell's Quadratically Convergent Method
\note The implementation is not tested (yet) 030512.
*/

mClass(Algo) ExtremeFinderND : public SequentialTask
{

public:

    			ExtremeFinderND( const FloatMathFunctionND&,
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
			    for ( int idx=0; idx<n_; idx++ )
			    	p_[idx] = sp[idx];
			}

    int			nextStep();
    			/*<!\retval 0	Finished
			    \retval 1	More to do
			    \retval -1	Error (no extreme found or
			   		itermax reached The extreme value)
			*/

    int			nrIter() const;
    			/*!<\return	The number of iterations */

    float 		extremeVal() { return fret_; }
    			/*!<\return	The extreme value */
    const float*	extremePos() { return p_; }
    			/*!<\return	A pointer to the extreme positions */

    template<class IDXABL>	
    void		extremePos( IDXABL& sp ) const
    			{
			    for ( int idx=0; idx<n_; idx++ )
			    	sp[idx] = p_[idx];
			}
    			/*!<\brief Sets the sp variable to the extreme position
			*/
private:

    float		linExtreme(float*);

    float*		p_;
    ObjectSet<float>	xi_;
    float		ftol_;
    int			n_;
    int			iter_;
    float		fret_;

    float*		pt_;

    const bool 			max_;
    const int			itermax_;
    const FloatMathFunctionND&	func_;
};


#endif

