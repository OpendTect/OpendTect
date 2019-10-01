/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Nov 2003
________________________________________________________________________

-*/


#include "uisetpickdirs.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribengman.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribstorprovider.h"
#include "attribsel.h"
#include "ioobjctxt.h"
#include "datacoldef.h"
#include "datapointset.h"
#include "executor.h"
#include "mousecursor.h"
#include "pickset.h"
#include "posidxpair2coord.h"
#include "seistrctr.h"
#include "separstr.h"
#include "survinfo.h"
#include "undefval.h"
#include "trigonometry.h"

#include "uiattrsel.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uisteeringsel.h"
#include "uitaskrunner.h"
#include "od_helpids.h"

#include <math.h>


using namespace Attrib;

uiSetPickDirs::uiSetPickDirs( uiParent* p, Pick::Set& s,
			      const DescSet* a, const NLAModel* n, float vel )
	: uiDialog(p,uiDialog::Setup(tr("Add direction to PointSet"),
				     tr("Specify directions for picks"),
				     mODHelpKey(mSetPickDirsHelpID) ))
	, ps_( s )
	, ads_( a ? new DescSet(*a) : new DescSet(false) )
	, nlamdl_( n )
	, dirinpfld_( 0 )
	, phifld_( 0 )
	, thetafld_( 0 )
	, steerfld_( 0 )
	, usesteering_( true )
	, createdset_( 0 )
	, velocity_(vel)
{
    ps_.ref();
    const bool is2d = ads_ ? ads_->is2D() : false;

    const SelInfo attrselinfo( ads_, DescID(), nlamdl_, is2d );
    if ( attrselinfo.ioobjids_.size() == 0 )
    {
	new uiLabel( this, tr("Please import a seismic cube first") );
	return;
    }

    dirinpfld_ = new uiGenInput( this, tr("Direction from"),
		    BoolInpSpec(true,uiStrings::sSteeringCube(),
				uiStrings::sAttribute(mPlural)));
    dirinpfld_->valuechanged.notify( mCB(this,uiSetPickDirs,dirinpSel) );
    steerfld_ = new uiSteerAttrSel( this, is2d );
    steerfld_->attach( alignedBelow, dirinpfld_ );

    uiAttrSelData asd( *ads_ );
    asd.nlamodel_ = nlamdl_;
    phifld_ = new uiAttrSel( this, asd,
			     tr("Azimuth Angle ~ North (phi=[0-360])") );
    if ( dirinpfld_ )
	phifld_->attach( alignedBelow, dirinpfld_ );
    thetafld_ = new uiAttrSel( this, asd,
				tr("Dip Angle ~ Horizontal (theta=[-90-90])") );
    thetafld_->attach( alignedBelow, phifld_ );

    postFinalise().notify( mCB(this,uiSetPickDirs,dirinpSel) );
}


uiSetPickDirs::~uiSetPickDirs()
{
    ps_.unRef();
    delete ads_;
    delete createdset_;
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

#define mAddColDef( dir, fld ) \
{ \
    BufferString coldefnm##fld = usesteering_ ? steerfld_->getInput() \
					      : fld->getAttrName(); \
    if ( usesteering_ ) \
    { coldefnm##fld += "_"; \
      coldefnm##fld += dir; coldefnm##fld += "_dip"; } \
\
    dcds += new DataColDef( coldefnm##fld ); \
}

bool uiSetPickDirs::acceptOK()
{
    if ( !thetafld_ )
	return true;

    if ( usesteering_ && !*steerfld_->getInput() )
	mErrRet( uiStrings::phrSelect(toUiString("SteeringCube")) )
    if ( !usesteering_
	&& ( !phifld_->haveSelection() || !thetafld_->haveSelection() ) )
	mErrRet(uiStrings::phrSelect(tr("input attributes for Phi and Theta")))

    TypeSet<DataPointSet::DataRow> pts;
    ObjectSet<DataColDef> dcds;
    mAddColDef( "inline", phifld_ )
    mAddColDef( "crline", thetafld_ )

    TypeSet<DataPointSet::Pos> positions;
    TypeSet<Pick::Set::LocID> locids;
    RefMan<DataPointSet> dps = new DataPointSet( pts, dcds, ads_->is2D() );
    Pick::SetIter psiter( ps_ );
    while ( psiter.next() )
    {
	DataPointSet::DataRow dtrow( DataPointSet::Pos(psiter.get().pos()) );
	dps->addRow( dtrow );
	positions += dtrow.pos_;
	locids.add( psiter.ID() );
    }
    psiter.retire();

    dps->dataChanged();
    if ( !getAndCheckAttribSelection( *dps ) )
	return false;

    bool success = extractDipOrAngl( *dps );
    if ( !success )
	mErrRet( tr("Cannot calculate attributes at these positions") );

    //remark: removed possibility of variable vector length (radius = 1)
    for ( int idx=0; idx<positions.size(); idx++ )
    {
	const Pick::Set::LocID locid( locids[idx] );
	float phi = 0;
	float theta = 0;
	DataPointSet::RowID rid = dps->find( positions[idx] );
	if ( rid < 0 )
	    continue;

	float inldip = dps->value( 0, rid )/2;
	float crldip = dps->value( 1, rid )/2;

	if ( mIsUdf(inldip) || mIsUdf(crldip) )
	    inldip = crldip = 0;

	SeparString dipvaluetext;
	dipvaluetext += ::toString( inldip );
	dipvaluetext += ::toString( crldip );
	Pick::Location pl( ps_.get(locid) );
	pl.setKeyedText( "Dip", dipvaluetext );

	if ( usesteering_ )
	{
	    phi = calcPhi( inldip, crldip );
	    theta = calcTheta( inldip, crldip );
	}
	else
	{
	    phi = Math::toRadians( (float) dps->value( 0, rid ) );
	    theta = Math::toRadians( (float) dps->value( 1, rid ) );
	    if ( !mIsUdf(phi) && !mIsUdf(theta) )
	    {
		wrapPhi( phi );
		wrapTheta( theta );
	    }
	    else
	    { phi = 0; theta = 0; }
	}
	pl.setDir( Sphere(1,theta,phi) );

	ps_.set( locid, pl );
    }

    OD::MarkerStyle3D mstyle = ps_.markerStyle();
    mstyle.type_ = OD::MarkerStyle3D::Plane;
    ps_.setMarkerStyle( mstyle );

    return true;
}


#define mSetColDef( nr ) \
    const Desc* tmpdesc##nr = createdset_->getDesc( ids[nr] );\
    if ( tmpdesc##nr ) \
    { \
	BufferString tmpdefstr##nr; \
	tmpdesc##nr->getDefStr( tmpdefstr##nr ); \
	FileMultiString fms( tmpdefstr##nr ); \
	fms += createdset_->getID(*tmpdesc##nr).getI(); \
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
	if ( !inldipid.isValid() ) mErrRet(
		    uiStrings::phrCannotRead(uiStrings::sSteeringCube()) )

	ids += inldipid;
	ids += steerfld_->crlDipID();
    }
    else
    {
	const DescID phiid = getAttribID( phifld_, nlaids );
	if ( !phiid.isValid() )
	    mErrRet( tr("No valid attribute selected for Phi") )
	ids += phiid;
	const DescID thetaid = getAttribID( thetafld_, nlaids );
	if ( !thetaid.isValid() )
	    mErrRet( tr("No valid attribute selected for Theta") );
	ids += thetaid;
    }

    if ( !createdset_ )
	createdset_ = ads_->isEmpty() || usesteering_
				? new DescSet(ads_->is2D() )
				: new DescSet( *ads_ );

    if ( !createdset_->getDesc( ids[0] ) && usesteering_ )
	*createdset_ = Attrib::DescSet::global( ads_->is2D() );

    mSetColDef(0)
    mSetColDef(1)

    return true;
}


bool uiSetPickDirs::getNLAIds( TypeSet<DescID>& ids )
{
    ids.setEmpty();
    if ( !nlamdl_ )
	return true;

    EngineMan aem;
    aem.setNLAModel( nlamdl_ );

    SelInfo selinfo( 0, DescID(), nlamdl_ );
    const int nrnlaouts = selinfo.nlaoutnms_.size();
    for ( int idx=0; idx<nrnlaouts; idx++ )
    {
	DescID nlaid;
	if ( ids.isEmpty() )
	{
	    SelSpec tmpspec( selinfo.nlaoutnms_.get(idx) );
	    tmpspec.setIDFromRef( *nlamdl_ );
	    aem.setAttribSpec( tmpspec );
	    uiRetVal uirv;
	    createdset_ = aem.createNLAADS( nlaid, uirv, ads_ );
	    if ( !uirv.isOK() )
		mErrRet( uirv );
	}
	else
	{
	    Desc* desc0 = createdset_->getDesc( ids[0] );
	    if ( !desc0 )
		continue;
	    Desc* ad = new Desc( *desc0 );
	    ad->setDescSet( createdset_ );
	    BufferString usrref( ad->userRef() ); usrref += "__"; usrref += idx;
	    ad->setUserRef( usrref );
	    ad->selectOutput( idx );
	    nlaid  = createdset_->addDesc( ad );
	}
	ids += nlaid;
    }

    return true;
}


DescID uiSetPickDirs::getAttribID( uiAttrSel* attrfld,
				   const TypeSet<DescID>& nlaids )
{
    const DescID attribid = attrfld->attribID();
    const int outputnr = attrfld->outputNr();
    DescID newid;
    if ( attribid.isValid() )
	newid = attribid;
    else if ( outputnr >= 0 && nlaids.size() > outputnr )
	newid = nlaids[outputnr];

    return newid;
}


bool uiSetPickDirs::extractDipOrAngl( DataPointSet& locations )
{
    uiRetVal uirv; Attrib::EngineMan aem;
    MouseCursorManager::setOverride( MouseCursor::Wait );
    PtrMan<Executor> tabextr =
		    aem.getTableExtractor( locations, *createdset_, uirv );
    MouseCursorManager::restoreOverride();
    if ( !uirv.isOK() )
	mErrRet(uirv)

    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, *tabextr ) )
	return false;

    return true;
}


float uiSetPickDirs::calcPhi( float inldip, float crldip )
{
    const float azi = Math::Atan2( inldip, crldip );

    const Pos::IdxPair2Coord& b2c = SI().binID2Coord();
    const double xcrl = b2c.getTransform(true).c;
    double ycrl = b2c.getTransform(false).c;

    const float angN = mCast(float, Math::Atan2( xcrl, ycrl ) );

    float phi;
    if ( SI().isRightHandSystem() )
	phi = angN - azi;
    else
	phi = azi + angN;

    if ( phi < 0 ) phi += M_2PIf;
    return phi;
}


float uiSetPickDirs::calcTheta( float inldip, float crldip )
{
    const float poldip = Math::Sqrt( inldip*inldip + crldip*crldip );

    float theta = (float) atan( poldip * velocity_ * 1e-6 );
    return theta;
}


void uiSetPickDirs::wrapPhi( float& phi )
{
    int nrcycles = (int)( phi / M_2PIf );
    phi -= (float) ( nrcycles * M_2PIf );
}


void uiSetPickDirs::wrapTheta( float& theta )
{
    float val = tan( theta );
    if ( val == val )
	theta = atan( val );
    else
	theta = M_PI_2f;
}


void uiSetPickDirs::createSteeringDesc( int dipnr, const DescID& did )
{
    Desc* desc = PF().createDescCopy( StorageProvider::attribName() );
    desc->setIsHidden( true );
    desc->selectOutput( dipnr );
    ValParam* keypar = desc->getValParam( StorageProvider::keyStr() );
    keypar->setValue( steerfld_->ioobj(true)->key().toString() );

    BufferString userref = steerfld_->ioobj(true)->name();
    userref += dipnr==0 ? "_inline_dip" : "_crline_dip";
    desc->setUserRef( userref );
    desc->updateParams();

    createdset_->addDesc( desc, did );
}
