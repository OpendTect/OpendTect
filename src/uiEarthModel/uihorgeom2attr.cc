/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uihorgeom2attr.h"

#include "uigeninput.h"
#include "uitaskrunner.h"
#include "uimsg.h"
#include "uihorsavefieldgrp.h"
#include "uiseparator.h"
#include "uistrings.h"

#include "arraynd.h"
#include "arrayndimpl.h"
#include "emhorizon3d.h"
#include "executor.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "emsurfaceauxdata.h"
#include "emmanager.h"
#include "emioobjinfo.h"
#include "survinfo.h"
#include "od_helpids.h"
#include "ioman.h"
#include "ioobj.h"


#define mAddMSFld(txt,att) \
    if ( SI().zIsTime() ) \
    { \
	msfld_ = new uiGenInput( this, txt, BoolInpSpec(true, \
				uiStrings::sMsec(),uiStrings::sSec()) ); \
	msfld_->attach( alignedBelow, att ); \
    }
#define mGetZFac(valifms) \
    const float zfac = (float) (msfld_ && msfld_->getBoolValue() ? valifms : 1)


uiHorGeom2Attr::uiHorGeom2Attr( uiParent* p, EM::Horizon3D& hor )
    : uiGetObjectName( p, Setup(tr("Store Z values as attribute"),
			       getItems(hor)).inptxt(uiStrings::sAttribName()) )
    , hor_(hor)
    , msfld_(0)
{
    hor_.ref();
    setHelpKey( mODHelpKey(mHorGeom2AttrHelpID) );

    mAddMSFld(tr("Store in"),inpfld_)
}


uiHorGeom2Attr::~uiHorGeom2Attr()
{
    delete itmnms_;
    hor_.unRef();
}


BufferStringSet& uiHorGeom2Attr::getItems( const EM::Horizon3D& hor )
{
    itmnms_ = new BufferStringSet;
    EM::IOObjInfo eminfo( EM::EMM().getMultiID(hor.id()) );
    eminfo.getAttribNames( *itmnms_ );
    return *itmnms_;
}

#define mErrRet(msg) { if ( msg ) uiMSG().error( msg ); return false; }


bool uiHorGeom2Attr::acceptOK( CallBacker* cb )
{
    PtrMan<IOObj> ioobj = IOM().get( hor_.multiID() );
    if ( !ioobj )
    {
	uiMSG().message( tr("Cannot create horizon data on this horizon") );
	return false;
    }

    if ( !uiGetObjectName::acceptOK(cb) )
    {
	uiMSG().message( tr("Please enter attribute name") );
	return false;
    }

    int auxidx = hor_.auxdata.auxDataIndex( text() );
    if ( auxidx >= 0 )
	hor_.auxdata.removeAuxData( auxidx );
    auxidx = hor_.auxdata.addAuxData( text() );

    mGetZFac( 1000 );

    EM::EMObjectIterator* iter = hor_.createIterator();
    while ( true )
    {
	const EM::PosID pid = iter->next();
	if ( !pid.isValid() )
	    break;

	if ( !hor_.geometry().isNodeOK(pid) )
	    continue;

	const float zval = (float) ( hor_.getPos(pid).z * zfac );
	hor_.auxdata.setAuxDataVal( auxidx, pid, zval );
    }
    delete iter;

    PtrMan<Executor> saver = hor_.auxdata.auxDataSaver( auxidx, true );
    if ( !saver )
	return false;

    uiTaskRunner taskrunner( this );
    return TaskRunner::execute( &taskrunner, *saver );
}


uiHorAttr2Geom::uiHorAttr2Geom( uiParent* p, EM::Horizon3D& hor,
       				const DataPointSet& dps, int colid )
    : uiDialog(p, Setup(tr("Set horizon Z values"),(tr("Set Z values from '%1'")
			.arg(toUiString(dps.dataSet().colDef(colid).name_)))
			,mODHelpKey(mHorAttr2GeomHelpID)) )
    , hor_(hor)
    , dps_(&dps)
    , colid_(colid-dps.nrFixedCols())
    , msfld_(0)
{
    hor_.ref();

    isdeltafld_ = new uiGenInput( this, tr("Values are"),
		    BoolInpSpec(false,tr("Relative (deltas)"),tr("Absolute")) );
    mAddMSFld(tr("Units"),isdeltafld_)

    uiSeparator* sep = new uiSeparator( this, "HSep" );
    if ( msfld_ ) sep->attach( stretchedBelow, msfld_ );
    else sep->attach( stretchedBelow, isdeltafld_ );

    savefldgrp_ = new uiHorSaveFieldGrp( this, &hor_, false );
    savefldgrp_->setSaveFieldName( "Save modified horizon" );
    savefldgrp_->attach( alignedBelow, isdeltafld_ );
    savefldgrp_->attach( ensureBelow, sep );
}


uiHorAttr2Geom::~uiHorAttr2Geom()
{
    hor_.unRef();
}


class uiHorAttr2GeomExec : public Executor
{ mODTextTranslationClass(uiHorAttr2GeomExec)
public:

uiHorAttr2GeomExec( EM::Horizon3D& h, const DataPointSet& dps,
		    int colid, float zfac, bool isdel )
    : Executor("Horizon geometry from attribute")
    , hor_(h)
    , dps_(&dps)
    , it_(h.createIterator(0))
    , colid_(colid)
    , stepnr_(0)
    , isdelta_(isdel)
    , zfac_(zfac)
    , horarray_(0)
    , uimsg_(tr("Setting Z values"))
{
    totnr_ = it_->approximateSize();
    hor_.enableGeometryChecks( false );
    hortks_ = hor_.range();
    mDeclareAndTryAlloc( Array2D<float>*, arr,
	    Array2DImpl<float>( hortks_.nrInl(), hortks_.nrCrl() ) );
    if ( arr && !arr->isEmpty() )
    {
	arr->setAll( mUdf(float) );
	horarray_ = arr;
    }

}

~uiHorAttr2GeomExec()
{
    delete it_;
    delete horarray_;
}

uiString uiMessage() const override	{ return uimsg_; }
uiString uiNrDoneText() const override	{ return tr("Nodes done"); }
od_int64 nrDone() const override	{ return stepnr_ * 1000; }
od_int64 totalNr() const override	{ return totnr_; }

int nextStep() override
{
    if ( !horarray_ )
    {
	uimsg_ = uiStrings::phrCannotCreate( tr("Array") );
	return ErrorOccurred();
    }

    for ( int idx=0; idx<1000; idx++ )
    {
	const EM::PosID pid = it_->next();
	if ( !pid.isValid() )
	{
	    fillHorizonArray();
	    return Finished();
	}

	const BinID bid = pid.getRowCol();
	DataPointSet::RowID rid = dps_->findFirst( bid );
	Coord3 crd = hor_.getPos( pid );
	if ( rid < 0 )
	{
	    if ( !isdelta_ )
		crd.z = mUdf(float);
	}
	else
	{
	    float newz = dps_->value( colid_, rid );
	    if ( mIsUdf(newz) && isdelta_ )
		newz = 0;

	    if ( mIsUdf(newz) )
		crd.z = newz;
	    else
	    {
		newz *= zfac_;
		if ( isdelta_ )
		    crd.z += newz;
		else
		    crd.z = newz;
	    }
	}

	const int inlidx = hortks_.inlIdx( bid.inl() );
	const int crlidx = hortks_.crlIdx( bid.crl() );
	if ( !horarray_->info().validPos(inlidx,crlidx) )
	    continue;
	horarray_->set( inlidx, crlidx, mCast(float,crd.z) );
    }
    stepnr_++;
    return MoreToDo();
}


void fillHorizonArray()
{
    Geometry::BinIDSurface* geom = hor_.geometry().geometryElement();
    geom->setArray( hortks_.start_, hortks_.step_, horarray_, true );
    horarray_ = 0;
    hor_.enableGeometryChecks( true );
}

    EM::Horizon3D&		hor_;
    ConstRefMan<DataPointSet>	dps_;
    EM::EMObjectIterator*	it_;
    const int			colid_;
    od_int64			stepnr_;
    od_int64			totnr_;
    bool			isdelta_;
    const float			zfac_;
    Array2D<float>*		horarray_;
    TrcKeySampling		hortks_;
    uiString			uimsg_;

}; // class uiHorAttr2GeomExe


bool uiHorAttr2Geom::acceptOK( CallBacker* )
{
    if ( !dps_ )
	return false;

    mGetZFac( 0.001f );
    const bool isdelta = isdeltafld_->getBoolValue();

    if ( !savefldgrp_->acceptOK(0) )
	return false;

    EM::Horizon3D* usedhor = &hor_;
    if ( !savefldgrp_->overwriteHorizon() )
	mDynamicCast( EM::Horizon3D*, usedhor, savefldgrp_->getNewHorizon() );

    if ( !usedhor )
	return false;

    uiHorAttr2GeomExec exec( *usedhor, *dps_, colid_, zfac, isdelta );
    uiTaskRunner taskrunner( this );
    const bool res = TaskRunner::execute( &taskrunner, exec )
	&& savefldgrp_->saveHorizon();
    return res;
}
