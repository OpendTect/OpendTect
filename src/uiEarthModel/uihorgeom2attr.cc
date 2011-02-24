/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Feb 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uihorgeom2attr.cc,v 1.1 2011-02-24 14:56:43 cvsbert Exp $";

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

    mAddMSFld("Store in",inpfld_)
}


uiHorGeom2Attr::~uiHorGeom2Attr()
{
    delete itmnms_;
}


BufferStringSet& uiHorGeom2Attr::getItems( const EM::Horizon3D& hor )
{
    itmnms_ = new BufferStringSet;
    for ( int idx=0; idx<hor.auxdata.nrAuxData(); idx++ )
    {
	const char* nm = hor.auxdata.auxDataName( idx );
	if ( nm )
	    itmnms_->add( nm );

    }
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

	    const float zval = hor_.getPos(pid).z * zfac;
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
		      dps.dataSet().colDef(colid).name_,"'"),mTODOHelpID))
    , hor_(hor)
    , dps_(dps)
    , colid_(colid)
    , msfld_(0)
{
    hor_.ref();

    isdeltafld_ = new uiGenInput( this, "Values are",
			  BoolInpSpec(false,"Relative (deltas)","Absolute") );
    mAddMSFld("Units",isdeltafld_)
}


bool uiHorAttr2Geom::acceptOK( CallBacker* cb )
{
    mGetZFac( 0.001 );
    const bool isdelta = isdeltafld_->getBoolValue();

    EM::EMObjectIterator* iter = hor_.createIterator( 0 );
    while ( true )
    {
	const EM::PosID pid = iter->next();
	if ( pid.objectID() == -1 )
	    break;
 
	const BinID bid( pid.subID() );
	DataPointSet::RowID rid = dps_.findFirst( bid );
	Coord3 crd = hor_.getPos( pid );
	if ( rid < 0 )
	{
	    if ( !isdelta )
		crd.z = mUdf(float);
	}
	else
	{
	    float newz = dps_.value( colid_, rid );
	    if ( !mIsUdf(newz) )
		newz *= zfac;
	    if ( isdelta )
		crd.z += newz;
	    else
		crd.z = newz;
	}
	hor_.setPos( pid, crd, false );
    }
    delete iter;

    return true;
}
