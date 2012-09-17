/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

static const char* rcsID = "$Id: uiwellposprov.cc,v 1.3 2012/02/24 23:13:20 cvsnanne Exp $";

#include "uiwellposprov.h"

#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "uiselsurvranges.h"
#include "uistepoutsel.h"
#include "uiwellsel.h"

#include "ctxtioobj.h"
#include "cubesampling.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "survinfo.h"
#include "wellposprovider.h"
#include "welltransl.h"


uiWellPosProvGroup::uiWellPosProvGroup( uiParent* p,
					const uiPosProvGroup::Setup& su )
    : uiPosProvGroup(p,su)
    , zrgfld_(0)
{
    wellfld_ = new uiWellParSel( this );

    stepoutfld_ = new uiStepOutSel( this, false, "Extension" );
    stepoutfld_->attach( alignedBelow, wellfld_ );

    zrgfld_ = new uiSelZRange( this, true, false, 0, su.zdomkey_ );
    zrgfld_->attach( alignedBelow, stepoutfld_ );

    setHAlignObj( wellfld_ );
}


uiWellPosProvGroup::~uiWellPosProvGroup()
{
}


#define mErrRet(s) { uiMSG().error(s); return false; }
#define mGetWellKey(k) IOPar::compKey(sKey::Well,Pos::WellProvider3D::k)


void uiWellPosProvGroup::usePar( const IOPar& iop )
{
    wellfld_->usePar( iop );
    BinID so; float zext = 0; bool onlysurfacecoords = true;
    iop.get( mGetWellKey(sKeyInlExt()), so.inl );
    iop.get( mGetWellKey(sKeyCrlExt()), so.crl );
    iop.get( mGetWellKey(sKeyZExt()), zext );
    iop.getYN( mGetWellKey(sKeySurfaceCoords()), onlysurfacecoords );

    stepoutfld_->setBinID( so );
}


bool uiWellPosProvGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type, sKey::Well );
    if ( !wellfld_->nrSel() )
	mErrRet("Please select at least one well")

    wellfld_->fillPar( iop );
    float zext = 0; bool onlysurfacecoords = true;
    const BinID so = stepoutfld_->getBinID();
    iop.set( mGetWellKey(sKeyInlExt()), so.inl );
    iop.set( mGetWellKey(sKeyCrlExt()), so.crl );
    iop.set( mGetWellKey(sKeyZExt()), zext );
    iop.setYN( mGetWellKey(sKeySurfaceCoords()), onlysurfacecoords );

    return true;
}


void uiWellPosProvGroup::getSummary( BufferString& txt ) const
{
    txt += "Around wells";
}


void uiWellPosProvGroup::setExtractionDefaults()
{
}


bool uiWellPosProvGroup::getIDs( TypeSet<MultiID>& ids ) const
{ wellfld_->getSelected( ids ); return ids.size(); }


void uiWellPosProvGroup::getZRange( StepInterval<float>& zrg ) const
{
    zrg = zrgfld_ ? zrgfld_->getRange() : SI().zRange(true);
}


void uiWellPosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Well );
}
