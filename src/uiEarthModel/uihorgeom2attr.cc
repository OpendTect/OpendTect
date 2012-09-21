/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2011
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uihorgeom2attr.h"

#include "uigeninput.h"
#include "uitaskrunner.h"
#include "uimsg.h"

#include "emhorizon3d.h"
#include "executor.h"
#include "datapointset.h"
#include "posvecdataset.h"
#include "datacoldef.h"
#include "emsurfaceauxdata.h"
#include "emmanager.h"
#include "emioobjinfo.h"
#include "survinfo.h"

#define mAddMSFld(txt,att) \
    if ( SI().zIsTime() ) \
    { \
	msfld_ = new uiGenInput( this, txt, BoolInpSpec(true,"ms","s") ); \
	msfld_->attach( alignedBelow, att ); \
    }
#define mGetZFac(valifms) \
    const float zfac = msfld_ && msfld_->getBoolValue() ? valifms : 1


uiHorGeom2Attr::uiHorGeom2Attr( uiParent* p, EM::Horizon3D& hor )
    : uiGetObjectName(p, Setup("Store Z values as attribute",
				getItems(hor)).inptxt("Attribute name") )
    , hor_(hor)
    , msfld_(0)
{
    hor_.ref();
    setHelpID( "104.4.8" );

    mAddMSFld("Store in",inpfld_)
}


uiHorGeom2Attr::~uiHorGeom2Attr()
{
    delete itmnms_;
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
    if ( !uiGetObjectName::acceptOK(cb) )
	return false;

    int auxidx = hor_.auxdata.auxDataIndex( text() );
    if ( auxidx >= 0 )
	hor_.auxdata.removeAuxData( auxidx );
    auxidx = hor_.auxdata.addAuxData( text() );

    mGetZFac( 1000 );

    for ( EM::SectionID isect=0; isect<hor_.nrSections(); isect++ )
    {
	EM::EMObjectIterator* iter = hor_.createIterator( isect );
	while ( true )
	{
	    const EM::PosID pid = iter->next();
	    if ( pid.objectID() == -1 )
		break;
	    if ( !hor_.geometry().isNodeOK(pid) )
		continue;

	    const float zval = (float) ( hor_.getPos(pid).z * zfac );
	    hor_.auxdata.setAuxDataVal( auxidx, pid, zval );
	}
	delete iter;
    }

    PtrMan<Executor> saver = hor_.auxdata.auxDataSaver( auxidx, true );
    uiTaskRunner tr( this );
    return tr.execute( *saver );
}


uiHorAttr2Geom::uiHorAttr2Geom( uiParent* p, EM::Horizon3D& hor,
       				const DataPointSet& dps, int colid )
    : uiDialog(p, Setup("Set horizon Z values",
		  BufferString("Set Z values from '",
		      dps.dataSet().colDef(colid).name_,"'"),"104.4.7"))
    , hor_(hor)
    , dps_(dps)
    , colid_(colid-dps.nrFixedCols())
    , msfld_(0)
{
    hor_.ref();

    isdeltafld_ = new uiGenInput( this, "Values are",
			  BoolInpSpec(false,"Relative (deltas)","Absolute") );
    mAddMSFld("Units",isdeltafld_)
}


class uiHorAttr2GeomExec : public Executor
{
public:

uiHorAttr2GeomExec( EM::Horizon3D& h, const DataPointSet& dps,
		    int colid, float zfac, bool isdel )
    : Executor("Horizon geometry from attribute")
    , hor_(h)
    , dps_(dps)
    , it_(h.createIterator(0))
    , stepnr_(0)
    , colid_(colid)
    , isdelta_(isdel)
    , zfac_(zfac)
{
    totnr_ = it_->approximateSize();
}

~uiHorAttr2GeomExec()
{
    delete it_;
}

const char* message() const	{ return "Setting Z values"; }
const char* nrDoneText() const	{ return "Nodes done"; }
od_int64 nrDone() const		{ return stepnr_ * 1000; }
od_int64 totalNr() const	{ return totnr_; }

int nextStep()
{
    for ( int idx=0; idx<1000; idx++ )
    {
	const EM::PosID pid = it_->next();
	if ( pid.objectID() == -1 )
	    return Finished();
 
	const BinID bid = pid.getRowCol();
	DataPointSet::RowID rid = dps_.findFirst( bid );
	Coord3 crd = hor_.getPos( pid );
	if ( rid < 0 )
	{
	    if ( !isdelta_ )
		crd.z = mUdf(float);
	}
	else
	{
	    float newz = dps_.value( colid_, rid );
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
	if ( mIsUdf(crd.z) )
	    hor_.unSetPos( pid, false );
	else
	    hor_.setPos( pid, crd, false );
    }
    stepnr_++;
    return MoreToDo();
}

    EM::Horizon3D&		hor_;
    const DataPointSet&		dps_;
    EM::EMObjectIterator*	it_;
    const int			colid_;
    od_int64			stepnr_;
    od_int64			totnr_;
    bool			isdelta_;
    const float			zfac_;

};


bool uiHorAttr2Geom::acceptOK( CallBacker* cb )
{
    mGetZFac( 0.001f );
    const bool isdelta = isdeltafld_->getBoolValue();

    uiHorAttr2GeomExec exec( hor_, dps_, colid_, zfac, isdelta );
    uiTaskRunner tr( this );
    return tr.execute( exec );
}
