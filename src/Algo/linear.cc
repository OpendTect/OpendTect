/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Oct 2003
-*/



#include "linear.h"

#include "trigonometry.h"

#define mArrVal(arr) ( arr[idx * offs / sizeof(float)] )

static void calcLS( LinStats2D& ls, const float* xvals, const float* yvals,
		    int nrpts, int offs )
{
    double sumx = 0, sumy = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	sumx += mArrVal(xvals);
	sumy += mArrVal(yvals);
    }

    ls.lp.a0_ = (float)sumx; ls.lp.ax_ = 0;
    Values::setUdf(ls.sd.a0_); Values::setUdf(ls.sd.ax_);
    ls.corrcoeff = 0;
    if ( nrpts < 2 )
	return;

    float avgx = (float) ( sumx / nrpts );
    float avgy = (float) ( sumy / nrpts );
    double sumxy = 0, sumx2 = 0, sumy2 = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	float xv = mArrVal(xvals) - avgx;
	float yv = mArrVal(yvals) - avgy;
	sumx2 += xv * xv;
	sumy2 += yv * yv;
	sumxy += xv * yv;
    }

    if ( sumx2 < 1e-30 )
    {
	// No x range
	ls.lp.a0_ = avgx;
	Values::setUdf(ls.lp.ax_);
	ls.sd.ax_ = 0; ls.corrcoeff = 1;
	return;
    }
    else if ( sumy2 < 1e-30 )
    {
	// No y range
	ls.lp.a0_ = avgy;
	ls.lp.ax_ = 0;
	ls.sd.a0_ = ls.sd.ax_ = 0;
	ls.corrcoeff = 1;
	return;
    }

    ls.lp.ax_ = (float) ( sumxy / sumx2 );
    ls.lp.a0_ = (float) ( (sumy - sumx*ls.lp.ax_) / nrpts );
    ls.corrcoeff = (float) (sumxy / (Math::Sqrt(sumx2) * Math::Sqrt(sumy2)));

    double sumd2 = 0;
    for ( int idx=0; idx<nrpts; idx++ )
    {
	const float ypred = ls.lp.a0_ + ls.lp.ax_ * mArrVal(xvals);
	const float yd = mArrVal(yvals) - ypred;
	sumd2 = yd * yd;
    }
    if ( nrpts < 3 )
	ls.sd.ax_ = ls.sd.a0_ = 0;
    else
    {
	ls.sd.ax_ = (float) ( Math::Sqrt( sumd2 / ((nrpts-2) * sumx2) ) );
	ls.sd.a0_ = (float) ( Math::Sqrt( (sumx2 * sumd2) /
					 (nrpts * (nrpts-2) * sumx2) ) );
    }
}


void LinStats2D::use( const float* xvals, const float* yvals, int nrpts )
{
    calcLS( *this, xvals, yvals, nrpts, sizeof(float) );
}


void LinStats2D::use( const Geom::Point2D<float>* vals, int nrpts )
{
    if ( nrpts < 1 ) return;
    calcLS( *this, &vals[0].x_, &vals[0].y_, nrpts,
	    sizeof(Geom::Point2D<float>) );
}


/*3D best fit plane */
Plane3DFit::Plane3DFit()
    : centroid_(0.0,0.0,0.0)
{}


bool Plane3DFit::compute( const TypeSet<Coord3>& pts, Plane3& plane )
{
    points_.erase();
    centroid_ = Coord3(0.0,0.0,0.0);
    int count = 0;
    for( int idx = 0; idx<pts.size(); idx++ )
    {
	if ( pts[idx].isUdf() )
	    continue;

	points_ += pts[idx];
	centroid_ += pts[idx];
	count++;
    }

    if ( count<1 )
	return false;
    else if ( count==1 )
    {
	plane.set( Coord3(0,0,1), centroid_, false );
	return true;
    }

    if ( count>0 )
	centroid_ /= count;

    double scattermatrix[3][3];
    int  order[3];
    double diagonalmatrix[3];
    double offdiagonalmatrix[3];
    setScatterMatrix( scattermatrix, order );
    tred2(scattermatrix,diagonalmatrix,offdiagonalmatrix);
    tqli(diagonalmatrix,offdiagonalmatrix,scattermatrix);

    /*Find the smallest eigenvalue first. */
    double min = diagonalmatrix[0];
    double max = diagonalmatrix[0];
    int minindex = 0;
    int middleindex = 0;
    int maxindex = 0;

    for( int idx=1; idx<3; idx++)
    {
	if( diagonalmatrix[idx] < min )
	{
	    min = diagonalmatrix[idx];
	    minindex = idx;
	}

	if( diagonalmatrix[idx] > max )
	{
	    max = diagonalmatrix[idx];
	    maxindex = idx;
	}
    }

    for( int idx=0; idx<3; idx++ )
    {
	if( minindex != idx && maxindex != idx )
	    middleindex = idx;
    }

    /* The normal of the plane is the smallest eigenvector.*/
    Coord3 normal;
    for( int idx=0; idx<3; idx++ )
    {
	if ( order[idx]==0 )
	    normal.x_ = scattermatrix[idx][minindex];
	else if ( order[idx]==1 )
	    normal.y_ = scattermatrix[idx][minindex];
	else
	    normal.z_ = scattermatrix[idx][minindex];

	if ( mIsUdf(scattermatrix[idx][minindex]) ||
	     mIsUdf(scattermatrix[idx][middleindex]) ||
	     mIsUdf(scattermatrix[idx][maxindex]) )
	{
	    normal = Coord3(1,0,0);
	    break;
	}
    }

    plane.set( normal, centroid_, false );
    return true;
}


void Plane3DFit::setScatterMatrix( double scattermatrix[3][3],  int order[3] )
{
    for ( int idx=0; idx<3; idx++ )
	scattermatrix[idx][0]=scattermatrix[idx][1]=scattermatrix[idx][2]=0.0;

    for( int idx=0; idx<points_.size(); idx++ )
    {
	const Coord3 d = points_[idx] - centroid_;
	scattermatrix[0][0] += d.x_*d.x_;
	scattermatrix[0][1] += d.x_*d.y_;
	scattermatrix[0][2] += d.x_*d.z_;
	scattermatrix[1][1] += d.y_*d.y_;
	scattermatrix[1][2] += d.y_*d.z_;
	scattermatrix[2][2] += d.z_*d.z_;
    }

    scattermatrix[1][0]=scattermatrix[0][1];
    scattermatrix[2][0]=scattermatrix[0][2];
    scattermatrix[2][1]=scattermatrix[1][2];

    order[0]=0;        /* Beginning order is x-y-z, as found above */
    order[1]=1;
    order[2]=2;
    int tempi;
    double tempd;
    if ( scattermatrix[0][0] > scattermatrix[1][1] )
    {
	tempd=scattermatrix[0][0];
	scattermatrix[0][0]=scattermatrix[1][1];
	scattermatrix[1][1]=tempd;
	tempd=scattermatrix[0][2];
	scattermatrix[0][2]=scattermatrix[2][0]=scattermatrix[1][2];
	scattermatrix[1][2]=scattermatrix[2][1]=tempd;
	tempi=order[0];
	order[0]=order[1];
	order[1]=tempi;
    }

    if ( scattermatrix[1][1] > scattermatrix[2][2] )
    {
	tempd=scattermatrix[1][1];
	scattermatrix[1][1]=scattermatrix[2][2];
	scattermatrix[2][2]=tempd;
	tempd=scattermatrix[0][1];
	scattermatrix[0][1]=scattermatrix[1][0]=scattermatrix[0][2];
	scattermatrix[0][2]=scattermatrix[2][0]=tempd;
	tempi=order[1];
	order[1]=order[2];
	order[2]=tempi;
    }

    if ( scattermatrix[0][0] > scattermatrix[1][1] )
    {
	tempd=scattermatrix[0][0];
	scattermatrix[0][0]=scattermatrix[1][1];
	scattermatrix[1][1]=tempd;
	tempd=scattermatrix[0][2];
	scattermatrix[0][2]=scattermatrix[2][0]=scattermatrix[1][2];
	scattermatrix[1][2]=scattermatrix[2][1]=tempd;
	tempi=order[0];
	order[0]=order[1];
	order[1]=tempi;
    }
}

#define    SIGN(a,b)    ((b)<0? -fabs(a):fabs(a))

void Plane3DFit::tred2( double a[3][3], double d[3], double e[3] )
{
    for( int i=3; i>=2; i-- )
    {
	const int l=i-1;
	double h=0.0;
	double scale=0.0;

	if( l>1 )
	{
	    for( int k=1; k<=l; k++ )
		scale += fabs(a[i-1][k-1]);

	    if( scale==0.0 )        /* skip transformation */
		e[i-1] = a[i-1][l-1];
	    else
	    {
		for( int k=1; k<=l; k++ )
		{
		    a[i-1][k-1] /= scale; /*use scaled a's for transformation.*/
		    h += a[i-1][k-1]*a[i-1][k-1];   /*form sigma in h.*/
		}

		double f = a[i-1][l-1];
		double g = f>0? -Math::Sqrt(h) : Math::Sqrt(h);
		e[i-1] = scale*g;
		h -= f*g;    /* now h is equation (11.2.4) */
		a[i-1][l-1] = f-g;    /* store u in the ith row of a. */
		f=0.0;
		for( int j=1; j<=l; j++ )
		{
		    /*store u/H in ith column of a.*/
		    a[j-1][i-1] = a[i-1][j-1]/h;
		    g=0.0;    /* form an element of A.u in g */
		    for( int k=1; k<=j; k++ )
			g += a[j-1][k-1]*a[i-1][k-1];

		    for( int k=j+1;k<=l;k++ )
			g += a[k-1][j-1]*a[i-1][k-1];

		    e[j-1]=g/h;
		    /* form element of p in temorarliy unused element of e. */

		    f+=e[j-1]*a[i-1][j-1];
		}

		double hh = f/(h+h); /* form K, equation (11.2.11) */

		/* form q and store in e overwriting p. */
		for( int j=1; j<=l; j++ )
		{
		    f = a[i-1][j-1]; /* Note that e[l]=e[i-1] survives */
		    e[j-1] = g = e[j-1]-hh*f;

		    for( int k=1; k<=j; k++ )/* reduce a, equation (11.2.13) */
			a[j-1][k-1] -= (f*e[k-1]+g*a[i-1][k-1]);
		}
	    }
	}
	else
	    e[i-1] = a[i-1][l-1];

	d[i-1] = h;
    }

    /*For computing eigenvector.*/
    d[0] = 0.0;
    e[0] = 0.0;

    for( int i=1; i<=3; i++ )/* begin accumualting of transfomation matrices */
    {
	const int l=i-1;
	if( d[i-1] ) /* this block skipped when i=1 */
	{
	    for( int j=1; j<=l; j++ )
	    {
		double g=0.0;

		/* use u and u/H stored in a to form P.Q */
		for( int k=1; k<=l; k++ )
		    g += a[i-1][k-1]*a[k-1][j-1];

		for( int k=1; k<=l; k++ )
		    a[k-1][j-1] -= g*a[k-1][i-1];
	    }
	}
	d[i-1] = a[i-1][i-1];
	a[i-1][i-1] = 1.0;
	/* reset row and column of a to identity matrix for next iteration */

	for( int j=1; j<=l; j++ )
	    a[j-1][i-1] = a[i-1][j-1] = 0.0;
    }
}


void Plane3DFit::tqli( double d[3], double e[3], double z[3][3] )
{
    for( int i=2; i<=3; i++ )
	e[i-2]=e[i-1];    /* convenient to renumber the elements of e */
    e[2]=0.0;

    for( int l=1; l<=3; l++ )
    {
	int m;
	do
	{
	    for( m=l; m<=2; m++ )
	    {
		/*Look for a single small subdiagonal element
		  to split the matrix. */
		double dd = fabs(d[m-1])+fabs(d[m]);
		if( fabs(e[m-1])+dd == dd )
		    break;
	    }

	    if( m!=l ) //Compain if iteration>30?
	    {
		double g = (d[l]-d[l-1])/(2.0*e[l-1]); /* form shift */
		double r = Math::Sqrt((g*g)+1.0);
		g = d[m-1]-d[l-1]+e[l-1]/(g+SIGN(r,g)); /* this is dm-ks */
		double s = 1.0;
		double c = 1.0;
		double p = 0.0;
		for( int i=m-1; i>=l; i-- )
		{
		    /*A plane rotation as in the original QL, followed by
		      Givens rotations to restore tridiagonal form.*/
		    double f = s*e[i-1];
		    double b = c*e[i-1];
		    if( fabs(f) >= fabs(g) )
		    {
			c = g/f;
			r = Math::Sqrt((c*c)+1.0);
			e[i] = f*r;
			c *= (s=1.0/r);
		    }
		    else
		    {
			s = f/g;
			r = Math::Sqrt((s*s)+1.0);
			e[i] = g*r;
			s *= (c=1.0/r);
		    }
		    g = d[i]-p;
		    r = (d[i-1]-g)*s+2.0*c*b;
		    p = s*r;
		    d[i] = g+p;
		    g = c*r-b;
		    for( int k=1; k<=3; k++ )
		    {
			/*Form eigenvectors*/
			f = z[k-1][i];
			z[k-1][i] = s*z[k-1][i-1]+c*f;
			z[k-1][i-1] = c*z[k-1][i-1]-s*f;
		    }
		}
		d[l-1] = d[l-1]-p;
		e[l-1] = g;
		e[m-1] = 0.0;
	    }
	} while( m != l );
    }
}


