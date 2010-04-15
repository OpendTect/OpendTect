/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert Bril
 Date:          Nov 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uisetpickdirs.cc,v 1.20 2010-04-15 08:38:53 cvsranojay Exp $";


#include "uisetpickdirs.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribstorprovider.h"
#include "attribsel.h"
#include "ctxtioobj.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "executor.h"
#include "mousecursor.h"
#include "pickset.h"
#include "rcol2coord.h"
#include "seistrctr.h"
#include "separstr.h"
#include "survinfo.h"
#include "undefval.h"

#include "uiattrsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uisteeringsel.h"
#include "uitaskrunner.h"

#include <math.h>


using namespace Attrib;

uiSetPickDirs::uiSetPickDirs( uiParent* p, Pick::Set& s,
			      const DescSet* a, const NLAModel* n )
	: uiDialog(p,uiDialog::Setup("Add direction to Pick Set",
				     "Specify directions for picks",
				     "105.1.1"))
	, ps_( s )
	, ads_( a ? new DescSet(*a) : new DescSet(false) )
	, nlamdl_( n )
	, dirinpfld_( 0 )
	, phifld_( 0 )
	, steerfld_( 0 )
	, steerctio_( 0 )
	, usesteering_( true )
	, createdset_( 0 )
{
    SelInfo attrselinfo( ads_, nlamdl_ );
    if ( attrselinfo.ioobjids.size() == 0 )
    {
	new uiLabel( this, "Please import a seismic cube first" );
	return;
    }

    const bool is2d = ads_ ? ads_->is2D() : false;
    const bool havesteer = true;
    if ( havesteer )
    {
	dirinpfld_ = new uiGenInput( this, "Direction from", 
			BoolInpSpec(true,"Steering cube","Attributes") );
	dirinpfld_->valuechanged.notify( mCB(this,uiSetPickDirs,dirinpSel) );
    	steerctio_ = uiSteerCubeSel::mkCtxtIOObj( is2d, true );
	steerfld_ = new uiSteerCubeSel( this, *steerctio_, ads_, is2d );
	steerfld_->attach( alignedBelow, dirinpfld_ );
    }

    uiAttrSelData asd( *ads_, false );
    asd.nlamodel_ = nlamdl_;
    phifld_ = new uiAttrSel( this, "Azimuth Angle ~ North (phi=[0-360])", asd );
    if ( dirinpfld_ )
	phifld_->attach( alignedBelow, dirinpfld_ );
    thetafld_ = new uiAttrSel( this, "Dip Angle ~ Horizontal (theta=[-90-90])", 
	    		      asd );
    thetafld_->attach( alignedBelow, phifld_ );

    finaliseDone.notify( mCB(this,uiSetPickDirs,dirinpSel) );
}


uiSetPickDirs::~uiSetPickDirs()
{
    delete ads_;
    delete createdset_;
    if ( steerctio_ )
	{ steerctio_->destroyAll(); delete steerctio_; }
}


void uiSetPickDirs::dirinpSel( CallBacker* )
{
    if ( !phifld_ ) return;

    usesteering_ = dirinpfld_ && dirinpfld_->getBoolValue();
    if ( steerfld_ )
	steerfld_->display( usesteering_ );
    phifld_->display( !usesteering_ );
    thetafld_->display( !usesteering_ );
}


#define mErrRet(msg) \
{ uiMSG().error( msg ); return false; }

bool uiSetPickDirs::acceptOK( CallBacker* )
{
    if ( usesteering_ && !*steerfld_->getInput() )
	mErrRet( "Please, select Steering Cube" )
    if ( !usesteering_ && ( !*phifld_->getInput() || !*thetafld_->getInput() ) )
	mErrRet( "Please, select input attribute(s) for Phi and Theta" )

    TypeSet<DataPointSet::DataRow> pts;
    ObjectSet<DataColDef> dcds;
    dcds += new DataColDef( usesteering_ ? "inline dip"
	    				 : phifld_->getAttrName() );
    dcds += new DataColDef( usesteering_ ? "crossline dip"
	    				 : thetafld_->getAttrName() );

    DataPointSet locations( pts, dcds, ads_->is2D() );
    for ( int idx=0; idx<ps_.size(); idx++ )
    {
	Pick::Location pl( ps_[idx] );
	DataPointSet::DataRow dtrow( DataPointSet::Pos( pl.pos ) );
	locations.addRow( dtrow );
    }

    locations.dataChanged();
    if ( !getAndCheckAttribSelection( locations ) )
	return false;

    bool success = extractDipOrAngl( locations );
    if ( !success ) 
	mErrRet( "Cannot calculate attributes at these positions" );

    TypeSet<float> thetas, phis;
    //remark: removed possibility of variable vector length (radius = 1) 
    for ( DataPointSet::RowID rid=0; rid<locations.size(); rid++ )
    {
	float phi = 0;
	float theta = 0;
	if ( usesteering_ )
	{
	    const float inldip = locations.value( 0, rid );
	    const float crldip = locations.value( 1, rid );
	    if ( !mIsUdf(inldip) && !mIsUdf(crldip) )
	    {
		phi = calcPhi( inldip, crldip );
		theta = calcTheta( inldip, crldip );
	    }
	}
	else
	{
	    phi = locations.value( 0, rid ) * M_PI / 180;
	    theta = locations.value( 1, rid ) * M_PI / 180;
	    if ( !mIsUdf(phi) && !mIsUdf(theta) )
	    {
		wrapPhi( phi );
		wrapTheta( theta );
	    }
	    else
	    { phi = 0; theta = 0; }
	}

	thetas += theta;
	phis += phi;
    }

    for ( int idx=0; idx<phis.size(); idx++ )
	ps_[idx].dir = Sphere( 1, thetas[idx], phis[idx] );
 
    ps_.disp_.markertype_ = MarkerStyle3D::Arrow;
    Pick::Mgr().reportChange( this, ps_ );
    Pick::Mgr().reportDispChange( this, ps_ );
    return true;
}


#define mSetColDef( nr ) \
    const Desc* tmpdesc##nr = createdset_->getDesc( ids[nr] );\
    if ( tmpdesc##nr ) \
    { \
	BufferString tmpdefstr##nr; \
	tmpdesc##nr->getDefStr( tmpdefstr##nr ); \
	FileMultiString fms( tmpdefstr##nr ); \
	fms += createdset_->getID(*tmpdesc##nr).asInt(); \
	loc.colDef(nr).ref_ = fms;\
    }

bool uiSetPickDirs::getAndCheckAttribSelection( DataPointSet& loc )
{
    TypeSet<DescID> nlaids;
    if ( !getNLAIds(nlaids) )
	return false;

    TypeSet<DescID> ids;
    if ( usesteering_ )
    {
	if ( createdset_ )
	    steerfld_->setDescSet( createdset_ );

	const DescID inldipid = steerfld_->inlDipID();
	if ( !inldipid.isValid() ) mErrRet( "Cannot read Steering Cube" )

	ids += inldipid;
	ids += steerfld_->crlDipID();
    }
    else
    {
	const DescID phiid = getAttribID( phifld_, nlaids );
	if ( !phiid.isValid() )
	    mErrRet( "No valid attribute selected for Phi" )
	ids += phiid;
	const DescID thetaid = getAttribID( thetafld_, nlaids );
	if ( !thetaid.isValid() )
	    mErrRet( "No valid attribute selected for Theta" );
	ids += thetaid;
    }

    if ( !createdset_ )
	createdset_ = ads_->isEmpty() ? new DescSet( ads_->is2D() )
    				      : new DescSet( *ads_ );

    if ( !createdset_->getDesc( ids[0] ) && usesteering_ )
    {
	createSteeringDesc( 0, ids[0] );
	createSteeringDesc( 1, ids[1] );
    }

    mSetColDef(0)
    mSetColDef(1)
  
    return true;
}


bool uiSetPickDirs::getNLAIds( TypeSet<DescID>& ids )
{
    if ( !nlamdl_ ) return true;

    EngineMan aem;
    aem.setNLAModel( nlamdl_ );

    SelInfo selinfo( 0, nlamdl_ );
    const int nrnlaouts = selinfo.nlaoutnms.size();
    for ( int idx=0; idx<nrnlaouts; idx++ )
    {
	if ( !idx )
	{
	    SelSpec tmpspec( selinfo.nlaoutnms.get( idx ) );
	    tmpspec.setIDFromRef(nlamdl_);
	    aem.setAttribSpec( tmpspec );
	    DescID nlaid(-1, true);
	    BufferString errmsg;
	    createdset_ = aem.createNLAADS( nlaid, errmsg, ads_ );
	    if ( errmsg.size() ) mErrRet( errmsg );
	    ids += nlaid;
	    continue;
	}

	Desc* desc0 = createdset_->getDesc( ids[0] );
	if ( !desc0 ) continue;
	Desc* ad = new Desc( *desc0 );
	ad->setDescSet( createdset_ );
	BufferString usrref( ad->userRef() ); usrref += "__"; usrref += idx;
	ad->setUserRef( usrref );
	ad->selectOutput( idx );
	DescID nlaid  = createdset_->addDesc( ad );
	ids += nlaid;
    }

    return true;
}


DescID uiSetPickDirs::getAttribID( uiAttrSel* attrfld,
				   const TypeSet<DescID>& nlaids )
{
    const DescID attribid = attrfld->attribID();
    const int outputnr = attrfld->outputNr();
    DescID newid( DescID::undef() );
    if ( attribid.isValid() )
	newid = attribid;
    else if ( outputnr >= 0 && nlaids.size() > outputnr )
	newid = nlaids[outputnr];

    return newid;
}


bool uiSetPickDirs::extractDipOrAngl( DataPointSet& locations )
{
    BufferString errmsg; Attrib::EngineMan aem;
    MouseCursorManager::setOverride( MouseCursor::Wait );
    PtrMan<Executor> tabextr =
		    aem.getTableExtractor( locations, *createdset_, errmsg );
    MouseCursorManager::restoreOverride();
    if ( !errmsg.isEmpty() ) mErrRet(errmsg)

    uiTaskRunner taskrunner( this );
    if ( !taskrunner.execute(*tabextr) )
	return false;

    return true;
}


float uiSetPickDirs::calcPhi( float inldip, float crldip )
{
    const float azi = atan2( inldip, crldip );

    const RCol2Coord& b2c = SI().binID2Coord();
    const float xcrl = b2c.getTransform(true).c;
    float ycrl = b2c.getTransform(false).c;

    const float angN = atan2( xcrl, ycrl );

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
    const float poldip = Math::Sqrt( inldip*inldip + crldip*crldip );
    
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


void uiSetPickDirs::createSteeringDesc( int dipnr, const DescID& did )
{
    Desc* desc = PF().createDescCopy( StorageProvider::attribName() );
    desc->setHidden( true );
    desc->selectOutput( dipnr );
    LineKey linekey( steerctio_->ioobj->key() );
    if ( createdset_->is2D() )
	linekey.setAttrName( "Steering" );
    ValParam* keypar = desc->getValParam( StorageProvider::keyStr() );
    keypar->setValue( linekey );

    BufferString userref = steerctio_->ioobj->name();
    userref += dipnr==0 ? "_inline_dip" : "_crline_dip";
    desc->setUserRef( userref );
    desc->updateParams();

    createdset_->addDesc( desc, did );
}
