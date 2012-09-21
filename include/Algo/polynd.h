#ifndef polynd_h
#define polynd_h

/*@+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          10-12-1999
 RCS:           $Id$
________________________________________________________________________

PolynomialND is a N-dimensional polynomial with arbitary orders in each
dimension. It can be fitted any ArrayND. To access the polynomial's data
use getValue. getValue3D is optimized for third order, tree-dimensional
cases.

@$*/

#include <linsolv.h>
#include <arrayndimpl.h>
#include <arrayndutils.h>
#include <simpnumer.h>

template <class T>
class PolynomialND
{
public:
    			PolynomialND( const ArrayNDInfo& );
    			~PolynomialND( );

    bool		fit( const ArrayND<T>& );

    void		setCoeff( const int* pos, T val )
			{ coeffs(pos) = val; }

    T			getCoeff( const int* pos ) const
			{ return coeffs.get( pos ); }

    T			getValue( const TypeSet<float>& ) const;
    T			getValue3D( float x0, float x1, float x2 ) const;

protected:
    ArrayNDImpl<T>	coeffs;
    LinSolver<T>*	solver;
}; 


template <class T>
PolynomialND<T>::PolynomialND( const ArrayNDInfo& size_ )
    : coeffs( size_ )
    , solver( 0 )
{}


template <class T>
PolynomialND<T>::~PolynomialND( )
{ delete solver; }


template <class T> inline
T PolynomialND<T>::getValue( const TypeSet<float>& pos ) const
{
    ArrayNDIter coeffiter( coeffs.info() );

    const int ndim = coeffs.info().getNDim();

    T res = 0;

    do
    {
	float posproduct = 1;	
	
	for ( int idx=0; idx<ndim; idx++ )
	{
	    posproduct *= intpow( pos[idx], coeffiter[idx] );
	}
     
	res += posproduct * coeffs.getND( coeffiter.getPos() );
    } while ( coeffiter.next() );

    return res;
}

template <class T> inline
T PolynomialND<T>::getValue3D( float p0, float p1, float p2 ) const
{
    const ArrayNDInfo& size = coeffs.info();
    if ( size.getNDim() != 3 || size.getTotalSz() != 64 )
    {
	TypeSet<float> pos( 3,0  );
	pos[0] = p0; pos[1] = p1; pos[2] = p2;

	return getValue( pos );
    }

    float p0_2 = p0 * p0; float p0_3 = p0_2 * p0;
    float p1_2 = p1 * p1; float p1_3 = p1_2 * p1;
    float p2_2 = p2 * p2; float p2_3 = p2_2 * p2;

    const float p0p1 =  p0 * p1;
    const float p0p1_2 = p0p1 * p1;
    const float p0p1_3 = p0p1_2 * p1;
    const float p0_2p1 =  p0_2 * p1;
    const float p0_2p1_2 = p0_2p1 * p1;
    const float p0_2p1_3 = p0_2p1_2 * p1;
    const float p0_3p1 =  p0_3 * p1;
    const float p0_3p1_2 = p0_3p1 * p1;
    const float p0_3p1_3 = p0_3p1_2 * p1;

    const T* ptr = coeffs.getData();

    T res = 	ptr[0] +
		ptr[1] * p2 +
		ptr[2] * p2_2 +
		ptr[3] * p2_3 +

    		ptr[4] * p1 +
		ptr[5] * p1 * p2 +
		ptr[6] * p1 * p2_2 +
		ptr[7] * p1 * p2_3 +

    		ptr[8] * p1_2 +
		ptr[9] * p1_2 * p2 +
		ptr[10] * p1_2 * p2_2 +
		ptr[11] * p1_2 * p2_3 +

    		ptr[12] * p1_3 +
		ptr[13] * p1_3 * p2 +
		ptr[14] * p1_3 * p2_2 +
		ptr[15] * p1_3 * p2_3 +


    		ptr[16] * p0 +
		ptr[17] * p0 * p2 +
		ptr[18] * p0 * p2_2 +
		ptr[19] * p0 * p2_3 +

    		ptr[20] * p0p1 +
		ptr[21] * p0p1 * p2 +
		ptr[22] * p0p1 * p2_2 +
		ptr[23] * p0p1 * p2_3 +

    		ptr[24] * p0p1_2 +
		ptr[25] * p0p1_2 * p2 +
		ptr[26] * p0p1_2 * p2_2 +
		ptr[27] * p0p1_2 * p2_3 +

    		ptr[28] * p0p1_3 +
		ptr[29] * p0p1_3 * p2 +
		ptr[30] * p0p1_3 * p2_2 +
		ptr[31] * p0p1_3 * p2_3 +


    		ptr[32] * p0_2 +
		ptr[33] * p0_2 * p2 +
		ptr[34] * p0_2 * p2_2 +
		ptr[35] * p0_2 * p2_3 +

    		ptr[36] * p0_2p1 +
		ptr[37] * p0_2p1 * p2 +
		ptr[38] * p0_2p1 * p2_2 +
		ptr[39] * p0_2p1 * p2_3 +

    		ptr[40] * p0_2p1_2 +
		ptr[41] * p0_2p1_2 * p2 +
		ptr[42] * p0_2p1_2 * p2_2 +
		ptr[43] * p0_2p1_2 * p2_3 +

    		ptr[44] * p0_2p1_3 +
		ptr[45] * p0_2p1_3 * p2 +
		ptr[46] * p0_2p1_3 * p2_2 +
		ptr[47] * p0_2p1_3 * p2_3 +



    		ptr[48] * p0_3 +
		ptr[49] * p0_3 * p2 +
		ptr[50] * p0_3 * p2_2 +
		ptr[51] * p0_3 * p2_3 +

    		ptr[52] * p0_3p1 +
		ptr[53] * p0_3p1 * p2 +
		ptr[54] * p0_3p1 * p2_2 +
		ptr[55] * p0_3p1 * p2_3 +

    		ptr[56] * p0_3p1_2 +
		ptr[57] * p0_3p1_2 * p2 +
		ptr[58] * p0_3p1_2 * p2_2 +
		ptr[59] * p0_3p1_2 * p2_3 +

    		ptr[60] * p0_3p1_3 +
		ptr[61] * p0_3p1_3 * p2 +
		ptr[62] * p0_3p1_3 * p2_2 +
		ptr[63] * p0_3p1_3 * p2_3;

    return res;
}


template<class T>
bool PolynomialND<T>::fit( const ArrayND<T>& input )
{
    const int totalsz = input.info().getTotalSz();

    if ( !solver || solver->size() != totalsz )
    {
	if ( solver ) delete solver;

	Array2DImpl<T> poscoeffs(totalsz,totalsz);

	ArrayNDIter positer( input.info() );
	const int ndim = input.info().getNDim();
	int row = 0; 
	do
	{
	    int col = 0;	
	    ArrayNDIter powiter( input.info() );
	
	    do
	    {
		int coeff = 1;
		for ( int idx=0; idx<ndim; idx++ )
		{
		    coeff *= intpow( positer[idx], powiter[idx] );
		}
	    
		poscoeffs.set( row, col, (T)coeff );
		col++;
	    } while ( powiter.next() );

	    row++;
	} while ( positer.next() );	

	solver = new LinSolver<T>( poscoeffs );

	if ( !solver->ready() ) return false;
    }

    solver->apply( input.getData(), coeffs.getData() );

    return true;
}

#endif
