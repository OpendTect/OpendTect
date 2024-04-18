/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uihorizonattrib.h"
#include "horizonattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "emioobjinfo.h"
#include "emsurfacetr.h"
#include "ioman.h"
#include "ioobj.h"
#include "od_helpids.h"
#include "survinfo.h"

#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uibutton.h"
#include "uigeninput.h"
#include "uiioobjsel.h"
#include "uiiosurface.h"
#include "uilabel.h"
#include "uimsg.h"

#include "hiddenparam.h"


using namespace Attrib;

static HiddenParam<uiHorizonAttrib,uiLabel*> hp_nohdlabel(nullptr);

static void getOutputNames( uiStringSet& strs )
{
    uiString zstr;
    if ( SI().zIsTime() )
	zstr = toUiString("%1 (s)").arg( uiStrings::sZ());
    else
	zstr = uiStrings::sZ();

    strs.add( zstr );
    strs.add( uiStrings::sHorizonData() );
}


mInitAttribUI(uiHorizonAttrib,Horizon,"Horizon",sKeyPositionGrp())


uiHorizonAttrib::uiHorizonAttrib( uiParent* p, bool is2d )
    : uiAttrDescEd(p,is2d, mODHelpKey(mHorizonAttribHelpID) )
    , nrouttypes_( 1 )
{
    inpfld_ = createInpFld( is2d );

    horfld_ = new uiHorizonSel( this, is2d, true );
    mAttachCB( horfld_->selectionDone, uiHorizonAttrib::horSel );
    horfld_->attach( alignedBelow, inpfld_ );

    uiStringSet strs;
    getOutputNames( strs );
    typefld_ = new uiGenInput( this, uiStrings::sOutput(),
			       StringListInpSpec(strs) );
    mAttachCB( typefld_->valueChanged, uiHorizonAttrib::typeSel );
    typefld_->attach( alignedBelow, horfld_ );

    isrelbox_ = new uiCheckBox( this, tr("Relative") );
    isrelbox_->attach( rightOf, typefld_ );

    surfdatafld_ = new uiGenInput( this, tr("Select Horizon Data"),
				   StringListInpSpec() );
    surfdatafld_->setElemSzPol( uiObject::Wide );
    surfdatafld_->attach( alignedBelow, typefld_ );

    const uiString nohdstr = tr("No Horizon Data available for this horizon.");
    auto* lbl = new uiLabel( this, nohdstr );
    lbl->attach( alignedBelow, typefld_ );
    hp_nohdlabel.setParam( this, lbl );

    setHAlignObj( inpfld_ );
}


uiHorizonAttrib::~uiHorizonAttrib()
{
    detachAllNotifiers();
    hp_nohdlabel.removeParam( this );
}


bool uiHorizonAttrib::setParameters( const Attrib::Desc& desc )
{
    if ( !desc.attribName().isEqual(Horizon::attribName()) )
	return false;

    mIfGetMultiID(Horizon::sKeyHorID(),horid,horfld_->setInput(horid))

    if ( horfld_->ioobj(true) )
	horSel( nullptr );

    mIfGetEnum(Horizon::sKeyType(), typ, typefld_->setValue(typ));

    mIfGetString( Horizon::sKeySurfDataName(), surfdtnm,
		  surfdatafld_->setValue( !surfdatanms_.isPresent( surfdtnm )
		      ? 0 : surfdatanms_.indexOf(surfdtnm) ) );

    mIfGetBool(Horizon::sKeyRelZ(), isrel, isrelbox_->setChecked(isrel));

    typeSel( nullptr );

    return true;
}


bool uiHorizonAttrib::setInput( const Attrib::Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiHorizonAttrib::getParameters( Attrib::Desc& desc )
{
    if ( !desc.attribName().isEqual(Horizon::attribName()) )
	return false;

    if ( !horfld_->ioobj(true) )
	return false;

    mSetMultiID( Horizon::sKeyHorID(), horfld_->ioobj()->key() );
    int typ = typefld_->getIntValue();
    if ( typ==1 )
    {
	const int surfdataidx = surfdatafld_->getIntValue();
	if ( !surfdatanms_.validIdx(surfdataidx) )
	    typ = 0;
	else
	{
	    const BufferString& surfdatanm = surfdatanms_.get( surfdataidx );
	    mSetString( Horizon::sKeySurfDataName(), surfdatanm.buf() )
	}
    }

    mSetEnum( Horizon::sKeyType(), typ );
    if ( typ==0 )
    {
	mSetBool( Horizon::sKeyRelZ(), isrelbox_->isChecked() )
    }

    return true;
}


bool uiHorizonAttrib::getInput( Desc& desc )
{
    inpfld_->processInput();
    fillInp( inpfld_, desc, 0 );
    return true;
}


void uiHorizonAttrib::horSel( CallBacker* )
{
    const IOObj* ioobj = horfld_->ioobj( true );
    if ( !ioobj )
	return;

    const EM::IOObjInfo eminfo( ioobj->key() );
    if ( !eminfo.isOK() )
    {
	const uiString msg = uiStrings::phrCannotRead( ioobj->uiName() );
	uiMSG().error( msg );
	return;
    }

    BufferStringSet attrnms;
    eminfo.getAttribNames( attrnms );
    surfdatanms_.erase();
    for ( int idx=0; idx<attrnms.size(); idx++ )
	surfdatanms_.add( attrnms.get(idx).buf() );
    surfdatafld_->newSpec( StringListInpSpec(surfdatanms_), 0 );

    typeSel( nullptr );
}


void uiHorizonAttrib::typeSel(CallBacker*)
{
    const bool isz = typefld_->getIntValue() == 0;
    isrelbox_->display( isz );

    const bool hashorizondata = !surfdatanms_.isEmpty();
    surfdatafld_->display( !isz && hashorizondata );
    hp_nohdlabel.getParam(this)->display( !isz && !hashorizondata );
}
