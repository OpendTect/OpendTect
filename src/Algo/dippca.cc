/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * fault dip calculation based the pca analysis to the fault attributes
 * AUTHOR   : Bo Zhang/Y.Liu
 * DATE     : June 2012
-*/


#include "dippca.h"

#include "arrayndimpl.h"
#include "executor.h"
#include "math.h"
#include "pca.h"
#include "task.h"


class Dip3DCalculator : public ParallelTask
{ mODTextTranslationClass(Dip3DCalculator);
public:
Dip3DCalculator( Dip3D& fd )
    : fd_(fd)
{}

protected:
od_int64 nrIterations() const	{ return fd_.input_.info().getTotalSz(); }
uiString uiMessage() const	{ return tr("Dip/Azimuth calculating.."); }

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const float threshold = fd_.setup_.threshold_;
    const bool isabove = fd_.setup_.isabove_;
    const int xgate = fd_.setup_.boxlength_;
    const int ygate = fd_.setup_.boxwidth_;
    const int zgate = fd_.setup_.boxheight_;

    for ( int idx=mCast(int, start); idx<=stop && shouldContinue();
						idx++,addToNrDone(1) )
    {
	int pos[3];
	if ( !fd_.input_.info().getArrayPos( idx, pos ) )
	    continue;

	const float curval = fd_.input_.get( pos[0], pos[1], pos[2] );
	if ( mIsUdf(curval) || (isabove && curval<threshold) ||
	     (!isabove && curval>threshold) )
	    continue;

	const int x_left = mMAX(0,pos[0]-xgate);
	const int x_right = mMIN(fd_.xsz_-1,pos[0]+xgate);
	const int y_left = mMAX(0,pos[1]-ygate);
	const int y_right = mMIN(fd_.ysz_-1,pos[1]+ygate);
	const int z_left = mMAX(0,pos[2]-zgate);
	const int z_right = mMIN(fd_.zsz_-1,pos[2]+zgate);

	float x_mean = 0.0;
	float y_mean = 0.0;
	float z_mean = 0.0;
	TypeSet<int> xs, ys, zs;
	for ( int kx=x_left; kx<=x_right; kx++ )
	{
	    for ( int ky=y_left; ky<=y_right; ky++ )
	    {
		for ( int kz=z_left; kz<=z_right; kz++ )
		{
		    if ( (isabove && fd_.input_.get(kx,ky,kz)>=threshold) ||
			 (!isabove && fd_.input_.get(kx,ky,kz)<=threshold) )
		    {
			xs += kx;
			ys += ky;
			zs += kz;

			x_mean += kx;
			y_mean += ky;
			z_mean += kz;
		    }
		}
	    }
	}
	const int npoints = xs.size();
	if ( npoints<2 )
	    continue;

	x_mean /= (float)npoints;
	y_mean /= (float)npoints;
	z_mean /= (float)npoints;

	/*get the covariance for x and y*/
	float var_xx = 0.0;
	float var_yy = 0.0;
	float var_zz = 0.0;
	float var_xy = 0.0;
	float var_yz = 0.0;
	float var_zx = 0.0;
	for ( int jp=0; jp<npoints; jp++ )
	{
	    var_xx += (xs[jp]-x_mean)*(xs[jp]-x_mean);
	    var_yy += (ys[jp]-y_mean)*(ys[jp]-y_mean);
	    var_zz += (zs[jp]-z_mean)*(zs[jp]-z_mean);
	    var_xy += (xs[jp]-x_mean)*(ys[jp]-y_mean);
	    var_yz += (ys[jp]-y_mean)*(zs[jp]-z_mean);
	    var_zx += (zs[jp]-z_mean)*(xs[jp]-x_mean);
	}

	var_xx /= (npoints-1);
	var_yy /= (npoints-1);
	var_xy /= (npoints-1);
	var_yz /= (npoints-1);
	var_zx /= (npoints-1);

	/* get the eigen value and eigen vector */
	const float d0[] = { var_xx, var_xy, var_zx };
	const float d1[] = { var_xy, var_yy, var_yz };
	const float d2[] = { var_zx, var_yz, var_zz };
	PCA pca(3);
	pca.addSample(d0);
	pca.addSample(d1);
	pca.addSample(d2);
	pca.calculate();

	TypeSet<float> eigenval;
	for ( int idy=0; idy<3; idy++ )
	    eigenval += pca.getEigenValue(idy);
	TypeSet<float> eigenvec0(3,0);
	pca.getEigenVector( 0, eigenvec0 );

	const float eratio = eigenval[0]/(eigenval[1]+eigenval[2]);
	if ( eratio<0.5 )
	    continue;

	const float edenominator = Math::Sqrt(eigenvec0[0]*eigenvec0[0]+
		eigenvec0[1]*eigenvec0[1]);
	const bool ishorizontal = mIsZero(eigenvec0[2],1e-8);
	const float absdip = ishorizontal ? 0.0f :
	    (float)(atan(edenominator/eigenvec0[2])*mRad2DegF);
	const float inldip = ishorizontal ? 0.0f :
	    eigenvec0[0]*fd_.xdist_/(fd_.zdist_*eigenvec0[2]);
	const float crldip = ishorizontal ? 0.0f :
	    eigenvec0[1]*fd_.ydist_/(fd_.zdist_*eigenvec0[2]);
	float azimuth = mUdf(float);
	if ( mIsZero(eigenvec0[0],1e-8) )
	    azimuth = eigenvec0[1]<0 ? 270.0f : 90.0f;
	else
	{
	    azimuth = (float)(atan(eigenvec0[1]/eigenvec0[0])*mRad2DegF);
	    if ( eigenvec0[0]>0 )
	    {
		if ( eigenvec0[1]<0 )
		    azimuth += 360.0f;
	    }
	    else
	    {
		azimuth += 180.0f;
	    }
	}

	fd_.absdip_->set( pos[0], pos[1], pos[2], absdip );
	fd_.inldip_->set( pos[0], pos[1], pos[2], inldip );
	fd_.crldip_->set( pos[0], pos[1], pos[2], crldip );
	fd_.azimuth_->set( pos[0], pos[1], pos[2], azimuth );
    }

    return true;
}

Dip3D& fd_;
};


class Dip2DCalculator : public ParallelTask
{ mODTextTranslationClass(Dip2DCalculator);
public:
Dip2DCalculator( Dip2D& fd )
    : fd_(fd)
{}

protected:
od_int64 nrIterations() const	{ return fd_.input_.info().getTotalSz(); }
uiString uiMessage() const	{ return tr("Dip calculating.."); }

bool doWork( od_int64 start, od_int64 stop, int threadid )
{
    const float threshold = fd_.setup_.threshold_;
    const bool isabove = fd_.setup_.isabove_;
    const int xgate = fd_.setup_.boxlength_;
    const int ygate = fd_.setup_.boxwidth_;

    for ( int idx=mCast(int,start); idx<=stop && shouldContinue();
							idx++,addToNrDone(1) )
    {
	int pos[2];
	if ( !fd_.input_.info().getArrayPos( idx, pos ) )
	    continue;

	if ( (isabove && fd_.input_.get(pos[0],pos[1])<threshold) ||
	     (!isabove && fd_.input_.get(pos[0],pos[1])>threshold) )
	    continue;

	const int x_left = mMAX(0,pos[0]-xgate);
	const int x_right = mMIN(fd_.xsz_-1,pos[0]+xgate);
	const int y_left = mMAX(0,pos[1]-ygate);
	const int y_right = mMIN(fd_.ysz_-1,pos[1]+ygate);

	float x_mean = 0.0;
	float y_mean = 0.0;
	TypeSet<int> xs, ys;
	for ( int kx=x_left; kx<=x_right; kx++ )
	{
	    for ( int ky=y_left; ky<=y_right; ky++ )
	    {
		if ( (isabove && fd_.input_.get(kx,ky)>=threshold) ||
		     (!isabove && fd_.input_.get(kx,ky)<=threshold) )
		{
		    xs += kx;
		    ys += ky;

		    x_mean += kx;
		    y_mean += ky;
		}
	    }
	}

	const int npoints = xs.size();
	if ( npoints<2 )
	    continue;

	x_mean /= npoints;
	y_mean /= npoints;

	/*get the covariance for x and y*/
	float var_xx = 0.0;
	float var_yy = 0.0;
	float var_xy = 0.0;
	for ( int jp=0; jp<npoints; jp++ )
	{
	    var_xx += (xs[jp]-x_mean)*(xs[jp]-x_mean);
	    var_yy += (ys[jp]-y_mean)*(ys[jp]-y_mean);
	    var_xy += (xs[jp]-x_mean)*(ys[jp]-y_mean);
	}

	var_xx /= (npoints-1);
	var_yy /= (npoints-1);
	var_xy /= (npoints-1);

	const float d0[] = { var_xx, var_xy };
	const float d1[] = { var_xy, var_yy };
	PCA pca(2);
	pca.addSample(d0);
	pca.addSample(d1);
	pca.calculate();

	TypeSet<float> eigenval;
	for ( int idy=0; idy<2; idy++ )
	    eigenval += pca.getEigenValue(idy);
	TypeSet<float> eigenvec0(2,0);
	pca.getEigenVector( 0, eigenvec0 );

	const float eratio = eigenval[0]/eigenval[1];
	if ( eratio<0.5 )
	    continue;

	float dip =
	    fd_.xdist_*eigenvec0[0]/(fd_.ydist_*eigenvec0[1]);
	//dip = atan(eigenvec1[1]/eigenvec1[0])*mRad2Angle;
	fd_.dip_->set( pos[0], pos[1], dip );
    }

    return true;
}

Dip2D& fd_;
};




DipPCA::Setup::Setup()
{
    threshold_ = 0;
    isabove_ = true;
    boxlength_ = 1;
    boxwidth_ = 1;
    boxheight_ = 1;
    thetarg_ = StepInterval<int>(30,85,5);
    alpharg_ = StepInterval<int>(5,85,5);
}


DipPCA::Setup& DipPCA::Setup::operator=(const DipPCA::Setup& sp )
{
    threshold_ = sp.threshold_;
    isabove_ = sp.isabove_;
    boxlength_ = sp.boxlength_;
    boxwidth_ = sp.boxwidth_;
    boxheight_ = sp.boxheight_;
    thetarg_ = sp.thetarg_;
    alpharg_ = sp.alpharg_;

    return *this;
}



Dip2D::Dip2D( const Array2D<float>& input, float xdist, float ydist )
    : input_(input)
    , xsz_(input.info().getSize(0))
    , ysz_(input.info().getSize(1))
    , xdist_(xdist)
    , ydist_(ydist)
{
    dip_ = new Array2DImpl<float>( xsz_, ysz_ );
    if ( !dip_ ) return;
    dip_->setAll( 0 );

    dilation_ = new Array2DImpl<float>( xsz_, ysz_ );
    if ( !dilation_ )
    {
	delete dip_; dip_=0;
	return;
    }
    dilation_->setAll( 0 );

    dthinner_ = new Array2DImpl<float>( xsz_, ysz_ );
    if ( !dthinner_ )
    {
	delete dip_; dip_=0;
	delete dilation_; dilation_ = 0;
	return;
    }
    dthinner_->setAll( 0 );

    setSetup( setup_ );
}


Dip2D::~Dip2D()
{
    delete dip_;
    delete dilation_;
    delete dthinner_;
}


const Array2D<float>* Dip2D::get( Output ro ) const
{
    return ro==Dip ? dip_ : (ro==Dilation ? dilation_ : dthinner_);
}


bool Dip2D::compute( TaskRunner* tr )
{
    if ( !dip_ )
	return false;

    Dip2DCalculator fdc( *this );
    if ( !TaskRunner::execute( tr, fdc ) )
	return false;

    return fillGap();
}


bool Dip2D::fillGap()
{
    // generate the angle set
    TypeSet<int> angleset;
    for ( int angle=setup_.thetarg_.start; angle<=setup_.thetarg_.stop;
	    angle += setup_.thetarg_.step )
	angleset += angle;
    for ( int angle=-setup_.thetarg_.stop; angle<=-setup_.thetarg_.start;
	    angle += setup_.thetarg_.step )
	angleset += angle;
    const int nangle = angleset.size();
    if ( !nangle )
	return false;

    const int min_npoixsz_needed = setup_.boxlength_*setup_.boxwidth_;

    // fault_dip_collection: store the fault dip in current window
    ArrayNDInfoImpl arrinfoimpl(4);
    arrinfoimpl.setSize(0, xsz_);
    arrinfoimpl.setSize(1, ysz_);
    arrinfoimpl.setSize(2, nangle);
    const int sz3 = (2*setup_.boxlength_+1)*(2*setup_.boxlength_+1);
    arrinfoimpl.setSize(3, sz3);
    mDeclareAndTryAlloc( PtrMan<ArrayND<float> >, fault_dip_collection,
	    ArrayNDImpl<float>(arrinfoimpl) );
    if ( !fault_dip_collection )
       return false;

    mDeclareAndTryAlloc( PtrMan<Array3D<int> >, accumulator,
	    Array3DImpl<int>(xsz_,ysz_,nangle) );
    if ( !accumulator )
	return false;

    fault_dip_collection->setAll(0);
    accumulator->setAll(0);

    const double fault_dip_threshold = 10;

    for (int jtrace=0; jtrace<ysz_; jtrace++ )
    {
	for ( int jt=xsz_-1; jt>=0; jt-- )
	{
	    if ( fabs(dip_->get(jt,jtrace))>fault_dip_threshold )
		continue;

	    for ( int jangle=0; jangle<nangle; jangle++ )
	    {
		/*find the line function passing current point using curent
		  angle, ax + by + c = 0 */
		float arc = (float)( mDeg2RadF*angleset[jangle] );
		float slope =  tan(arc);
		float a_term = -slope;
		float b_term = 1;
		float c_term = slope*jtrace-jt;

                // check the boundary
		int jt_left = mMAX(0,jt-setup_.boxlength_);
		int jt_right = mMIN(xsz_-1,jt+setup_.boxlength_);
		int jtrace_left = mMAX(0,jtrace-setup_.boxlength_);
		int jtrace_right = mMIN(ysz_-1,jtrace+setup_.boxlength_);

                // collect points in current window
		for ( int ktrace=jtrace_left; ktrace<=jtrace_right; ktrace++ )
		{
		    for ( int kt=jt_left; kt<=jt_right; kt++ )
		    {
			//calculate the distance between point and the line
			float denominator =
			    Math::Sqrt(a_term*a_term+b_term*b_term);
			float numerator = fabs(a_term*ktrace+b_term*kt+c_term);
			float distance = numerator/denominator;

			//collection the non-zero value points
			if ( distance<=setup_.boxwidth_ &&
			     fabs(dip_->get(kt,ktrace))>fault_dip_threshold )
			{
			    accumulator->set( jt, jtrace, jangle,
				    accumulator->get(jt,jtrace,jangle)+1 );
			    int jpoint = accumulator->get(jt,jtrace,jangle);
			    int pos[4] = { jt, jtrace, jangle, jpoint };
			    fault_dip_collection->setND(pos,
				    dip_->get(kt,ktrace));
			}
		    }
		}
	    }
	}
    }

/*
    dot produce to get the energy in each angle zone
    angle_zone_energy:		the energy in current angle zone
    min_npoixsz_needed: minimum point number needed for a valid zone
				identified
*/

    mDeclareAndTryAlloc( PtrMan<Array3D<float> >, angle_zone_energy,
	    Array3DImpl<float>(xsz_,ysz_,nangle) );
    if ( !angle_zone_energy )
	return false;

    angle_zone_energy->setAll(0);

    for ( int jtrace=0; jtrace<ysz_; jtrace++ )
    {
	for ( int jt=xsz_-1; jt>=0; jt-- )
	{
	    if ( fabs(dip_->get(jt,jtrace))>fault_dip_threshold )
		continue;

	    for ( int jangle=0; jangle<nangle; jangle++ )
	    {
		float arc = (float)( angleset[jangle]*mDeg2RadF );
		float v0X = cos(arc);
		float v0Y = sin(arc);
		int npoint = accumulator->get(jt,jtrace,jangle);
		if ( npoint<min_npoixsz_needed )
		    continue;

		for ( int jpoint=0; jpoint<npoint; jpoint++ )
		{
		    int pos[4] = { jt, jtrace, jangle, jpoint };
		    float angle_temp = fault_dip_collection->getND(pos);
		    arc = (float)( angle_temp*mDeg2RadF );
		    float v1X = cos(arc);
		    float v1Y = sin(arc);
		    angle_zone_energy->set(jt,jtrace,jangle,
			   angle_zone_energy->get(jt,jtrace,jangle)+
			   (v0X*v1X+v0Y*v1Y) );
		}
	    }
	}
    }

    /*value current zero degree point*/
    mDynamicCastGet(Array2DImpl<float>*, dilationcp, dilation_);
    dilationcp->copyFrom( *dip_ );

    /*energy threshold
      energy_threshold:         threshold based on angle diffrence
      energy_threshold_percent: the point percent which have the value of
      energy_threshold */
    const float energy_threshold = (float) cos(30*mDeg2RadF);
    const float energy_threshold_percent = 0.7;

    for ( int jtrace=0; jtrace<ysz_; jtrace++ )
    {
	for ( int jt=xsz_-1; jt>=0; jt-- )
	{
	    if ( fabs(dip_->get(jt,jtrace))>fault_dip_threshold )
		continue;

	    /*find the maximu and minimum*/
	    float max_energy_angle mUnusedVar = 0;
	    /*max_energy = angle_zone_energy->jt,jtrace,1);
	      min_energy = angle_zone_energy->jt,jtrace,1);*/

	    int max_energy_index = 1;
	    int min_energy_index mUnusedVar = 1;

	    float max_energy = -999;
	    float min_energy = 999;
	    int angle_flag = 0;

	    for ( int jangle=0; jangle<nangle; jangle++ )
	    {
		int npoint = accumulator->get(jt,jtrace,jangle);
		if ( npoint<min_npoixsz_needed )
		    continue;

		const float cureng = angle_zone_energy->get(jt,jtrace,jangle);
		if ( cureng>=max_energy )
		{
		    max_energy = cureng;
		    max_energy_index = jangle;
		}

		if ( cureng<min_energy )
		{
		    min_energy = cureng;
		    min_energy_index = jangle;
		}

		angle_flag = 1;
	    }

	    const int npoint = accumulator->get(jt,jtrace,max_energy_index);
	    const float energy_thresholdCurrent =
		npoint*energy_threshold*energy_threshold_percent;
	    if  ( max_energy<energy_thresholdCurrent || angle_flag!=1 )
		continue;

	    /* angle_index = fabs(min_energy)>fabs(max_energy) ?
		min_energy_index : max_energy_index;*/
	    dilation_->set(jt,jtrace,mCast(float,angleset[max_energy_index]));
	}
    }

    /*skeletonization*/
    mDeclareAndTryAlloc( PtrMan<Array2D<char> >, invalid_marker,
	    Array2DImpl<char>(xsz_,ysz_) );
    if ( !invalid_marker )
	return false;

    invalid_marker->setAll(0);
    const int min_pixel_width = setup_.boxlength_*2+1;
    for ( int jtrace=0; jtrace<ysz_; jtrace++ )
    {
	for ( int jt=0; jt<xsz_; jt++ )
	{
	    float dilation = dilation_->get(jt,jtrace)-dip_->get(jt,jtrace);
	    if ( fabs(dilation)<fault_dip_threshold )
		continue;

	    int positive_negative_flag = dilation>fault_dip_threshold ? 1 : -1;

	    /*find the left most point location which has the same sign with
	      dip vaule of current point */

	    int positive_negative_flag_local = 0;
	    int jtrace_left = jtrace;
	    for ( int ktrace=jtrace; ktrace>=0; ktrace-- )
	    {
		if ( dilation_->get(jt,ktrace)>fault_dip_threshold )
		    positive_negative_flag_local = 1;
		else if ( dilation_->get(jt,ktrace)<-fault_dip_threshold )
		    positive_negative_flag_local = -1;
		else
		    positive_negative_flag_local = 0;

		jtrace_left = ktrace;
		if ( positive_negative_flag!=positive_negative_flag_local )
		    break;
	    }

	    /*find the right most point location which has the same sign with
	      dip value of current point*/

	    int jtrace_right = jtrace;
	    positive_negative_flag_local = 0;
	    for ( int ktrace=jtrace; ktrace<ysz_; ktrace++ )
	    {
		if ( dilation_->get(jt,ktrace)>fault_dip_threshold )
		    positive_negative_flag_local = 1;
		else if ( dilation_->get(jt,ktrace)<-fault_dip_threshold )
		    positive_negative_flag_local = -1;
		else
		    positive_negative_flag_local = 0;

		jtrace_right = ktrace;
		if ( positive_negative_flag!=positive_negative_flag_local )
		    break;
	    }

	    /*check the pixel width after dilation*/
	    int pixel_width = jtrace_right-jtrace_left;
	    if ( pixel_width<=min_pixel_width )
		continue;

	    int jtrace_right_original = jtrace_right;
	    int jtrace_left_original = jtrace_left;
	    /*get the pixel width before dilation*/
	    for ( int ktrace=jtrace_left; ktrace<=jtrace_right; ktrace++ )
	    {
		jtrace_left_original = ktrace;
		/*if ( dilation>fault_dip_threshold )
			positive_negative_flag_local1 = 1;
		  else if ( dilation<-fault_dip_threshold )
			 positive_negative_flag_local1 = -1;
		    else
			 positive_negative_flag_local1 = 0;

		    if ( positive_negative_flag_local1!=positive_negative_flag )
		    break;*/

		if ( fabs(dip_->get(jt,ktrace))>fault_dip_threshold )
		    break;
	    }

	    for ( int ktrace=jtrace_right; ktrace>=jtrace_left; ktrace-- )
	    {
		jtrace_right_original = ktrace;
		/*if dilation > fault_dip_threshold
			positive_negative_flag_local1 = 1;
		elseif dilation < -fault_dip_threshold
			positive_negative_flag_local1 = -1;
		else
			positive_negative_flag_local1 = 0;

		if abs(positive_negative_flag_local1-positive_negative_flag)>0.1
		    break;*/

		if ( fabs(dip_->get(jt,ktrace))>fault_dip_threshold )
		    break;
	    }

	    /*maker the useless interpolated value*/
	    for (int ktrace=jtrace_left;ktrace<=jtrace_left_original; ktrace++)
		invalid_marker->set(jt,ktrace,1);

	    for (int ktrace=jtrace_right_original;ktrace<=jtrace_right;ktrace++)
		invalid_marker->set(jt,ktrace, 1);
	}
    }

    mDynamicCastGet(Array2DImpl<float>*, dthin, dthinner_);
    dthin->copyFrom( *dilation_ );
    for ( int jtrace=0; jtrace<ysz_; jtrace++ )
    {
	for ( int jt=0; jt<xsz_; jt++ )
	{
	    if ( invalid_marker->get(jt,jtrace)==1 )
		dthinner_->set(jt,jtrace,0);
	}
    }

    return true;
}


Dip3D::Dip3D( const Array3D<float>& input, float xdist, float ydist,
	float zdist )
    : input_(input)
    , xsz_(input.info().getSize(0))
    , ysz_(input.info().getSize(1))
    , zsz_(input.info().getSize(2))
    , xdist_(xdist)
    , ydist_(ydist)
    , zdist_(zdist)
{
    absdip_ = new Array3DImpl<float>( xsz_, ysz_, zsz_ );
    inldip_ = new Array3DImpl<float>( xsz_, ysz_, zsz_ );
    crldip_ = new Array3DImpl<float>( xsz_, ysz_, zsz_ );
    azimuth_ = new Array3DImpl<float>( xsz_, ysz_,zsz_ );

    if ( !inldip_ || !crldip_ || !azimuth_ || !absdip_ )
    {
	delete absdip_; absdip_ = 0;
	delete inldip_; inldip_ = 0;
	delete crldip_; crldip_ = 0;
	delete azimuth_; azimuth_ = 0;
	return;
    }

    inldip_->setAll( mUdf(float) );
    crldip_->setAll( mUdf(float) );
    azimuth_->setAll( mUdf(float) );
    absdip_->setAll( mUdf(float) );

    setSetup( setup_ );
}


Dip3D::~Dip3D()
{
    delete absdip_;
    delete inldip_;
    delete crldip_;
    delete azimuth_;
}


const Array3D<float>* Dip3D::get( Output opt ) const
{
    return opt==AbsDip ? absdip_ :
	(opt==InlDip ? inldip_ : (opt==CrlDip ? crldip_ : azimuth_));
}


bool Dip3D::compute( TaskRunner* tr )
{
    if ( !inldip_ )
	return false;

    Dip3DCalculator fdc( *this );
    return TaskRunner::execute( tr, fdc );
}
