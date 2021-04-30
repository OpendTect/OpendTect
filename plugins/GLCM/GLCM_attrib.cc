/*+
 * (C) JOANNEUM RESEARCH; http://www.joanneum.at
 * AUTHOR   : Christoph Eichkitz;
 * http://www.joanneum.at/resources/gph/mitarbeiterinnen/
 mitarbeiter-detailansicht/person/0/3144/eichkitz.html
 * DATE     : November 2013
-*/


#include "GLCM_attrib.h"
#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribparamgroup.h"
#include "statruncalc.h"
#include "survinfo.h"
#include <math.h>
#include "vectoraccess.h"
#include <iostream>


#define mAttributeF1	0
#define mAttributeF2	1
#define mAttributeF3	2
#define mAttributeF4	3
#define mAttributeF5	4
#define mAttributeF6	5
#define mAttributeF7	6
#define mAttributeF8	7
#define mAttributeF9	8
#define mAttributeF10	9
#define mAttributeF11	10
#define mAttributeF12	11
#define mAttributeF13	12
#define mAttributeG1	13
#define mAttributeG2	14
#define mAttributeG3	15
#define mAttributeG4	16
#define mAttributeG5	17
#define mAttributeG6	18
#define mAttributeG7	19
#define mAttributeG8	20
#define mAttributeG9	21
#define mAttributeG10	22

#define mDirection1	0
#define mDirection2	1
#define mDirection3	2
#define mDirection4	3
#define mDirection5	4
#define mDirection6	5
#define mDirection7	6
#define mDirection8	7
#define mDirection9	8
#define mDirection10	9
#define mDirection11	10
#define mDirection12	11
#define mDirection13	12
#define mDirectionIL	13
#define mDirectionXL	14
#define mDirectionTime	15
#define mDirectionAll	16

#define eps = 0.0000001

namespace Attrib
{

mAttrDefCreateInstance( GLCM_attrib )

void GLCM_attrib::initClass()
{
    mAttrStartInitClassWithUpdate

    EnumParam* attribute = new EnumParam( attributeStr() );
    attribute->addEnum( attribTypeStr(mAttributeF1) );
    attribute->addEnum( attribTypeStr(mAttributeF2) );
    attribute->addEnum( attribTypeStr(mAttributeF3) );
    attribute->addEnum( attribTypeStr(mAttributeF4) );
    attribute->addEnum( attribTypeStr(mAttributeF5) );
    attribute->addEnum( attribTypeStr(mAttributeF6) );
    attribute->addEnum( attribTypeStr(mAttributeF7) );
    attribute->addEnum( attribTypeStr(mAttributeF8) );
    attribute->addEnum( attribTypeStr(mAttributeF9) );
    attribute->addEnum( attribTypeStr(mAttributeF10) );
    attribute->addEnum( attribTypeStr(mAttributeF11) );
    attribute->addEnum( attribTypeStr(mAttributeF12) );
    attribute->addEnum( attribTypeStr(mAttributeF13) );
    attribute->addEnum( attribTypeStr(mAttributeG1) );
    attribute->addEnum( attribTypeStr(mAttributeG2) );
    attribute->addEnum( attribTypeStr(mAttributeG3) );
    attribute->addEnum( attribTypeStr(mAttributeG4) );
    attribute->addEnum( attribTypeStr(mAttributeG5) );
    attribute->addEnum( attribTypeStr(mAttributeG6) );
    attribute->addEnum( attribTypeStr(mAttributeG7) );
    attribute->addEnum( attribTypeStr(mAttributeG8) );
    attribute->addEnum( attribTypeStr(mAttributeG9) );
    attribute->addEnum( attribTypeStr(mAttributeG10) );
    attribute->setDefaultValue( mAttributeF1 );
    desc->addParam( attribute );

    EnumParam* direction = new EnumParam( directionStr() );
    direction->addEnum( directTypeStr(mDirection1) );
    direction->addEnum( directTypeStr(mDirection2) );
    direction->addEnum( directTypeStr(mDirection3) );
    direction->addEnum( directTypeStr(mDirection4) );
    direction->addEnum( directTypeStr(mDirection5) );
    direction->addEnum( directTypeStr(mDirection6) );
    direction->addEnum( directTypeStr(mDirection7) );
    direction->addEnum( directTypeStr(mDirection8) );
    direction->addEnum( directTypeStr(mDirection9) );
    direction->addEnum( directTypeStr(mDirection10) );
    direction->addEnum( directTypeStr(mDirection11) );
    direction->addEnum( directTypeStr(mDirection12) );
    direction->addEnum( directTypeStr(mDirection13) );
    direction->addEnum( directTypeStr(mDirectionIL) );
    direction->addEnum( directTypeStr(mDirectionXL) );
    direction->addEnum( directTypeStr(mDirectionTime) );
    direction->addEnum( directTypeStr(mDirectionAll) );
    direction->setDefaultValue( mDirectionAll );
    desc->addParam( direction );

    FloatParam* minlimit = new FloatParam( minlimitStr() );
    minlimit->setLimits( Interval<float>(-100000,mUdf(float)) );
    desc->addParam( minlimit);

    FloatParam* maxlimit = new FloatParam( maxlimitStr() );
    maxlimit->setLimits( Interval<float>(0,mUdf(float)) );
    desc->addParam( maxlimit);

    IntParam* greylevels = new IntParam( numbergreyStr() );
    greylevels->setLimits( 4, 1024, 1);
    greylevels->setDefaultValue(16);
    desc->addParam( greylevels );

    BinIDParam* stepout = new BinIDParam( stepoutStr() );
    stepout->setDefaultValue( BinID(1,1) );
    desc->addParam( stepout );

    IntParam* samprange = new IntParam( sampStr() );
    samprange->setDefaultValue( 5 );
    desc->addParam( samprange );

    BoolParam* steering = new BoolParam( steeringStr() );
    steering->setDefaultValue( false );
    desc->addParam( steering );

    desc->addOutputDataType( Seis::UnknowData );
    desc->addInput( InputSpec("Input data",true) );

    InputSpec steerspec( "Steering data", false );
    steerspec.issteering_ = true;
    desc->addInput( steerspec );

    mAttrEndInitClass
}

const char* GLCM_attrib::attribTypeStr( int attrType )
{
    if ( attrType==mAttributeF1 ) return "Energy f1";
    if ( attrType==mAttributeF2 ) return "Contrast f2";
    if ( attrType==mAttributeF3 ) return "Correlation f3";
    if ( attrType==mAttributeF4 ) return "Variance f4";
    if ( attrType==mAttributeF5 ) return "Inverse Difference Moment f5";
    if ( attrType==mAttributeF6 ) return "Sum Average f6";
    if ( attrType==mAttributeF7 ) return "Sum Variance f7";
    if ( attrType==mAttributeF8 ) return "Sum Entropy f8";
    if ( attrType==mAttributeF9 ) return "Entropy f9";
    if ( attrType==mAttributeF10 ) return "Difference Variance f10";
    if ( attrType==mAttributeF11 ) return "Difference Entropy f11";
    if ( attrType==mAttributeF12 )
	return "Information Measures of Correlation f12";
    if ( attrType==mAttributeF13 )
	return "Information Measures of Correlation f13";
    if ( attrType==mAttributeG1 ) return "Homogeneity g1";
    if ( attrType==mAttributeG2 ) return "Sum Mean g2";
    if ( attrType==mAttributeG3 ) return "Maximum Probability g3";
    if ( attrType==mAttributeG4 ) return "Cluster Tendency g4";
    if ( attrType==mAttributeG5 ) return "Cluster Shade g5";
    if ( attrType==mAttributeG6 ) return "Cluster Prominence g6";
    if ( attrType==mAttributeG7 ) return "Dissimilarity g7";
    if ( attrType==mAttributeG8 ) return "Difference Mean g8";
    if ( attrType==mAttributeG9 ) return "Autocorrelation g9";
    if ( attrType==mAttributeG10 ) return "Inertia g10";

    return 0;
}

const char* GLCM_attrib::directTypeStr( int dirType )
{
    if ( dirType==mDirection1 ) return "Azimuth 0, Dip 0";
    if ( dirType==mDirection2 ) return "Azimuth 0, Dip 45";
    if ( dirType==mDirection3 ) return "Azimuth 0, Dip 90";
    if ( dirType==mDirection4 ) return "Azimuth 0, Dip 135";
    if ( dirType==mDirection5 ) return "Azimuth 45, Dip 0";
    if ( dirType==mDirection6 ) return "Azimuth 45, Dip 45";
    if ( dirType==mDirection7 ) return "Azimuth 45, Dip 135";
    if ( dirType==mDirection8 ) return "Azimuth 90, Dip 0";
    if ( dirType==mDirection9 ) return "Azimuth 90, Dip 45";
    if ( dirType==mDirection10 ) return "Azimuth 90, Dip 135";
    if ( dirType==mDirection11 ) return "Azimuth 135, Dip 0";
    if ( dirType==mDirection12 ) return "Azimuth 135, Dip 45";
    if ( dirType==mDirection13 ) return "Azimuth 135, Dip 135";
    if ( dirType==mDirectionIL ) return "Inline Direction";
    if ( dirType==mDirectionXL ) return "Crossline Direction";
    if ( dirType==mDirectionTime ) return "Timeslice/Depthslice Direction";
    if ( dirType==mDirectionAll ) return "All Directions";

    return 0;
}


void GLCM_attrib::updateDesc( Desc& desc )
{
    desc.setParamEnabled( minlimitStr(), true );
    desc.setParamEnabled( maxlimitStr(), true );
    desc.setParamEnabled( numbergreyStr(), true );

    desc.setParamEnabled( stepoutStr(), true );
    desc.setParamEnabled( sampStr(), true );
    desc.inputSpec(1).enabled_ =
			desc.getValParam( steeringStr())->getBoolValue();
}


GLCM_attrib::GLCM_attrib( Desc& desc )
    : Provider( desc )
    , usegreylevels_(0)
{
    if ( !isOK() ) return;

    mGetInt( samprange_, sampStr() );
    gate_.start = mCast( float, -2*samprange_);
    gate_.stop = mCast( float, 2*samprange_);

    mGetFloat( minlimit_, minlimitStr() );
    mGetFloat( maxlimit_, maxlimitStr() );
    if ( mIsEqual(minlimit_,maxlimit_,1e-3) )
    {
	errmsg_ = tr("Minimum and Maximum values cannot be the same.");
	errmsg_.addMoreInfo(
		tr("Values represent the clipping range of the input."), true );
	return;
    }

    mGetInt( usegreylevels_, numbergreyStr() );
    mGetBinID( stepout_, stepoutStr() );
    mGetEnum( attribute_, attributeStr() );
    mGetEnum( direction_, directionStr() );
    mGetBool( dosteer_, steeringStr() );

    sampgate_.start = -samprange_;
    sampgate_.stop = samprange_;

    int posidx = 0;
    for ( int ildx=-stepout_.inl(); ildx<=stepout_.inl(); ildx++ )
    {
	for ( int cdx=-stepout_.crl(); cdx<=stepout_.crl(); cdx++ )
	{
	    const BinID bid (ildx, cdx );
	    posandsteeridx_.positions_ += bid;
	    posandsteeridx_.posidx_ += posidx;
	    ++posidx;
	    posandsteeridx_.steerindexes_ += getSteeringIndex( bid );
	}
    }

    desgate_ = Interval<float>( gate_.start, gate_.stop );
    inpdata_.allowNull( true );

}


int GLCM_attrib::computeGreyLevel( float trcval) const
{
    trcval = ceil( (trcval-minlimit_) /
    ((Math::Abs(maxlimit_)+Math::Abs(minlimit_))/(usegreylevels_-1)));
    int trcvali = mNINT32(trcval);
    if ( trcvali < 0 )
	trcvali = 0;
    if ( trcvali > (usegreylevels_-1) )
	trcvali = usegreylevels_-1;

    return trcvali;
}


bool GLCM_attrib::getInputOutput( int input, TypeSet<int>& res ) const
{
    if ( !input )
	return Provider::getInputOutput( input, res );

    for ( int idx=0; idx<posandsteeridx_.steerindexes_.size(); idx++ )
	res += posandsteeridx_.steerindexes_[idx];

    return true;
}


bool GLCM_attrib::getInputData( const BinID& relpos, int zintv )
{
    if ( inpdata_.isEmpty() )
	inpdata_ += 0;

    const DataHolder* inpdata = inputs_[0]->getData( relpos, zintv );
    if ( !inpdata ) return false;

    inpdata_.replace( 0, inpdata);
    inpdata_.replace( 0,0);
    const int maxlength = mMAX( stepout_.inl(), stepout_.crl()) * 2 +1;
    while ( inpdata_.size() < maxlength*maxlength )
	inpdata_ += 0;

    const BinID bidstep = inputs_[0]->getStepoutStep();

    for ( int idx=0; idx<posandsteeridx_.posidx_.size(); idx++)
    {
	const BinID inpos = relpos + bidstep*posandsteeridx_.positions_[idx];
	const DataHolder* data = inputs_[0]->getData( inpos );
	inpdata_.replace( posandsteeridx_.posidx_[idx], data );
    }

    dataidx_ = getDataIndex( 0 );
    steerdata_ = inputs_[1] ? inputs_[1]->getData( relpos, zintv ) : 0;

    return true;
}


const BinID* GLCM_attrib::desStepout( int inp, int out ) const
{
    return &stepout_;
}


bool GLCM_attrib::computeData( const DataHolder& output, const BinID& relpos,
			       int z0, int nrsamples, int threadid ) const
{
    Node* LinkedList = new Node;
    Node* newNode = new Node;
    Node* temp = new Node;

    for ( int idx=0; idx<nrsamples; idx++ )
    {
	float outval = 0;

	int elements = 0;
	int greyI = 0;
	int greyJ = 0;

	LinkedList = NULL;
	int findVal = 0;

//------------------------------------------------------------------------------
//			Direction 1 - Azimuth 0 / Dip 0
//------------------------------------------------------------------------------

	if ( direction_==0 || direction_==13 || direction_==15 ||
	     direction_==16 )
	{
	    for ( int isamp=sampgate_.start; isamp<=sampgate_.stop; isamp++ )
	    {
		for ( int posidx=0; posidx<(inpdata_.size()-1); posidx++ )
		{
		    if ( (posidx+1) % ( 2*stepout_.inl()+1 ) != 0 || posidx==0 )
		    {
			float shift = 0;
			if ( steerdata_ )
			{
			    const int steeridx =
					posandsteeridx_.steerindexes_[posidx];
			    shift = getInputValue( *steerdata_, steeridx,
						   idx, z0 );
			}
			const float valI = getInterpolInputValue(
			   *inpdata_[posidx], dataidx_, idx+isamp+shift, z0 );
			const float valJ = getInterpolInputValue(
			   *inpdata_[posidx+1], dataidx_, idx+isamp+shift, z0 );
			greyI = computeGreyLevel( valI );
			greyJ = computeGreyLevel( valJ );
			elements += 2;

			if ( LinkedList == NULL)
			{
			    newNode = new Node;
			    newNode->nodeI = greyI;
			    newNode->nodeJ = greyJ;
			    newNode->numbercoocurrence = 1;
			    newNode->next = NULL;
			    LinkedList = newNode;
			}
			else
			{
			    temp = LinkedList;
			    while ( temp->next != NULL )
			    {
				if ( temp->nodeI == greyI
				  && temp->nodeJ == greyJ )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}
				if ( temp->nodeI == greyJ
				  && temp->nodeJ == greyI )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}

				temp = temp->next;
			    }

			    if ( findVal == 0 )
			    {
				newNode = new Node;
				newNode->nodeI = greyI;
				newNode->nodeJ = greyJ;
				newNode->numbercoocurrence = 1;
				newNode->next = NULL;
				temp->next = newNode;
			    }
			    findVal = 0;
			}
		    }
		}
	    }
	}

//------------------------------------------------------------------------------
//			Direction 02 - Azimuth 0 / Dip 45
//------------------------------------------------------------------------------

	if ( direction_==1 || direction_==13 || direction_==16 )
	{
	    for ( int isamp=sampgate_.start; isamp<=(sampgate_.stop-1); isamp++)
	    {
		for ( int posidx=0; posidx<(inpdata_.size()-1); posidx++ )
		{
		    if ( (posidx+1) % ( 2*stepout_.inl()+1 )!=0 || posidx == 0 )
		    {
			float shift = 0;
			if ( dosteer_ )
			{
			    const int steeridx =
					posandsteeridx_.steerindexes_[posidx+1];
			    shift = getInputValue( *steerdata_, steeridx,
						   idx, z0 );
			}
			const float valI = getInterpolInputValue(
						*inpdata_[posidx], dataidx_,
						idx+isamp+shift, z0 );
			const float valJ = getInterpolInputValue(
						*inpdata_[posidx+1], dataidx_,
						idx+isamp+shift+1, z0 );
			greyI = computeGreyLevel( valI );
			greyJ = computeGreyLevel( valJ );
			elements += 2;

			if ( LinkedList == NULL)
			{
			    newNode = new Node;
			    newNode->nodeI = greyI;
			    newNode->nodeJ = greyJ;
			    newNode->numbercoocurrence = 1;
			    newNode->next = NULL;
			    LinkedList = newNode;
			}
			else
			{
			    temp = LinkedList;
			    while ( temp->next != NULL )
			    {
				if ( temp->nodeI == greyI
				  && temp->nodeJ == greyJ )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}
				if ( temp->nodeI == greyJ
				  && temp->nodeJ == greyI )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}

				temp = temp->next;
			    }

			    if ( findVal == 0 )
			    {
				newNode = new Node;
				newNode->nodeI = greyI;
				newNode->nodeJ = greyJ;
				newNode->numbercoocurrence = 1;
				newNode->next = NULL;
				temp->next = newNode;
			    }
			    findVal = 0;
			}
		    }
		}
	    }
	}

//------------------------------------------------------------------------------
//			Direction 03 - Azimuth 0 / Dip 90
//------------------------------------------------------------------------------

	if ( direction_==2 || direction_==13 || direction_==14
	  || direction_==16 )
	{
	    for ( int isamp=sampgate_.start; isamp<=(sampgate_.stop-1); ++isamp)
	    {
		for ( int posidx=0; posidx<inpdata_.size(); posidx++ )
		{
		    const float valI = getInputValue( *inpdata_[posidx],
						      dataidx_, idx+isamp, z0 );
		    const float valJ = getInputValue( *inpdata_[posidx],
						      dataidx_,
						      idx+isamp+1, z0 );
		    greyI = computeGreyLevel( valI );
		    greyJ = computeGreyLevel( valJ );
		    elements += 2;

		    if ( LinkedList == NULL)
		    {
			newNode = new Node;
			newNode->nodeI = greyI;
			newNode->nodeJ = greyJ;
			newNode->numbercoocurrence = 1;
			newNode->next = NULL;
			LinkedList = newNode;
		    }
		    else
		    {
			temp = LinkedList;
			while ( temp->next != NULL )
			{
			    if ( temp->nodeI == greyI && temp->nodeJ == greyJ )
			    {
				temp->numbercoocurrence += 1;
				findVal = 1;
			    }
			    if ( temp->nodeI == greyJ && temp->nodeJ == greyI )
			    {
				temp->numbercoocurrence += 1;
				findVal = 1;
			    }

			    temp = temp->next;
			}

			if ( findVal == 0 )
			{
			    newNode = new Node;
			    newNode->nodeI = greyI;
			    newNode->nodeJ = greyJ;
			    newNode->numbercoocurrence = 1;
			    newNode->next = NULL;
			    temp->next = newNode;
			}
			findVal = 0;
		    }
		}
	    }
	}

//------------------------------------------------------------------------------
//			Direction 04 - Azimuth 0 / Dip 135
//------------------------------------------------------------------------------

	if ( direction_==3 || direction_==13 || direction_==16 )
	{
	    for ( int isamp=(sampgate_.start+1); isamp<=sampgate_.stop; isamp++)
	    {
		for ( int posidx=0; posidx<(inpdata_.size()-1); posidx++ )
		{
		    if ( (posidx+1) % ( 2*stepout_.inl()+1 )!=0 || posidx == 0 )
		    {
			float shift = 0;
			if ( dosteer_ )
			{
			    const int steeridx =
					posandsteeridx_.steerindexes_[posidx+1];
			    shift = getInputValue( *steerdata_, steeridx,
						   idx, z0 );
			}
			const float valI =
			    getInterpolInputValue( *inpdata_[posidx], dataidx_,
						   idx+isamp+shift, z0 );
			const float valJ =
			    getInterpolInputValue( *inpdata_[posidx+1],
						   dataidx_, idx+isamp+shift-1,
						   z0 );
			greyI = computeGreyLevel( valI );
			greyJ = computeGreyLevel( valJ );
			elements += 2;

			if ( LinkedList == NULL)
			{
			    newNode = new Node;
			    newNode->nodeI = greyI;
			    newNode->nodeJ = greyJ;
			    newNode->numbercoocurrence = 1;
			    newNode->next = NULL;
			    LinkedList = newNode;
			}
			else
			{
			    temp = LinkedList;
			    while ( temp->next != NULL )
			    {
				if ( temp->nodeI == greyI
				  && temp->nodeJ == greyJ )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}
				if ( temp->nodeI == greyJ
				  && temp->nodeJ == greyI )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}

				temp = temp->next;
			    }

			    if ( findVal == 0 )
			    {
				newNode = new Node;
				newNode->nodeI=greyI;
				newNode->nodeJ=greyJ;
				newNode->numbercoocurrence = 1;
				newNode->next=NULL;
				temp->next = newNode;
			    }
			    findVal = 0;
			}
		    }
		}
	    }
	}

//------------------------------------------------------------------------------
//			Direction 05 - Azimuth 45 / Dip 0
//------------------------------------------------------------------------------

	if ( direction_==4 || direction_==15 || direction_==16 )
	{
	    for ( int isamp=sampgate_.start; isamp<=sampgate_.stop; isamp++)
	    {
		for ( int posidx=0;
		      posidx<(inpdata_.size()-(2*stepout_.inl()+1) ); posidx++ )
		{
		    if ( (posidx+1) % ( 2*stepout_.inl()+1 )!=0 || posidx == 0 )
		    {
			float shift = 0;
			if ( dosteer_ )
			{
			    const int steeridx = posandsteeridx_.steerindexes_[
						    posidx+stepout_.inl()*2+2];
			    shift = getInputValue( *steerdata_, steeridx,
						   idx, z0 );
			}
			const float valI =
			    getInterpolInputValue( *inpdata_[posidx], dataidx_,
						   idx+isamp+shift, z0 );
			const float valJ =
			    getInterpolInputValue(
					*inpdata_[posidx+stepout_.inl()*2+2],
					dataidx_, idx+isamp+shift, z0 );
			greyI = computeGreyLevel( valI );
			greyJ = computeGreyLevel( valJ );
			elements += 2;

			if ( LinkedList == NULL)
			{
			    newNode = new Node;
			    newNode->nodeI = greyI;
			    newNode->nodeJ = greyJ;
			    newNode->numbercoocurrence = 1;
			    newNode->next = NULL;
			    LinkedList = newNode;
			}
			else
			{
			    temp = LinkedList;
			    while ( temp->next != NULL )
			    {
				if ( temp->nodeI == greyI
				  && temp->nodeJ == greyJ )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}
				if ( temp->nodeI == greyJ
				  && temp->nodeJ == greyI )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}

				temp = temp->next;
			    }

			    if ( findVal == 0 )
			    {
				newNode = new Node;
				newNode->nodeI = greyI;
				newNode->nodeJ = greyJ;
				newNode->numbercoocurrence = 1;
				newNode->next = NULL;
				temp->next = newNode;
			    }
			    findVal = 0;
			}
		    }
		}
	    }
	}

//------------------------------------------------------------------------------
//			Direction 06 - Azimuth 45 / Dip 45
//------------------------------------------------------------------------------

	if ( direction_==5 || direction_==16 )
	{
	    for ( int isamp=sampgate_.start; isamp<=(sampgate_.stop-1); isamp++)
	    {
		for ( int posidx=0;
		      posidx<(inpdata_.size()-(2*stepout_.inl()+1) ); posidx++ )
		{
		    if ( (posidx+1) % ( 2*stepout_.inl()+1 ) != 0 || posidx==0)
		    {
			float shift = 0;
			if ( dosteer_ )
			{
			    const int steeridx = posandsteeridx_.steerindexes_[
						    posidx+stepout_.inl()*2+2];
			    shift = getInputValue( *steerdata_, steeridx,
						   idx, z0 );
			}
			const float valI = getInterpolInputValue(
			    *inpdata_[posidx], dataidx_, idx+isamp+shift, z0 );
			const float valJ = getInterpolInputValue(
			    *inpdata_[posidx+stepout_.inl()*2+2], dataidx_,
			    idx+isamp+shift+1, z0 );
			greyI = computeGreyLevel( valI );
			greyJ = computeGreyLevel( valJ );
			elements += 2;

			if ( LinkedList == NULL)
			{
			    newNode = new Node;
			    newNode->nodeI = greyI;
			    newNode->nodeJ = greyJ;
			    newNode->numbercoocurrence = 1;
			    newNode->next = NULL;
			    LinkedList = newNode;
			}
			else
			{
			    temp = LinkedList;
			    while ( temp->next != NULL )
			    {
				if ( temp->nodeI == greyI
				  && temp->nodeJ == greyJ )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}
				if ( temp->nodeI == greyJ
				  && temp->nodeJ == greyI )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}

				temp = temp->next;
			    }

			    if ( findVal == 0 )
			    {
				newNode = new Node;
				newNode->nodeI = greyI;
				newNode->nodeJ = greyJ;
				newNode->numbercoocurrence = 1;
				newNode->next = NULL;
				temp->next = newNode;
			    }
			    findVal = 0;
			}
		    }
		}
	    }
	}

//------------------------------------------------------------------------------
//			Direction 07 - Azimuth 45 / Dip 135
//------------------------------------------------------------------------------

	if ( direction_==6 || direction_==16 )
	{
	    for ( int isamp=(sampgate_.start+1); isamp<=sampgate_.stop; isamp++)
	    {
		for ( int posidx=0;
		      posidx<(inpdata_.size()-(2*stepout_.inl()+1) ); posidx++ )
		{
		    if ( (posidx+1) % (2*stepout_.inl()+1) != 0 || posidx == 0 )
		    {
			float shift = 0;
			if ( dosteer_ )
			{
			    const int steeridx = posandsteeridx_.steerindexes_[
						    posidx+stepout_.inl()*2+2];
			    shift = getInputValue( *steerdata_, steeridx,
						   idx, z0 );
			}
			const float valI = getInterpolInputValue(
					*inpdata_[posidx], dataidx_,
					idx+isamp+shift, z0 );
			const float valJ = getInterpolInputValue(
					*inpdata_[posidx+stepout_.inl()*2+2],
					dataidx_, idx+isamp+shift-1, z0 );
			greyI = computeGreyLevel( valI );
			greyJ = computeGreyLevel( valJ );
			elements += 2;

			if ( LinkedList == NULL)
			{
			    newNode = new Node;
			    newNode->nodeI = greyI;
			    newNode->nodeJ = greyJ;
			    newNode->numbercoocurrence = 1;
			    newNode->next = NULL;
			    LinkedList = newNode;
			}
			else
			{
			    temp = LinkedList;
			    while ( temp->next != NULL )
			    {
				if ( temp->nodeI == greyI
				  && temp->nodeJ == greyJ )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}
				if ( temp->nodeI == greyJ
				  && temp->nodeJ == greyI )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}

				temp = temp->next;
			    }

			    if ( findVal == 0 )
			    {
				newNode = new Node;
				newNode->nodeI = greyI;
				newNode->nodeJ = greyJ;
				newNode->numbercoocurrence = 1;
				newNode->next = NULL;
				temp->next = newNode;
			    }
			    findVal = 0;
			}
		    }
		}
	    }
	}

//------------------------------------------------------------------------------
//			Direction 08 - Azimuth 90 / Dip 0
//------------------------------------------------------------------------------

	if ( direction_==7 || direction_==14 || direction_==15
	  || direction_==16 )
	{
	    for ( int isamp=sampgate_.start; isamp<=sampgate_.stop; isamp++)
	    {
		for ( int posidx=0;
		      posidx<(inpdata_.size()-(stepout_.inl()*2+1)); posidx++ )
		{
		    float shift = 0;
		    if ( dosteer_ )
		    {
			const int steeridx = posandsteeridx_.steerindexes_[
						posidx+(stepout_.inl()*2+1)];
			shift = getInputValue( *steerdata_, steeridx, idx, z0 );
		    }
		    const float valI = getInterpolInputValue(
					*inpdata_[posidx], dataidx_,
					idx+isamp+shift, z0 );
		    const float valJ = getInterpolInputValue(
					*inpdata_[posidx+(stepout_.inl()*2+1)],
					dataidx_, idx+isamp+shift, z0 );
		    greyI = computeGreyLevel( valI );
		    greyJ = computeGreyLevel( valJ );
		    elements += 2;

		    if ( LinkedList == NULL)
		    {
			newNode = new Node;
			newNode->nodeI = greyI;
			newNode->nodeJ = greyJ;
			newNode->numbercoocurrence = 1;
			newNode->next = NULL;
			LinkedList = newNode;
		    }
		    else
		    {
			temp = LinkedList;
			while ( temp->next != NULL )
			{
			    if ( temp->nodeI == greyI && temp->nodeJ == greyJ )
			    {
				temp->numbercoocurrence += 1;
				findVal = 1;
			    }
			    if ( temp->nodeI == greyJ && temp->nodeJ == greyI )
			    {
				temp->numbercoocurrence += 1;
				findVal = 1;
			    }

			    temp = temp->next;
			}

			if ( findVal == 0 )
			{
			    newNode = new Node;
			    newNode->nodeI = greyI;
			    newNode->nodeJ = greyJ;
			    newNode->numbercoocurrence = 1;
			    newNode->next = NULL;
			    temp->next = newNode;
			}
			findVal = 0;
		    }
		}
	    }
	}

//------------------------------------------------------------------------------
//			Direction 09 - Azimuth 90 / Dip 45
//------------------------------------------------------------------------------

	if ( direction_ == 8 || direction_ == 14 || direction_ == 16 )
	{
	    for ( int isamp=sampgate_.start; isamp<=(sampgate_.stop-1); isamp++)
	    {
		for ( int posidx=0;
		      posidx<(inpdata_.size()-(stepout_.inl()*2+1)); posidx++ )
		{
		    float shift = 0;
		    if ( dosteer_ )
		    {
			const int steeridx = posandsteeridx_.steerindexes_[
						posidx+(stepout_.inl()*2+1)];
			shift = getInputValue( *steerdata_, steeridx, idx, z0 );
		    }
		    const float valI = getInterpolInputValue(
			    *inpdata_[posidx], dataidx_, idx+isamp+shift, z0 );
		    const float valJ = getInterpolInputValue(
			    *inpdata_[posidx+(stepout_.inl()*2+1)], dataidx_,
			    idx+isamp+shift+1, z0 );
		    greyI = computeGreyLevel( valI );
		    greyJ = computeGreyLevel( valJ );
		    elements += 2;

		    if ( LinkedList == NULL)
		    {
			newNode = new Node;
			newNode->nodeI = greyI;
			newNode->nodeJ = greyJ;
			newNode->numbercoocurrence = 1;
			newNode->next = NULL;
			LinkedList = newNode;
		    }
		    else
		    {
			temp = LinkedList;
			while ( temp->next != NULL )
			{
			    if ( temp->nodeI == greyI && temp->nodeJ == greyJ )
			    {
				temp->numbercoocurrence += 1;
				findVal = 1;
			    }
			    if ( temp->nodeI == greyJ && temp->nodeJ == greyI )
			    {
				temp->numbercoocurrence += 1;
				findVal = 1;
			    }

			    temp = temp->next;
			}

			if ( findVal == 0 )
			{
			    newNode = new Node;
			    newNode->nodeI = greyI;
			    newNode->nodeJ = greyJ;
			    newNode->numbercoocurrence = 1;
			    newNode->next = NULL;
			    temp->next = newNode;
			}
			findVal = 0;
		    }
		}
	    }
	}

//------------------------------------------------------------------------------
//			Direction 10 - Azimuth 90 / Dip 135
//------------------------------------------------------------------------------

	if ( direction_==9 || direction_==14 || direction_==16 )
	{
	    for ( int isamp=(sampgate_.start+1); isamp<=sampgate_.stop; isamp++)
	    {
		for ( int posidx=0;
		      posidx<(inpdata_.size()-(stepout_.inl()*2+1)); posidx++ )
		{
		    float shift = 0;
		    if ( dosteer_ )
		    {
			const int steeridx = posandsteeridx_.steerindexes_[
						posidx+(stepout_.inl()*2+1)];
			shift = getInputValue( *steerdata_, steeridx, idx, z0 );
		    }
		    const float valI = getInterpolInputValue(
			    *inpdata_[posidx], dataidx_, idx+isamp+shift, z0 );
		    const float valJ = getInterpolInputValue(
			    *inpdata_[posidx+(stepout_.inl()*2+1)], dataidx_,
			    idx+isamp+shift-1, z0 );
		    greyI = computeGreyLevel( valI );
		    greyJ = computeGreyLevel( valJ );
		    elements += 2;

		    if ( LinkedList == NULL)
		    {
			newNode = new Node;
			newNode->nodeI = greyI;
			newNode->nodeJ = greyJ;
			newNode->numbercoocurrence = 1;
			newNode->next = NULL;
			LinkedList = newNode;
		    }
		    else
		    {
			temp = LinkedList;
			while ( temp->next != NULL )
			{
			    if ( temp->nodeI == greyI && temp->nodeJ == greyJ )
			    {
				temp->numbercoocurrence += 1;
				findVal = 1;
			    }
			    if ( temp->nodeI == greyJ && temp->nodeJ == greyI )
			    {
				temp->numbercoocurrence += 1;
				findVal = 1;
			    }

			    temp = temp->next;
			}

			if ( findVal == 0 )
			{
			    newNode = new Node;
			    newNode->nodeI = greyI;
			    newNode->nodeJ = greyJ;
			    newNode->numbercoocurrence = 1;
			    newNode->next = NULL;
			    temp->next = newNode;
			}
			findVal = 0;
		    }
		}
	    }
	}

//------------------------------------------------------------------------------
//			Direction 11 - Azimuth 135 / Dip 0
//------------------------------------------------------------------------------

	if ( direction_ == 10 || direction_ == 15 || direction_ == 16 )
	{
	    for ( int isamp=sampgate_.start; isamp<=sampgate_.stop; isamp++)
	    {
		for ( int posidx=0;
		      posidx<(inpdata_.size()-(2*stepout_.inl()+1) ); posidx++ )
		{
		    if ( (posidx) % ( 2*stepout_.inl()+1 ) != 0 )
		    {
			float shift = 0;
			if ( dosteer_ )
			{
			    const int steeridx = posandsteeridx_.steerindexes_[
						    posidx+stepout_.inl()*2];
			    shift = getInputValue( *steerdata_, steeridx,
						   idx, z0 );
			}
			const float valI = getInterpolInputValue(
					*inpdata_[posidx], dataidx_,
					idx+isamp+shift, z0 );
			const float valJ = getInterpolInputValue(
					*inpdata_[posidx+stepout_.inl()*2],
					dataidx_, idx+isamp+shift, z0 );
			greyI = computeGreyLevel( valI );
			greyJ = computeGreyLevel( valJ );
			elements += 2;

			if ( LinkedList == NULL)
			{
			    newNode = new Node;
			    newNode->nodeI = greyI;
			    newNode->nodeJ = greyJ;
			    newNode->numbercoocurrence = 1;
			    newNode->next = NULL;
			    LinkedList = newNode;
			}
			else
			{
			    temp = LinkedList;
			    while ( temp->next != NULL )
			    {
				if ( temp->nodeI == greyI
				  && temp->nodeJ == greyJ )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}
				if ( temp->nodeI == greyJ
				  && temp->nodeJ == greyI )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}

				temp = temp->next;
			    }

			    if ( findVal == 0 )
			    {
				newNode = new Node;
				newNode->nodeI = greyI;
				newNode->nodeJ = greyJ;
				newNode->numbercoocurrence = 1;
				newNode->next = NULL;
				temp->next = newNode;
			    }
			    findVal = 0;
			}
		    }
		}
	    }
	}

//------------------------------------------------------------------------------
//			Direction 12 - Azimuth 135 / Dip 45
//------------------------------------------------------------------------------

	if ( direction_==11 || direction_==16 )
	{
	    for ( int isamp=sampgate_.start; isamp<=(sampgate_.stop-1); isamp++)
	    {
		for ( int posidx=0;
		      posidx<(inpdata_.size()-(2*stepout_.inl()+1) ); posidx++ )
		{
		    if ( (posidx) % ( 2*stepout_.inl()+1 ) != 0 )
		    {
			float shift = 0;
			if ( dosteer_ )
			{
			    const int steeridx = posandsteeridx_.steerindexes_[
						    posidx+stepout_.inl()*2];
			    shift = getInputValue( *steerdata_, steeridx,
						   idx, z0 );
			}
			const float valI = getInterpolInputValue(
			    *inpdata_[posidx], dataidx_, idx+isamp+shift, z0 );
			const float valJ = getInterpolInputValue(
			    *inpdata_[posidx+stepout_.inl()*2], dataidx_,
			    idx+isamp+shift+1, z0 );
			greyI = computeGreyLevel( valI );
			greyJ = computeGreyLevel( valJ );

			elements += 2;

			if ( LinkedList == NULL)
			{
			    newNode = new Node;
			    newNode->nodeI = greyI;
			    newNode->nodeJ = greyJ;
			    newNode->numbercoocurrence = 1;
			    newNode->next = NULL;
			    LinkedList = newNode;
			}
			else
			{
			    temp = LinkedList;
			    while ( temp->next != NULL )
			    {
				if ( temp->nodeI == greyI
				  && temp->nodeJ == greyJ )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}
				if ( temp->nodeI == greyJ
				  && temp->nodeJ == greyI )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}

				temp = temp->next;
			    }

			    if ( findVal == 0 )
			    {
				newNode = new Node;
				newNode->nodeI = greyI;
				newNode->nodeJ = greyJ;
				newNode->numbercoocurrence = 1;
				newNode->next = NULL;
				temp->next = newNode;
			    }
			    findVal = 0;
			}
		    }
		}
	    }
	}

//------------------------------------------------------------------------------
//			Direction 13 - Azimuth 135 / Dip 135
//------------------------------------------------------------------------------

	if ( direction_==12 || direction_==16 )
	{
	    for ( int isamp=(sampgate_.start+1); isamp<=sampgate_.stop; isamp++)
	    {
		for ( int posidx=0;
		      posidx<(inpdata_.size()-(2*stepout_.inl()+1) ); posidx++ )
		{
		    if ( (posidx) % ( 2*stepout_.inl()+1 ) != 0 )
		    {
			float shift = 0;
			if ( dosteer_ )
			{
			    const int steeridx = posandsteeridx_.steerindexes_[
						    posidx+stepout_.inl()*2];
			    shift = getInputValue( *steerdata_, steeridx,
						   idx, z0 );
			}
			const float valI = getInterpolInputValue(
					    *inpdata_[posidx], dataidx_,
					    idx+isamp+shift, z0 );
			const float valJ = getInterpolInputValue(
					    *inpdata_[posidx+stepout_.inl()*2],
					    dataidx_, idx+isamp+shift-1, z0 );
			greyI = computeGreyLevel( valI );
			greyJ = computeGreyLevel( valJ );

			elements += 2;

			if ( LinkedList == NULL)
			{
			    newNode = new Node;
			    newNode->nodeI = greyI;
			    newNode->nodeJ = greyJ;
			    newNode->numbercoocurrence = 1;
			    newNode->next = NULL;
			    LinkedList = newNode;
			}
			else
			{
			    temp = LinkedList;
			    while ( temp->next != NULL )
			    {
				if ( temp->nodeI == greyI
				  && temp->nodeJ == greyJ )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}
				if ( temp->nodeI == greyJ
				  && temp->nodeJ == greyI )
				{
				    temp->numbercoocurrence += 1;
				    findVal = 1;
				}

				temp = temp->next;
			    }

			    if ( findVal == 0 )
			    {
				newNode = new Node;
				newNode->nodeI = greyI;
				newNode->nodeJ = greyJ;
				newNode->numbercoocurrence = 1;
				newNode->next = NULL;
				temp->next = newNode;
			    }
			    findVal = 0;
			}
		    }
		}
	    }
	}
//------------------------------------------------------------------------------
//			Calculation of GLCM Attributes
//------------------------------------------------------------------------------

	double trcval = 0;

	switch ( attribute_ )
	{
	    case 0:
	    {
		trcval = computeEnergy( LinkedList, elements );
		break;
	    }
	    case 1:
	    {
		trcval = computeContrast( LinkedList, elements );
		break;
	    }
	    case 2:
	    {
		trcval = computeCorrelation( LinkedList, elements );
		break;
	    }
	    case 3:
	    {
		trcval = computeVariance( LinkedList, elements );
		break;
	    }
	    case 4:
	    {
		trcval = computeInverseDifferenceMoment( LinkedList, elements );
		break;
	    }
	    case 5:
	    {
		trcval = computeSumAverage( LinkedList, elements );
		break;
	    }
	    case 6:
	    {
		trcval = computeSumVariance( LinkedList, elements );
		break;
	    }
	    case 7:
	    {
		trcval = computeSumEntropy( LinkedList, elements );
		break;
	    }
	    case 8:
	    {
		trcval = computeEntropy( LinkedList, elements );
		break;
	    }
	    case 9:
	    {
		trcval = computeDifferenceVariance( LinkedList, elements );
		break;
	    }
	    case 10:
	    {
		trcval = computeDifferenceEntropy( LinkedList, elements );
		break;
	    }
	    case 11:
	    {
		trcval = computeF12( LinkedList, elements );
		break;
	    }
	    case 12:
	    {
		trcval = computeF13( LinkedList, elements );
		break;
	    }
	    case 13:
	    {
		trcval = computeHomogeneity( LinkedList, elements );
		break;
	    }
	    case 14:
	    {
		trcval = computeSumMean( LinkedList, elements );
		break;
	    }
	    case 15:
	    {
		trcval = computeMaximumProbability( LinkedList, elements );
		break;
	    }
	    case 16:
	    {
		trcval = computeClusterTendency( LinkedList, elements );
		break;
	    }
	    case 17:
	    {
		trcval = computeClusterShade( LinkedList, elements );
		break;
	    }
	    case 18:
	    {
		trcval = computeClusterProminence( LinkedList, elements );
		break;
	    }
	    case 19:
	    {
		trcval = computeDissimilarity( LinkedList, elements );
		break;
	    }
	    case 20:
	    {
		trcval = computeDifferenceMean( LinkedList, elements );
		break;
	    }
	    case 21:
	    {
		trcval = computeAutocorrelation( LinkedList, elements );
		break;
	    }
	    case 22:
	    {
		trcval = computeInertia( LinkedList, elements );
		break;
	    }
	}

	outval = mCast(float,trcval);

	setOutputValue( output, 0, idx, z0, outval );

	temp = LinkedList;
	while ( temp )
	{
	    LinkedList = temp->next;
	    delete temp;
	    temp = LinkedList;
	}

    }

    return true;
}


std::pair<double, double>  GLCM_attrib::computeMu( Node* LinkedList,
						   int elements ) const
{
    double MuX = 0.0;
    double MuY = 0.0;
    Node* values = LinkedList;
    while ( values )
    {
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	int i = values->nodeI;
	int j = values->nodeJ;
	MuX += (i+1) * mval;
	MuY += (j+1) * mval;
	values = values->next;
    }
    return std::make_pair(MuX, MuY);
}

std::pair<double, double>  GLCM_attrib::computeSigma( Node* LinkedList,
						      int elements, double MuX,
						      double MuY ) const
{
    double SigmaX = 0.0;
    double SigmaY = 0.0;
    Node* values = LinkedList;

    while ( values )
    {
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	SigmaX += mval * pow(double (1-MuX),2);
	SigmaY += mval * pow(double (1-MuY),2);
	values = values->next;
    }
    return std::make_pair(SigmaX, SigmaY);
}


double GLCM_attrib::computeMean( Node* LinkedList, int elements ) const
{
    double Mean = 0;
    Node* values = LinkedList;

    while ( values )
    {
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	Mean += mval/((usegreylevels_+1)*2);
	values = values->next;
    }
    return Mean;
}


double GLCM_attrib::computeMuXminusY( Node* LinkedList,
				      int elements ) const
{
    double MuXminusY = 0;

    for ( int n=0; n<usegreylevels_; n++)
    {
	double MXminusY = 0;
	MXminusY = computeMXminusY( LinkedList, elements, n );
	MuXminusY += n * MXminusY;
    }
    return MuXminusY;
}


double GLCM_attrib::computeMXplusY( Node* LinkedList, int elements,
				    int nr ) const
{
    double MXplusY = 0;
    Node* values = LinkedList;

    while ( values )
    {
	if ( values->nodeI + values->nodeJ == nr )
	{
	    double mval = values->numbercoocurrence /
				static_cast<double>(elements);
	    MXplusY += mval;
	}
	values = values->next;
    }

    return MXplusY;
}


double GLCM_attrib::computeMXminusY( Node* LinkedList, int elements,
				     int nr ) const
{
    double MXminusY = 0;
    Node* values = LinkedList;

    while ( values )
    {
	if ( (values->nodeI - values->nodeJ == nr )
	  || (values->nodeI - values->nodeJ == -nr ) )
	{
	    double mval = values->numbercoocurrence /
				static_cast<double>(elements);
	    MXminusY += mval;
	}
	values = values->next;
    }

    return MXminusY;
}


double GLCM_attrib::computeMxI( Node* LinkedList, int elements,
				int i ) const
{
    double MxI = 0;
    Node* values = LinkedList;

    while ( values )
    {
	if ( values->nodeI == i )
	{
	    double mval = values->numbercoocurrence /
			    static_cast<double>(elements);
	    MxI += mval;
	}
	values = values->next;
    }

    return MxI;
}


double GLCM_attrib::computeMyJ( Node* LinkedList, int elements,
				int j ) const
{
    double MyJ = 0;
    Node* values = LinkedList;

    while ( values )
    {
	if ( values->nodeJ == j )
	{
	    double mval = values->numbercoocurrence /
			static_cast<double>(elements);
	    MyJ += mval;
	}
	values = values->next;
    }

    return MyJ;
}


double GLCM_attrib::computeHXY1( Node* LinkedList, int elements ) const
{
    double HXY1 = 0;
    Node* values = LinkedList;

    while ( values )
    {
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	int i = values->nodeI;
	int j = values->nodeJ;
	double MxI = computeMxI( LinkedList, elements, i );
	double MyJ = computeMyJ( LinkedList, elements, j );
	HXY1 += mval * log(MxI*MyJ+0.000001) * (-1);

	values = values->next;
    }

    return HXY1;
}


double GLCM_attrib::computeHXY2( Node* LinkedList, int elements ) const
{
    double HXY2 = 0;
    Node* values = LinkedList;

    while ( values )
    {
	int i = values->nodeI;
	int j = values->nodeJ;
	double MxI = computeMxI( LinkedList, elements, i );
	double MyJ = computeMyJ( LinkedList, elements, j );
	HXY2 += MxI * MyJ * log(MxI*MyJ + 0.0000001) * (-1);

	values = values->next;
    }

    return HXY2;
}


double GLCM_attrib::computeHX( Node* LinkedList, int elements ) const
{
    double HX = 0;
    Node* values = LinkedList;

    while ( values )
    {
	int i = values->nodeI;
	double MxI = computeMxI( LinkedList, elements, i );
	HX += MxI * log(MxI + 0.0000001) * (-1);

	values = values->next;
    }

    return HX;
}


double GLCM_attrib::computeHY( Node* LinkedList, int elements ) const
{
    double HY = 0;
    Node* values = LinkedList;

    while ( values )
    {
	int j = values->nodeJ;
	double MyJ = computeMxI( LinkedList, elements, j );
	HY += MyJ * log(MyJ + 0.0000001) * (-1);

	values = values->next;
    }

    return HY;
}


double GLCM_attrib::computeEnergy( Node* LinkedList, int elements ) const
{
    double trcval = 0;
    Node* values = LinkedList;

    while ( values )
    {
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	trcval += mval*mval;
	values = values->next;
    }
    return trcval;
}


double GLCM_attrib::computeContrast( Node* LinkedList,
				     int elements ) const
{
    double trcval = 0;
    Node* values = LinkedList;

    for ( int n=0; n<usegreylevels_-1; n++ )
    {
	while ( values )
	{
	    int i = values->nodeI;
	    int j = values->nodeJ;
	    double mval = values->numbercoocurrence /
			static_cast<double>(elements);
	    trcval += mval*static_cast<double>(i-j+2)
					*static_cast<double>(i-j+2);
	    values = values->next;
	}
    }
    return trcval;
}


double GLCM_attrib::computeCorrelation( Node* LinkedList,
					int elements ) const
{
    std::pair<double, double> Mu(0.0, 0.0);
    Mu = computeMu(LinkedList, elements);
    std::pair<double, double> Sigma(0.0, 0.0);
    Sigma = computeSigma(LinkedList, elements, Mu.first, Mu.second);

    double trcval = 0;
    Node* values = LinkedList;

    while ( values )
    {
	int i = values->nodeI;
	int j = values->nodeJ;
	double mval = values->numbercoocurrence / mCast(double,elements);
	trcval += ( mCast(double,i+1) * mCast(double,j+1) * mval
			- Mu.first*Mu.second ) / (Sigma.first*Sigma.second);
	values = values->next;
    }
    return trcval;
}


double GLCM_attrib::computeVariance( Node* LinkedList,
				     int elements ) const
{
    double Mean = computeMean( LinkedList, elements );
    double trcval = 0;
    Node* values = LinkedList;

    while ( values )
    {
	int i = values->nodeI;
	double mval = values->numbercoocurrence / mCast(double,elements);
	trcval += (i-Mean+1) * (i-Mean+1) * mval;
	values = values->next;
    }
    return trcval;
}


double GLCM_attrib::computeInverseDifferenceMoment( Node* LinkedList,
						    int elements ) const
{
    double trcval = 0;
    Node* values = LinkedList;

    while ( values )
    {
	int i = values->nodeI;
	int j = values->nodeJ;
	double mval = values->numbercoocurrence / mCast(double,elements);
	trcval += mval / (1+mCast(double,i-j+2) * mCast(double,i-j+2));
	values = values->next;
    }
    return trcval;
}


double GLCM_attrib::computeSumAverage( Node* LinkedList,
				       int elements ) const
{
    double trcval = 0;

    for ( int n=0; n<(2*usegreylevels_-1); n++ )
    {
	    double MXplusY=0;
	    MXplusY = computeMXplusY( LinkedList, elements, n );
	    trcval += MXplusY * (n+2);
    }
    return trcval;
}


double GLCM_attrib::computeSumVariance( Node* LinkedList,
					int elements ) const
{
    double trcval = 0;

    double f8 = computeSumEntropy( LinkedList, elements );

    for ( int n=0; n<(2*usegreylevels_-1); n++ )
    {
	double MXplusY = computeMXplusY( LinkedList, elements, n );
	trcval += (n+2-f8) * (n+2-f8) * MXplusY;
    }
    return trcval;
}


double GLCM_attrib::computeSumEntropy( Node* LinkedList,
				       int elements ) const
{
    double trcval = 0;

    for ( int n=0; n<(2*usegreylevels_-1); n++ )
    {
	double MXplusY = 0;
	MXplusY = computeMXplusY( LinkedList, elements, n );
	trcval += MXplusY * log(MXplusY + 0.0000001) * (-1);
    }
    return trcval;
}


double GLCM_attrib::computeEntropy( Node* LinkedList,
				    int elements ) const
{
    double trcval = 0;
    Node* values = LinkedList;

    while (values )
    {
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	trcval += mval * log(mval + 0.000001) * (-1);
	values = values->next;
    }
    return trcval;
}


double GLCM_attrib::computeDifferenceVariance( Node *LinkedList,
					       int elements ) const
{
    double trcval = 0;
    double MuXminusY = computeMuXminusY( LinkedList, elements );

    for ( int n=0; n<usegreylevels_; n++ )
    {
	double MXminusY = computeMXminusY( LinkedList, elements, n );
	trcval += (n-MuXminusY) * (n-MuXminusY) * MXminusY * (-1);
    }
    return trcval;
}


double GLCM_attrib::computeDifferenceEntropy( Node* LinkedList,
					      int elements ) const
{
    double trcval = 0;

    for ( int n=0; n<usegreylevels_; n++ )
    {
	double MXminusY = computeMXminusY( LinkedList, elements, n );
	trcval += MXminusY * log(MXminusY + 0.0000001) * (-1);
    }
    return trcval;
}


double GLCM_attrib::computeF12( Node* LinkedList, int elements ) const
{
    double trcval = 0;
    double MaxHX_HY = 0;

    double f9 = computeEntropy( LinkedList, elements );
    double HXY1 = computeHXY1( LinkedList, elements );
    double HX = computeHX( LinkedList, elements );
    double HY = computeHY( LinkedList, elements );

    if ( HX < HY )
	MaxHX_HY = HY;
    else if ( HY <= HX )
	MaxHX_HY = HX;

    trcval = (f9-HXY1) / (MaxHX_HY);
    return trcval;
}


double GLCM_attrib::computeF13( Node* LinkedList, int elements ) const
{
    double trcval = 0;

    double f9 = computeEntropy( LinkedList, elements );
    double HXY2 = computeHXY2( LinkedList, elements );

    double SquarePart = 1 - exp(-2*(HXY2-f9));
    if ( SquarePart >= 0 )
	trcval = Math::Sqrt(SquarePart);
    else if ( SquarePart < 0 )
	trcval = 0;

    return trcval;
}


double GLCM_attrib::computeHomogeneity( Node* LinkedList,
					int elements ) const
{
    double trcval = 0;
    Node *values = LinkedList;

    while ( values )
    {
	int i = values->nodeI;
	int j = values->nodeJ;
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	trcval += mval / double(1+ Math::Abs(i-j+2));
	values = values->next;
    }
    return trcval;
}

double GLCM_attrib::computeSumMean( Node* LinkedList,
				    int elements ) const
{
    double trcval = 0;
    Node* values = LinkedList;

    while ( values )
    {
	int i = values->nodeI;
	int j = values->nodeJ;
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	trcval += 0.5 * mval * static_cast<double>(i+j+2);
	values = values->next;
    }
    return trcval;
}


double GLCM_attrib::computeMaximumProbability( Node* LinkedList,
					       int elements ) const
{
    double trcval = 0;
    Node* values = LinkedList;

    while ( values )
    {
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	if ( mval > trcval )
	    trcval = mval;

	values = values->next;
    }
    return trcval;
}


double GLCM_attrib::computeClusterTendency( Node* LinkedList,
					    int elements ) const
{
    std::pair<double, double> Mu(0.0, 0.0);
    Mu = computeMu(LinkedList, elements);

    double trcval = 0;
    Node* values = LinkedList;

    while ( values )
    {
	int i = values->nodeI;
	int j = values->nodeJ;
	double mval = values->numbercoocurrence /
			static_cast<double>(elements);
	trcval += mval * pow(double (i+j+2-Mu.first-Mu.second),2);
	values = values->next;
    }
    return trcval;
}


double GLCM_attrib::computeClusterShade( Node* LinkedList, int elements ) const
{
    std::pair<double, double> Mu(0.0, 0.0);
    Mu = computeMu( LinkedList, elements );

    double trcval = 0;
    Node* values = LinkedList;

    while ( values )
    {
	int i = values->nodeI;
	int j = values->nodeJ;
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	trcval += mval * pow(double (i+j+2-Mu.first-Mu.second),3);
	values = values->next;
    }
    return trcval;
}


double GLCM_attrib::computeClusterProminence( Node* LinkedList,
					      int elements ) const
{
    std::pair<double, double> Mu(0.0, 0.0);
    Mu = computeMu( LinkedList, elements );

    double trcval = 0;
    Node* values = LinkedList;

    while ( values )
    {
	int i = values->nodeI;
	int j = values->nodeJ;
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	trcval += mval*pow(double (i+j+2-Mu.first-Mu.second),4);
	values = values->next;
    }
    return trcval;
}


double GLCM_attrib::computeDissimilarity( Node* LinkedList,
					  int elements ) const
{
    double trcval = 0;
    Node* values = LinkedList;

    while ( values )
    {
	int i = values->nodeI;
	int j = values->nodeJ;
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	trcval += mval*Math::Abs(static_cast<double>(i-j+2));
	values = values->next;
    }
    return trcval;
}


double GLCM_attrib::computeDifferenceMean( Node* LinkedList,
					   int elements ) const
{
    double trcval = 0;
    Node* values = LinkedList;

    while ( values )
    {
	int i = values->nodeI;
	int j = values->nodeJ;
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	trcval += mval * (i-j+2) * 0.5;
	values = values->next;
    }
    return trcval;
}


double GLCM_attrib::computeAutocorrelation( Node* LinkedList,
					    int elements ) const
{
    double trcval = 0;
    Node* values = LinkedList;

    while ( values )
    {
	int i = values->nodeI;
	int j = values->nodeJ;
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	trcval += mval*static_cast<double>(i+1)*static_cast<double>(j+1);
	values = values->next;
    }
    return trcval;
}


double GLCM_attrib::computeInertia( Node* LinkedList,
				    int elements ) const
{
    double trcval = 0;
    Node* values = LinkedList;

    while ( values )
    {
	int i = values->nodeI;
	int j = values->nodeJ;
	double mval = values->numbercoocurrence / static_cast<double>(elements);
	trcval += mval*static_cast<double>(i-j+2)*static_cast<double>(i-j+2);
	values=values->next;
    }
    trcval = trcval / ( ((usegreylevels_+1)*(usegreylevels_+1)-1) *
		((usegreylevels_+1)*(usegreylevels_+1)-1) );
    return trcval;
}


const BinID* GLCM_attrib::reqStepout( int input, int output ) const
{
    return &stepout_;
}


const Interval<int>* GLCM_attrib::desZSampMargin( int, int ) const
{
    return &sampgate_;
}


const Interval<float>* GLCM_attrib::reqZMargin( int inp, int ) const
{
    return &gate_;
}


const Interval<float>* GLCM_attrib::desZMargin( int inp, int ) const
{
    return &desgate_;
}

}
