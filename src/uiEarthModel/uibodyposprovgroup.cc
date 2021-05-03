/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Y. Liu
 Date:          Nov 2011
________________________________________________________________________

-*/


#include "uibodyposprovgroup.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uimsg.h"
#include "keystrs.h"
#include "embodytr.h"
#include "emsurfaceposprov.h"
#include "uipossubsel.h"
#include "uistrings.h"


#define mErrRet(s) { uiMSG().error(s); return false; }

uiBodyPosProvGroup::uiBodyPosProvGroup( uiParent* p,
					const uiPosProvGroup::Setup& su )
    : uiPosProvGroup(p,su)
    , ctio_(*mMkCtxtIOObj(EMBody))
{
    inoutbut_ = new uiGenInput(this, uiString::emptyString(),
                               BoolInpSpec(true,tr("Inside"),
                                tr("Outside")) );
    inoutbut_->valuechanged.notify( mCB(this,uiBodyPosProvGroup,ioChg) );
    bodyfld_ = new uiIOObjSel( this, ctio_, uiStrings::sBody() );
    bodyfld_->attach( alignedBelow, inoutbut_ );

    outsidergfld_ = new uiPosSubSel( this, uiPosSubSel::Setup(false,true)
	    .withstep(false).seltxt("Within bounding box") );
    outsidergfld_->attach( alignedBelow, bodyfld_ );
    ioChg( 0 );

    setHAlignObj( bodyfld_ );
}


uiBodyPosProvGroup::~uiBodyPosProvGroup()
{
    delete ctio_.ioobj_; delete &ctio_;
}


void uiBodyPosProvGroup::ioChg( CallBacker* )
{
    outsidergfld_->display( !inoutbut_->getBoolValue() );
}


uiPosProvGroup* uiBodyPosProvGroup::create( uiParent* p,
					    const uiPosProvGroup::Setup& su )
{ return new uiBodyPosProvGroup(p,su); }


void uiBodyPosProvGroup::usePar( const IOPar& iop )
{
    bodyfld_->usePar( iop, sKey::Body() );

    bool useinside;
    iop.getYN( Pos::EMImplicitBodyProvider::sKeyUseInside(), useinside );
    inoutbut_->setValue( useinside );

    if ( !useinside )
    {
	Interval<int> inlrg, crlrg;
	iop.get( Pos::EMImplicitBodyProvider::sKeyBBInlrg(), inlrg );
	iop.get( Pos::EMImplicitBodyProvider::sKeyBBCrlrg(), crlrg );
	Interval<float> zrg;
	iop.get( Pos::EMImplicitBodyProvider::sKeyBBZrg(), zrg );

	TrcKeyZSampling cs(true);
	cs.hsamp_.set( inlrg, crlrg );
	cs.zsamp_.setFrom( zrg );
	outsidergfld_->setInput( cs );
    }

    ioChg( 0 );
}


bool uiBodyPosProvGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), sKey::Body() );
    if ( !bodyfld_->commitInput() || !bodyfld_->fillPar(iop,sKey::Body()) )
	mErrRet(tr("Please select the body"));

    iop.setYN( Pos::EMImplicitBodyProvider::sKeyUseInside(),
	    inoutbut_->getBoolValue() );
    if ( !inoutbut_->getBoolValue() )
    {
	const TrcKeyZSampling& cs = outsidergfld_->envelope();
	iop.set( Pos::EMImplicitBodyProvider::sKeyBBInlrg(),
		cs.hsamp_.inlRange());
	iop.set( Pos::EMImplicitBodyProvider::sKeyBBCrlrg(),
		cs.hsamp_.crlRange());
	iop.set( Pos::EMImplicitBodyProvider::sKeyBBZrg(), cs.zsamp_ );
    }
    return true;
}


void uiBodyPosProvGroup::getSummary( BufferString& txt ) const
{
    txt += inoutbut_->getBoolValue() ? "Inside geo-body" : "Outside geo-body";
}


bool uiBodyPosProvGroup::getID( MultiID& ky ) const
{
    if ( !bodyfld_->commitInput() || !ctio_.ioobj_ )
	return false;

    ky = ctio_.ioobj_->key();
    return true;
}


void uiBodyPosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Body() );
}
