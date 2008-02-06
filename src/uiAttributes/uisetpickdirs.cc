/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert Bril
 Date:          Nov 2003
 RCS:           $Id: uisetpickdirs.cc,v 1.10 2008-02-06 04:22:04 cvsraman Exp $
________________________________________________________________________

-*/


#include "uisetpickdirs.h"
#include "attribsel.h"
#include "ctxtioobj.h"
#include "seistrctr.h"
#include "pickset.h"
#include "uigeninput.h"
#include "uiattrsel.h"
#include "uisteeringsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "sorting.h"
#include "scaler.h"
#include "binidvalset.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
//#include "attribdescsetproc.h"
//#include "attriboutputimpl.h"
//#include "attribexecutor.h"
#include "uitaskrunner.h"
#include "survinfo.h"
#include "rcol2coord.h"

#include <math.h>


using namespace Attrib;

uiSetPickDirs::uiSetPickDirs( uiParent* p, Pick::Set& s,
			      const DescSet* a, const NLAModel* n )
	: uiDialog(p,uiDialog::Setup("Add direction to Pick Set",
				     "Specify directions for picks",
				     "105.1.1"))
	, ps(s)
	, ads(a?a->clone():new DescSet(false) )//TODO verify for 2D
	, nlamdl(n)
	, dirinpfld(0)
	, phifld(0)
	, steerfld(0)
	, steerctio(0)
	, usesteering(true)
	, createdset(0)
{
    SelInfo attrselinfo( ads, nlamdl );
    if ( attrselinfo.ioobjids.size() == 0 )
    {
	new uiLabel( this, "Please import a seismic cube first" );
	return;
    }

//  const bool havesteer = AF().forms(true).size() > 0;
    const bool havesteer = true;
    if ( havesteer )
    {
	dirinpfld = new uiGenInput( this, "Direction from", 
			BoolInpSpec(true,"Steering cube","Attributes") );
	dirinpfld->valuechanged.notify( mCB(this,uiSetPickDirs,dirinpSel) );
    	steerctio = mMkCtxtIOObj( SeisTrc );
	steerctio->ctxt.forread = true;
	steerfld = new uiSteerCubeSel( this, *steerctio, ads, "Steering cube" );
	steerfld->attach( alignedBelow, dirinpfld );
    }

    uiAttrSelData ad( ads );
    ad.nlamodel = nlamdl;
    const bool is2d = ads ? ads->is2D() : false;
    phifld = new uiAttrSel( this, "Azimuth Angle ~ North (phi=[0-360])",
	    		    ad, is2d );
    if ( dirinpfld )
	phifld->attach( alignedBelow, dirinpfld );
    thetafld = new uiAttrSel( this, "Dip Angle ~ Horizontal (theta=[-90-90])", 
	    		      ad, is2d );
    thetafld->attach( alignedBelow, phifld );

    rfld = new uiAttrSel( this, "Vector length (R)", ad, is2d );
    rfld->attach( alignedBelow, thetafld );

    finaliseDone.notify( mCB(this,uiSetPickDirs,dirinpSel) );
}


uiSetPickDirs::~uiSetPickDirs()
{
    delete ads;
    delete createdset;
    if ( steerctio )
	{ steerctio->destroyAll(); delete steerctio; }
}


void uiSetPickDirs::dirinpSel( CallBacker* )
{
    if ( !phifld ) return;

    usesteering = dirinpfld && dirinpfld->getBoolValue();
    if ( steerfld )
	steerfld->display( usesteering );
    phifld->display( !usesteering );
    thetafld->display( !usesteering );
}


#define mErrRet(msg) \
{ uiMSG().error( msg ); return false; }

bool uiSetPickDirs::acceptOK( CallBacker* )
{
    pErrMsg( "Not implemented yet" );
    return false;
/*
    if ( !phifld ) return true;

    if ( usesteering && !*steerfld->getInput() )
	mErrRet( "Please, select Steering Cube" )
    if ( !usesteering && ( !*phifld->getInput() || !*thetafld->getInput() ) )
	mErrRet( "Please, select input attribute(s)" )

    BinIDValueSet locations( 1, true );
    for ( int idx=0; idx<ps.size(); idx++ )
    {
	Pick::Location pl( ps[idx] );
	locations.add( SI().transform(pl.pos), pl.z );
    }

    BoolTypeSet issel;
    TypeSet<int> idxs;
    if ( !getAttribSelection(issel,idxs) )
	return false;

    FeatureSet* fs = calcAttribs( locations, issel );
    if ( !fs ) 
	mErrRet( "Cannot calculate attributes at these positions" );

    TypeSet<float> thetas, phis;
    RunningStatistics<float> radia;
    BinIDValueSet::Pos pos; BinIDValue biv;
    while ( locations.next(pos) )
    {
	locations.get( pos, biv );
	const FVPos pos( biv.binid.inl, biv.binid.crl, biv.value );
	FeatureVec* fvec = fs->getVec( pos );
	if ( !fvec ) continue;

	float phi = 0;
	float theta = 0;
	if ( usesteering )
	{
	    const float inldip = (*fvec)[ idxs[0] ];
	    const float crldip = (*fvec)[ idxs[1] ];
	    if ( !mIsUndefined(inldip) && !mIsUndefined(crldip) )
	    {
		phi = calcPhi( inldip, crldip );
		theta = calcTheta( inldip, crldip );
	    }
	}
	else
	{
	    phi = (*fvec)[ idxs[0] ] * M_PI / 180;
	    theta = (*fvec)[ idxs[1] ] * M_PI / 180;
	    if ( !mIsUndefined(phi) && !mIsUndefined(theta) )
	    {
		wrapPhi( phi );
		wrapTheta( theta );
	    }
	    else
	    { phi = 0; theta = 0; }
	}

	float radius = idxs.size()>2 ? (*fvec)[ idxs[2] ] : 1;
	if ( mIsUndefined(radius) ) radius = 0;
	thetas += theta;
	phis += phi;
	radia += radius;
    }

    float rmin = radia.min();
    float rmax = radia.max();
    LinScaler scaler( 0, 1 );
    if ( mIsEqual(rmax,rmin,mDefEps) )
    {
	scaler.factor = 1.0 / rmin;
	scaler.constant = 0;
    }
    else
    {
	scaler.factor = 1.0 / ( rmax-rmin );
	scaler.constant = -rmin * scaler.factor;
    }
    
    for ( int idx=0; idx<phis.size(); idx++ )
    {
	float radius = radia.getData()[idx];
	ps[idx].dir = Sphere( scaler.scale(radius), thetas[idx], phis[idx] );
    }

    return true;
    */
}


bool uiSetPickDirs::getAttribSelection( BoolTypeSet& issel, TypeSet<int>& idxs )
{
    TypeSet<DescID> nlaids;
    if ( !getNLAIds(nlaids) )
	return false;

    TypeSet<DescID> ids;
    if ( usesteering )
    {
	if ( createdset )
	    steerfld->setDescSet( createdset );

	const DescID inldipid = steerfld->inlDipID();
	if ( inldipid < 0 ) mErrRet( "Cannot read Steering Cube" )

	ids += inldipid;
	ids += steerfld->crlDipID();
    }
    else
    {
	const DescID phiid = getAttribID( phifld, nlaids );
	if ( phiid < 0 ) mErrRet( "No valid attribute selected for Phi" )
	ids += phiid;
	const DescID thetaid = getAttribID( thetafld, nlaids );
	if ( thetaid < 0 ) mErrRet( "No valid attribute selected for Theta" );
	ids += thetaid;
    }
  
    ids += getAttribID( rfld, nlaids );

    const DescSet* curset = createdset ? createdset : ads;
    issel = BoolTypeSet( curset->nrDescs(), false );
    TypeSet<DescID> allids;
    curset->getIds( allids );
    TypeSet<int> descnrs, sorted;
    for ( int idx=0; idx<ids.size(); idx++ )
    {
	const int descidx = allids.indexOf( ids[idx] );
	issel[descidx] = true;
	descnrs += descidx;
	sorted += descidx;
    }

    sort_array( sorted.arr(), sorted.size() );
    for ( int idx=0; idx<sorted.size(); idx++ )
	idxs += sorted.indexOf( descnrs[idx] );

    return true;
}


bool uiSetPickDirs::getNLAIds( TypeSet<DescID>& ids )
{
    if ( !nlamdl ) return true;

/*
    EngineMan aem;
    aem.setNLAModel( nlamdl );

    SelInfo selinfo( 0, nlamdl );
    const int nrnlaouts = selinfo.nlaoutnms.size();
    for ( int idx=0; idx<nrnlaouts; idx++ )
    {
	if ( !idx )
	{
	    aem.setAttribSpec( SelSpec("",idx,true) );
	    DescID nlaid; BufferString errmsg;
	    createdset = aem.createNLAADS( nlaid, errmsg, ads );
	    if ( errmsg.size() ) mErrRet( errmsg );
	    ids += nlaid;
	    continue;
	}

	Desc* desc0 = createdset->getDesc( ids[0] );
	if ( !desc0 ) continue;
	Desc* ad = desc0->clone();
	ad->setDescSet( createdset );
	BufferString usrref( ad->userRef() ); usrref += "__"; usrref += idx;
	ad->setUserRef( usrref );
	ad->selectOutput( idx );
	DescID nlaid  = createdset->addDesc( ad );
	ids += nlaid;
    }
*/

    return true;
}


DescID uiSetPickDirs::getAttribID( uiAttrSel* attrfld,
				   const TypeSet<DescID>& nlaids )
{
    const DescID attribid = attrfld->attribID();
    const int outputnr = attrfld->outputNr();
    DescID newid( DescID::undef() );
    if ( attribid >= 0 )
	newid = attribid;
    else if ( outputnr >= 0 && nlaids.size() > outputnr )
	newid = nlaids[outputnr];

    return newid;
}


void uiSetPickDirs::calcAttribs( const BinIDValueSet& locations,
       					const BoolTypeSet& issel )
{
    /*
    BufferString errmsg;
    const DescSet* curset = createdset ? createdset : ads;
    DescSetProcessor* processor = new DescSetProcessor( *curset );
    OutputExecutor* outexec = new OutputExecutor( *processor );
    FeatureSetAttribOutput* output = new FeatureSetAttribOutput(*processor,0);
    output->setLocations( locations );
    output->isselected = issel;

    FeatureSet* fs = new FeatureSet;
    output->setFeatureSet( fs );
    outexec->addOutput( output );
    if ( !outexec->init() )
    {
	errmsg = outexec->message();
	delete outexec;
	return 0;
    }

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(*outexec) ) return 0;

    return fs;
*/
}


float uiSetPickDirs::calcPhi( float inldip, float crldip )
{
    const float azi = atan(inldip/crldip);

    const RCol2Coord& b2c = SI().binID2Coord();
    const float xcrl = b2c.getTransform(true).c;
    const float ycrl = b2c.getTransform(false).c;

    const float angN = atan(xcrl/ycrl);

    float phi;
    if ( SI().isClockWise() )
	phi = angN - azi;
    else
	phi = azi + angN;

    if ( phi < 0 ) phi += 2*M_PI;
    return phi;
}


float uiSetPickDirs::calcTheta( float inldip, float crldip )
{
    const float velocity = 2000;
    const float poldip = sqrt( inldip*inldip + crldip*crldip );
    
    float theta = atan( poldip * velocity * 1e-6 );
    return theta;
}


void uiSetPickDirs::wrapPhi( float& phi )
{
    int nrcycles = (int)( phi / (2*M_PI) );
    phi -= (nrcycles * 2*M_PI);
}


void uiSetPickDirs::wrapTheta( float& theta )
{
    float val = tan( theta );
    if ( val == val )
	theta = atan( val );
    else
	theta = M_PI/2;
}
