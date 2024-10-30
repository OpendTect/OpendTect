/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "attribsetcreator.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "ctxtioobj.h"
#include "ioobj.h"
#include "iopar.h"
#include "seistrctr.h"

#include "uidialog.h"
#include "uiioobjsel.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseparator.h"
#include "uistrings.h"


using namespace Attrib;
MultiID storhint_;


class uiSelExternalAttrInps : public uiDialog
{ mODTextTranslationClass(uiSelExternalAttrInps);
public:
uiSelExternalAttrInps( uiParent* p, DescSet* ads,
		const BufferStringSet& indirinps,
		const BufferStringSet& dirinps )
    : uiDialog(p,uiDialog::Setup(tr("Specify inputs"),
		     tr("Network without attributes: definitions"),
		     mNoHelpKey))
    , attrset_(ads)
    , nrindir_(indirinps.size())
{
    uiGroup* indirgrp = nullptr;
    if ( nrindir_ )
    {
	indirgrp = new uiGroup( this, "Indirect attribs" );
	const uiString txt = indirinps.size() > 1
		? uiStrings::phrSpecify(tr("the input for these attributes"))
		: uiStrings::phrSpecify(tr("the input for this attribute"));
	mkGrp( indirgrp, txt, indirinps );
    }

    uiSeparator* sep = nullptr;
    if ( indirgrp && dirinps.size() )
    {
	sep = new uiSeparator( this, "hor sep" );
	sep->attach( stretchedBelow, indirgrp, -2 );
    }

    if ( dirinps.size() )
    {
	auto* dirgrp = new uiGroup( this, "Direct attribs" );
	const uiString txt = dirinps.size() > 1
		? uiStrings::phrSpecify(
			tr("the cubes that contain these attributes"))
		: uiStrings::phrSpecify(
			tr("the cube that contains this attribute"));
	mkGrp( dirgrp, txt, dirinps );
	if ( indirgrp )
	    dirgrp->attach( alignedBelow, indirgrp );
	if ( sep )
	    dirgrp->attach( ensureBelow, sep );
    }
}


~uiSelExternalAttrInps()
{
    for ( int idx=0; idx<sels_.size(); idx++ )
    {
	delete sels_[idx]->ctxtIOObj().ioobj_;
	delete &sels_[idx]->ctxtIOObj();
    }
}


void mkGrp( uiGroup* mkgrp, const uiString& lbltxt,
	    const BufferStringSet& inps )
{
    auto* lbl = new uiLabel( mkgrp, lbltxt );
    auto* fldgrp = new uiGroup( mkgrp, "sels" );
    auto* curgrp = new uiGroup( fldgrp, "sels1" );
    mkgrp->setHAlignObj( fldgrp );
    fldgrp->setHAlignObj( curgrp );
    fldgrp->attach( centeredBelow, lbl );

    uiIOObjSel* prevsel = nullptr;
    const int maxnrselinrow = 9;
    bool neednewgrp = false;

    for ( int idx=0; idx<inps.size(); idx++ )
    {
	IOObjContext ctxt = mIOObjContext( SeisTrc );
	ctxt.forread_ = true;
	if ( neednewgrp )
	{
	    auto* newgrp = new uiGroup( fldgrp, "selsN" );
	    newgrp->attach( rightOf, curgrp );
	    curgrp = newgrp;
	}

	auto* newsel = new uiIOObjSel( curgrp, ctxt, toUiString(*inps[idx]) );
	if ( neednewgrp )
	    curgrp->setHAlignObj( newsel );
	else if ( prevsel )
	    newsel->attach( alignedBelow, prevsel );

	neednewgrp = !((idx+1) % maxnrselinrow);
	newsel->selectionDone.notify( mCB(this,uiSelExternalAttrInps,cubeSel) );
	sels_ += newsel;
	prevsel = newsel;
    }
}


void cubeSel( CallBacker* cb )
{
    mDynamicCastGet(uiIOObjSel*,cursel,cb)
    if ( !cursel ) { pErrMsg("Huh"); return; }

    cursel->commitInput();
    const IOObj* ioobj = cursel->ctxtIOObj().ioobj_;
    if ( !ioobj ) return;
    int curidx = sels_.indexOf( cursel );
    if ( curidx >= nrindir_ )
	return;

    IOPar iopar;
    cursel->updateHistory( iopar );
    for ( int idx=0; idx<nrindir_; idx++ )
    {
	uiIOObjSel& sel = *sels_[idx];
	if ( &sel == cursel ) continue;
	sel.getHistory( iopar );
	if ( !sel.ctxtIOObj().ioobj_ )
	{
	    sel.ctxtIOObj().setObj( ioobj->clone() );
	    sel.updateInput();
	}
    }
}


bool acceptOK( CallBacker* ) override
{
    for ( int isel=0; isel<sels_.size(); isel++ )
    {
	uiIOObjSel& sel = *sels_[isel];
	sel.commitInput();
	const IOObj* ioobj = sel.ctxtIOObj().ioobj_;
	if ( !ioobj )
	{
	    uiMSG().error(
		    tr("Please supply input for '%1'").arg(sel.labelText()) );
	    return false;
	}

	const DescID descid =
		attrset_->getID( sel.labelText().getFullString(), true );
	if ( !descid.isValid() )
	{
	    const uiString msg =
		tr( "There is a problem importing '%1'.\n"
		    "Please contact support for possible workarounds.").
		    arg( sel.labelText() );

	    uiMSG().error( msg );
	    return false;
	}

	Desc& ad = *attrset_->getDesc( descid );
	if ( ad.isStored() )
	{
//	    ad.setDefStr( ioobj->key(), false );
	}
	else
	{
	    const DescID inpid = attrset_->getStoredID( ioobj->key(), 0, true );
	    if ( !inpid.isValid() ) return false;

	    for ( int iinp=0; iinp<ad.nrInputs(); iinp++ )
		ad.setInput( iinp, attrset_->getDesc(inpid).ptr() );
	}
    }

    return true;
}


protected:

    DescSet*			attrset_;
    ObjectSet<uiIOObjSel>	sels_;
    int				nrindir_;
};


AttributeSetCreator::AttributeSetCreator( uiParent* p_,
					  const BufferStringSet& inps_,
					  DescSet* ads )
    : prnt_(p_)
    , attrset_(ads)
{
    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	const BufferString& uref = *inps_[idx];
	Desc* ad = getDesc( uref );
	if ( !ad )
	    { attrset_->removeAll( false ); attrset_ = nullptr; return; }

	if ( ad->isStored() )
	    directs_ += new BufferString( uref );
	else
	    indirects_ += new BufferString( uref );
    }
}


AttributeSetCreator::~AttributeSetCreator()
{}


bool AttributeSetCreator::create()
{
    const int nrdescs = attrset_ ? attrset_->size() : 0;
    if ( nrdescs < 1 )
    {
	uiMSG().error(tr("The attributes cannot be converted"));
	return false;
    }

    ConstRefMan<Desc> stored;
    for ( int idx=0; idx<nrdescs; idx++ )
    {
	ConstRefMan<Desc> desc = attrset_->desc( idx );
	if ( desc->isHidden() )
	   continue;
	if ( desc->isStored() )
	{
	    if ( desc->getStoredID() == storhint_ )
	       stored = desc;
	}
    }

    if ( !stored && !storhint_.isUdf() )
    {
	DescID did = attrset_->createStoredDesc( storhint_, 0,
						BufferString("") );
	stored = attrset_->getDesc( did );
    }

    if ( !stored )
    {
	uiSelExternalAttrInps dlg( prnt_, attrset_, indirects_, directs_ );
	return dlg.go();
    }
    else
    {
	for ( int idx=0; idx<nrdescs; idx++ )
	{
	    RefMan<Desc> desc = attrset_->desc( idx );
	    if ( desc->isHidden() || desc->isStored() )
	       continue;

	    desc->setInput( 0, stored.ptr() );
	}
    }

    return true;
}


static void addGate( BufferString& defstr, const char* extdesc )
{
    BufferString extstr( extdesc );
    char* ptr = extstr.find( '[' );
    BufferString gatestr( ptr ? ptr : "[-32,32]" );
    ptr = gatestr.find( ']' );
    if ( ptr ) *(ptr+1) = '\0';
    defstr += " gate=";
    defstr += gatestr;
}


Desc* AttributeSetCreator::getDesc( const char* extdesc )
{
    const StringView fsextdesc( extdesc );
    if ( fsextdesc.isEmpty() )
	return nullptr;

    BufferString defstr;
    if ( fsextdesc.startsWith("Energy",OD::CaseInsensitive) )
    {
	defstr = "Energy";
	addGate( defstr, extdesc );
	defstr += "output=0";
    }
    else if ( fsextdesc.startsWith("Reference time",OD::CaseInsensitive) )
    {
	defstr = "Reference output=2";
    }
    else if ( fsextdesc.startsWith("Sample",OD::CaseInsensitive) )
    {
	BufferString offs = extdesc + 7;
	offs.trimBlanks();
	char* ptr = offs.getCStr();
	ptr = offs.find( ' ' );
	if ( ptr ) *ptr = '\0';
	defstr = "Shift pos=0,0 steering=No time=";
	defstr += offs;
    }
    else if ( fsextdesc.startsWith("Similarity",OD::CaseInsensitive) )
    {
	defstr = "Similarity steering=No";
	addGate( defstr, extdesc );
	BufferString work( extdesc );
	char* pos0ptr = work.find( '=' );
	if ( pos0ptr ) pos0ptr++;
	char* pos1ptr = pos0ptr ? firstOcc( pos0ptr, 'x' ) : 0;
	char* ampptr = pos1ptr ? firstOcc( pos1ptr, '&' ) : 0;

	if ( !pos0ptr || !*pos0ptr || !pos1ptr )
	    defstr += " pos0=-1,-1 pos1=1,1 extension=90";
	else
	{
	    if ( ampptr )
	    {
		defstr += " extension=";
		defstr += ampptr+1;
		*ampptr = '\0';
	    }
	    *pos1ptr++ = '\0';
	    defstr += " pos0="; defstr += pos0ptr;
	    defstr += " pos1="; defstr += pos1ptr;
	}
    }

    if ( defstr.isEmpty() )
	defstr = "Storage id=100010.x";

    BufferString attribname;
    if ( !Desc::getAttribName(defstr,attribname) )
    {
	uiMSG().error(uiStrings::sCantFindAttrName());
	return nullptr;
    }

    RefMan<Desc> desc = PF().createDescCopy( attribname );
    if ( !desc )
    {
	uiMSG().error( DescSet::sFactoryEntryNotFound(attribname) );
	return nullptr;
    }

    if ( !desc->isStored() && !desc->parseDefStr(defstr) )
    {
	uiString err = tr("Cannot parse: %1").arg(defstr);
	uiMSG().error( err );
	return nullptr;
    }

    desc->setUserRef( extdesc );
    attrset_->addDesc( desc.ptr() );
    return desc.ptr();
}


void AttributeSetCreator::setStorageHint( const MultiID& m )
{ storhint_ = m; }
