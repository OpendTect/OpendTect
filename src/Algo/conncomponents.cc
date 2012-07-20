/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * fault dip calculation based the pca analysis to the fault attributes
 * AUTHOR   : Bo Zhang/Yuancheng Liu
 * DATE     : July 2012
-*/

static const char* rcsID mUnusedVar = "$Id: conncomponents.cc,v 1.1 2012-07-20 20:05:01 cvsyuancheng Exp $";

#include "conncomponents.h"

#include "arrayndimpl.h"
#include "executor.h"
#include "sorting.h"
//#include "stmatrix.h"
#include "task.h"



ConnComponents::ConnComponents( const Array2D<bool>& input ) 
    : input_(input)
{}


void ConnComponents::compute( TaskRunner* tr )
{
    const int r = input_.info().getSize(0);
    const int c = input_.info().getSize(1);

    mDeclareAndTryAlloc( Array2DImpl<int>*, mark, Array2DImpl<int>(r,c) );
    if ( !mark ) return;
    
    mark->setAll(0);
    classifyMarks( *mark );

    const int sz = input_.info().getTotalSz();
    int* markers = mark->getData();
    components_.erase();
    TypeSet<int> labels;
    
    for ( int idx=0; idx<sz; idx++ )
    {
	const int curidx = labels.indexOf(markers[idx]);
	if ( curidx==-1 )
	{
	    labels += markers[idx];
	     
	    TypeSet<int> ids;
	    ids += idx;
	    components_ += ids;
	}
	else
	{
	    components_[curidx] += idx;
	}
    }
    
    const int nrcomps = labels.size(); 
    TypeSet<int> nrnodes;
    sortedindex_.erase();
    for ( int idx=0; idx<nrcomps; idx++ )
    {
	nrnodes += components_[idx].size();
	sortedindex_ += idx;
    }

    sort_coupled( nrnodes.arr(), sortedindex_.arr(), nrcomps );
}


void ConnComponents::classifyMarks( Array2D<int>& mark )
{
    const int r = mark.info().getSize(0);
    const int c = mark.info().getSize(1);

    int index = 0;
    for ( int i=1; i<r-2; i++ )
    {
	for ( int j=1; j<c-2; j++ )
	{
	    if ( !input_.get(i,j) )
		continue;

	    if ( input_.get(i-1,j+1) )
	    {
		const int m = mark.get(i-1,j+1);
		if ( m>0 )
		{
		    mark.set(i,j,m);
		    if ( mark.get(i,j-1)>0 && 
			 mark.get(i,j-1)!=mark.get(i-1,j+1) )
		    {
			if ( mark.get(i-1,j+1)<mark.get(i,j-1) )
			{
			    setMark( mark, mark.get(i,j-1),
					    mark.get(i-1,j+1) ); 
			}
			else
			{
			    setMark( mark, mark.get(i-1,j+1),
					    mark.get(i,j-1) ); 
			}
		    }

		    if ( mark.get(i-1,j-1)>0 &&
			 mark.get(i-1,j-1)!=mark.get(i-1,j+1) )
		    {
			if ( mark.get(i-1,j+1)<mark.get(i-1,j-1) )
			{
			    setMark( mark, mark.get(i-1,j-1),
					    mark.get(i-1,j+1) ); 
			}
			else
			{
			    setMark( mark, mark.get(i-1,j+1),
					    mark.get(i-1,j-1) ); 
			}
		    }
		}
		else
		{
		    index++;
		    mark.set(i,j,index);
		}
	    }
	    else if ( input_.get(i-1,j) )
	    {
		if ( mark.get(i-1,j)>0 )
		    mark.set(i,j,mark.get(i-1,j));
		else
		{
		    index++;
		    mark.set(i,j,index);
		}
	    }
	    else if ( input_.get(i-1,j-1) )
	    {
		if ( mark.get(i-1,j-1)>0 )
		    mark.set(i,j,mark.get(i-1,j-1));
		else
		{
		    index++;
		    mark.set(i,j,index);
		}
	    }
	    else if ( input_.get(i,j-1) )
	    {
		if ( mark.get(i,j-1)>0 )
		    mark.set(i,j,mark.get(i,j-1));
		else
		{
		    index++;
		    mark.set(i,j,index);
		}
	    }
	    else
	    {
		index++;
		mark.set(i,j,index);
	    }
	}
    }
}


void ConnComponents::setMark( Array2D<int>& mark, int source, int newval )
{
    const int sz = mark.info().getTotalSz();
    int* vals = mark.getData();
    for ( int idx=0; idx<sz; idx++ )
    {
	if ( vals[idx]==source )
	    vals[idx] = newval;
    }
}


int ConnComponents::nrComponents() const
{ return components_.size(); }


const TypeSet<int>* ConnComponents::getComponent( int cidx )
{ 
    return  cidx<0 || cidx>=nrComponents() ? 0 : 
	&components_[sortedindex_[cidx]]; 
}


bool ConnComponents::quadraticFit( int cidx, TypeSet<int>& res )
{
    /*
    const TypeSet<int>* comp = getComponent( cidx );
    if ( !comp )
	return false;

    if ( !hasTrifurcation(*comp) )
    {
	res = *comp;
	return true;
    }

    const int nrnodes = comp->size();
    Matrix a, at, m, inverse;
    a.rows = a.cols = 3; a.matrix = (double*)malloc(sizeof(double)*9);
    at.rows = at.cols = 3; at.matrix = (double*)malloc(sizeof(double)*9);
    m.rows = m.cols = 3; m.matrix = (double*)malloc(sizeof(double)*9);
    inverse.rows = inverse.cols = 3; 
    inverse.matrix = (double*)malloc(sizeof(double)*9);

    const int csz = input_.info().getSize(1);
    float d[3] = {0,0,0};
    float a11=nrnodes, a12=0, a22=0, a23=0, a33=0;
    TypeSet<int> rs;
    for ( int idx=0; idx<nrnodes; idx++ )
    {
	int r = (*comp)[idx]/csz;
	int c = (*comp)[idx]%csz;
	int r2 = r*r;
	rs += r;

	a12 += r;
	a22 += r2;
	a23 += r*r2;
	a33 += r2*r2;
	d[0] += c;
	d[1] += r*c;
	d[2] += r2*c;
    }

    a.matrix[0] = a11;
    a.matrix[1] = a12;
    a.matrix[2] = a22;
    a.matrix[3] = a12;
    a.matrix[4] = a22;
    a.matrix[5] = a23;
    a.matrix[6] = a22;
    a.matrix[7] = a23;
    a.matrix[8] = a33;
    matrix_transpose( &at, a );
    matrix_multiply( &m, at, a );
    matrix_inverse( &inverse, m );
    matrix_multiply( &m, inverse, at );
    float c0 = m.matrix[0]*d[0]+m.matrix[1]*d[1]+m.matrix[2]*d[2];
    float c1 = m.matrix[3]*d[0]+m.matrix[4]*d[1]+m.matrix[5]*d[2];
    float c2 = m.matrix[6]*d[0]+m.matrix[7]*d[1]+m.matrix[8]*d[2];

    res.erase();
    for ( int idx=0; idx<nrnodes; idx++ )
    {
	int c = (int)(c0+c1*rs[idx]+c2*rs[idx]*rs[idx]);
	if ( c<0 || c>=csz )
	    continue;

	res += rs[idx]*csz+c;
    }

    free( a.matrix );
    free( at.matrix );
    free( m.matrix );
    free( inverse.matrix );*/
    return true;
}


bool ConnComponents::hasTrifurcation( const TypeSet<int>& component )
{
    const int sz = component.size();
    const int csz = input_.info().getSize(1);

    for ( int idx=0; idx<sz; idx++ )
    {
	int nrdir = 0;
	for ( int i=-1; i<2; i++ )
	{
	    const int gidx = i*csz+component[idx];
	    for ( int j=-1; j<2; j++ )
	    {
		if ( !i && !j )
		    continue;

		if ( component.indexOf(gidx+j)!=-1 )
		{
		    nrdir++;
		    if ( nrdir>2 )
			return true;
		}
	    }
	}
    }

    return false;
}



float ConnComponents::overLapRate( int componentidx )
{
    return 0;
}

