/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Yuancheng Liu
 * DATE     : August 2009
-*/

static const char* rcsID mUnusedVar = "$Id$";

#include "horizon3dtesselator.h"

#include "survinfo.h"
#include "positionlist.h"
#include "simpnumer.h"



Horizon3DTesselator::Horizon3DTesselator( const Coord3List* coords,
	int nrcoordcols, unsigned char spacing,
	int nrrows,int nrcols, 
	TypeSet<int>* pointci, TypeSet<int>* lineci, TypeSet<int>* stripci, 
	TypeSet<int>* pointni, TypeSet<int>* lineni, TypeSet<int>* stripni,
        Coord3List* normals, int nrnormalcols, int normstartidx )
    : coords_( coords )
    , nrrows_( nrrows )
    , nrcols_( nrcols )  
    , normalstart_( normstartidx )			 
    , spacing_( spacing )
    , pointci_( pointci )
    , lineci_( lineci )
    , stripci_( stripci )
    , pointni_( pointni )
    , lineni_( lineni )
    , stripni_( stripni )
    , normals_( normals )			 
    , nrnormalcols_( nrnormalcols )
    , cosanglexinl_( cos(SI().computeAngleXInl()) )
    , sinanglexinl_( sin(SI().computeAngleXInl()) )		     
    , nrcoordcols_( nrcoordcols )
{}

#define mStrip 3
#define mLine 2
#define mPoint 1

#define mAddPosInitialTriangle( ci0, ci1, ci2 ) \
{ \
    isstripterminated =  false; \
    mAddPosIndex( ci0, mStrip ); \
    mAddPosIndex( ci1, mStrip ); \
    mAddPosIndex( ci2, mStrip ); \
}


#define mTerminatePosStrip \
if ( !isstripterminated ) \
{ \
    isstripterminated = true; \
    mAddPosIndex( -1, mStrip ); \
}


#define mAddPosIndex( ci, obj ) \
{ \
    if ( obj==mStrip ) \
    { \
	if ( stripci_ )	(*stripci_) += ci; \
	if ( stripni_ ) (*stripni_) += getNormalIdx(ci); \
    } \
    else if ( obj==mLine ) \
    { \
	if ( lineci_ ) (*lineci_) += ci; \
	if ( lineni_ ) (*lineni_) += getNormalIdx(ci); \
    } \
    else if ( obj==mPoint ) \
    { \
	if ( pointci_ )	(*pointci_) += ci; \
	if ( pointni_ ) (*pointni_) += getNormalIdx(ci); \
    } \
}

#define m00 0
#define m01 1
#define m02 2
#define m10 3
#define m11 4
#define m12 5
#define m20 6
#define m21 7
#define m22 8

int Horizon3DTesselator::nextStep()
{
    if ( !coords_ || !spacing_ )
    	return SequentialTask::Finished();

    const int lastrowidx = (nrrows_-1)/spacing_;
    const int lastcolidx = (nrcols_-1)/spacing_;

    for ( int ridx=0; ridx<=lastrowidx; ridx++ )
    {
	int ci11 = ridx*spacing_*nrcoordcols_;
	int ci21 = ci11 + spacing_*nrcoordcols_;

	bool nbdef[] = { false, false, false,		// 00   01     02
			 false, false, false,		// 10   11(me) 12
			 false, false, false };		// 20   21     22

	nbdef[m11] = coords_->isDefined(ci11);
	nbdef[m21] = ridx!=lastrowidx ? coords_->isDefined(ci21) : false;
	if ( ridx )
	{
	    int ci01 = ci11 - spacing_*nrcoordcols_;
	    nbdef[m01] = coords_->isDefined(ci01);
	}

	bool isstripterminated = true;
	for ( int cidx=0; cidx<=lastcolidx; cidx++ )
	{
	    const int ci12 = ci11 + spacing_;
	    const int ci22 = ci21 + spacing_;
	    
	    nbdef[m12] = cidx!=lastcolidx ? coords_->isDefined(ci12) : false;
	    nbdef[m22] = (cidx==lastcolidx || ridx==lastrowidx) ? 
		false : coords_->isDefined(ci22);
	    
	    int ci02 = ci12 - spacing_*nrcoordcols_;
	    nbdef[m02] = ridx ? coords_->isDefined(ci02) : false;

	    const int defsum = nbdef[m11]+nbdef[m12]+nbdef[m21]+nbdef[m22];
	    if ( defsum<3 ) 
	    {
		mTerminatePosStrip;
		if ( ridx<lastrowidx && cidx<lastcolidx && nbdef[m11] )
		{
		    const bool con12 = nbdef[m12] &&
			(!ridx || (!nbdef[m01] && !nbdef[m02]) );
		    const bool con21 = nbdef[m21] &&
			(!cidx || (!nbdef[m10] && !nbdef[m20]) );

		    if ( con12 || con21 )
		    {
			const int lastci = lineci_ && lineci_->size()
			    ? (*lineci_)[lineci_->size()-1] : -1;

			if ( lastci!=ci11 )
			{
			    if ( lastci!=-1 )
				mAddPosIndex( -1, mLine );

			    mAddPosIndex( ci11, mLine );
			}

			mAddPosIndex( con12 ? ci12 : ci21, mLine );
		    }
		}
		else if ( nbdef[m11] && !nbdef[m10] && !nbdef[m12] && 
			 !nbdef[m01] && !nbdef[m21] )
		{
		    mAddPosIndex( ci11, mPoint ) 
		} 
	    }
	    else if ( defsum==3 )
	    {
		mTerminatePosStrip;
		if ( !nbdef[m11] )
		    mAddPosInitialTriangle( ci12, ci21, ci22 )
		else if ( !nbdef[m21] )
		    mAddPosInitialTriangle( ci11, ci22, ci12 )
		else if ( !nbdef[m12] )
		    mAddPosInitialTriangle( ci11, ci21, ci22 )
		else
		    mAddPosInitialTriangle( ci11, ci21, ci12 )
		mTerminatePosStrip;
	    }
	    else
	    {
		const float diff0 = (float) (coords_->get(ci11).z-
				    coords_->get(ci22).z);
		const float diff1 = (float) (coords_->get(ci12).z-
				    coords_->get(ci21).z);

		const bool do11to22 = fabs(diff0) < fabs(diff1);
		if ( do11to22 )
		{
		    mTerminatePosStrip;
		    mAddPosInitialTriangle( ci21, ci22, ci11 );
		    mAddPosIndex( ci12, mStrip );
		    mTerminatePosStrip;
		}
		else
		{
		    if ( isstripterminated )
		    {
			mAddPosInitialTriangle( ci11, ci21, ci12 );
			mAddPosIndex( ci22, mStrip );
		    }
		    else
		    {
			mAddPosIndex( ci12, mStrip );
			mAddPosIndex( ci22, mStrip );
		    }
		}
	    } 
	
	    nbdef[m00] = nbdef[m01]; nbdef[m01] = nbdef[m02];
	    nbdef[m10] = nbdef[m11]; nbdef[m11] = nbdef[m12];
	    nbdef[m20] = nbdef[m21]; nbdef[m21] = nbdef[m22];
	    ci11 = ci12; ci21 = ci22;
	}

	mTerminatePosStrip;
    }
    	
    return SequentialTask::Finished();;
}


void Horizon3DTesselator::computeNormal( int ni, int row, int col )
{
    if ( !normals_ )
	return;

    TypeSet<float> posarray, zarray;
    for ( int idx=-spacing_; idx<=spacing_; idx++ )
    {
	const int currow = row+idx;
	if ( currow<0 )
	{
	    idx += -currow-1;
	    continue;
	}

	if ( currow>nrrows_ )
	    break;

	const Coord3 pos = coords_->get(currow*nrcoordcols_+col);
	if ( pos.isDefined() )
	{
	    posarray += idx*SI().inlDistance();
	    zarray += (float) pos.z;
	}
    }
	   
    double drow = 0;
    if ( zarray.size()>1 )
	getGradient( posarray.arr(), zarray.arr(), zarray.size(), 0, 0, &drow );

    posarray.erase(); zarray.erase();    
    for ( int idx=-spacing_; idx<=spacing_; idx++ )
    {
	const int curcol = col+idx;
	if ( curcol<0 )
	{
	    idx += -curcol-1;
	    continue;
	}

	if ( curcol>nrcols_ )
	    break;
	
	const Coord3 pos = coords_->get(row*nrcoordcols_+curcol);
	if ( pos.isDefined() )
	{
	    posarray += idx*SI().crlDistance();
	    zarray += (float) pos.z;
	}
    }

    double dcol = 0;
    if ( zarray.size()>1 )
	getGradient( posarray.arr(), zarray.arr(), zarray.size(), 0, 0, &dcol );

   normals_->set( ni, Coord3( drow*cosanglexinl_+dcol*sinanglexinl_,
	    	      dcol*cosanglexinl_-drow*sinanglexinl_, -1 ) );
}


int Horizon3DTesselator::getNormalIdx( int crdidx )
{
    if ( crdidx<0 )
	return -1;

    const int row = crdidx/nrcoordcols_;
    const int col = crdidx%nrcoordcols_;

    if ( !(row%spacing_) && !(col%spacing_) )
    {
	const int resrow = row/spacing_;
	const int rescol = col/spacing_;
	const int ni = resrow*nrnormalcols_ + rescol + normalstart_;
	if ( normals_ && !normals_->isDefined(ni) )
    	    computeNormal( ni, row, col );

	return ni;
    }

    return -1;
}
