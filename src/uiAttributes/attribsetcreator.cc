/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          April 2003
________________________________________________________________________

-*/
static const char* rcsID = "$Id$";


#include "attribsetcreator.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribfactory.h"
#include "uiseparator.h"
#include "uiioobjsel.h"
#include "seiscbvs.h"
#include "ctxtioobj.h"
#include "uidialog.h"
#include "uilabel.h"
#include "uimsg.h"
#include "ioobj.h"
#include "iopar.h"

using namespace Attrib;
MultiID storhint_;


class uiSelExternalAttrInps : public uiDialog
{
public:
uiSelExternalAttrInps( uiParent* p, DescSet* ads,
		const BufferStringSet& indirinps, 
		const BufferStringSet& dirinps )
    : uiDialog(p,uiDialog::Setup("Specify inputs",
		     "Network without attributes: definitions",
		     "102.0.1"))
    , attrset(ads)
    , nrindir(indirinps.size())
{
    uiGroup* indirgrp = 0;
    if ( nrindir )
    {
	indirgrp = new uiGroup( this, "Indirect attribs" );
	const char* txt = indirinps.size() > 1 ?
			    "Specify the input for these attributes"
			  : "Specify the input for this attribute";
	mkGrp( indirgrp, txt, indirinps );
    }

    uiSeparator* sep = 0;
    if ( indirgrp && dirinps.size() )
    {
	sep = new uiSeparator( this, "hor sep" );
	sep->attach( stretchedBelow, indirgrp, -2 );
    }

    if ( dirinps.size() )
    {
	uiGroup* dirgrp = new uiGroup( this, "Direct attribs" );
	const char* txt = dirinps.size() > 1 ?
		    "Specify the cubes that contain these attributes"
		  : "Specify the cube that contains this attribute";
	mkGrp( dirgrp, txt, dirinps );
	if ( indirgrp )
	    dirgrp->attach( alignedBelow, indirgrp );
	if ( sep )
	    dirgrp->attach( ensureBelow, sep );
    }
}


~uiSelExternalAttrInps()
{
    for ( int idx=0; idx<sels.size(); idx++ )
    {
	delete sels[idx]->ctxtIOObj().ioobj;
	delete &sels[idx]->ctxtIOObj();
    }
}


void mkGrp( uiGroup* mkgrp, const char* lbltxt,
		const BufferStringSet& inps )
{
    uiLabel* lbl = new uiLabel( mkgrp, lbltxt );
    uiGroup* fldgrp = new uiGroup( mkgrp, "sels" );
    uiGroup* curgrp = new uiGroup( fldgrp, "sels1" );
    mkgrp->setHAlignObj( fldgrp );
    fldgrp->setHAlignObj( curgrp );
    fldgrp->attach( centeredBelow, lbl );

    uiIOObjSel* prevsel = 0;
    const int maxnrselinrow = 9;
    bool neednewgrp = false;

    for ( int idx=0; idx<inps.size(); idx++ )
    {
    	CtxtIOObj* newctio = mMkCtxtIOObj(SeisTrc);
	newctio->ctxt.forread = true;
	newctio->ctxt.toselect.allowtransls_ =
	    CBVSSeisTrcTranslator::translKey();
	if ( neednewgrp )
	{
	    uiGroup* newgrp = new uiGroup( fldgrp, "selsN" );
	    newgrp->attach( rightOf, curgrp );
	    curgrp = newgrp;
	}
	uiIOObjSel* newsel = new uiIOObjSel( curgrp, *newctio, *inps[idx] );
	if ( neednewgrp )
	    curgrp->setHAlignObj( newsel );
	else if ( prevsel )
	    newsel->attach( alignedBelow, prevsel );

	neednewgrp = !((idx+1) % maxnrselinrow);
	newsel->selectionDone.notify( mCB(this,uiSelExternalAttrInps,cubeSel) );
	sels += newsel;
	prevsel = newsel;
    }
}


void cubeSel( CallBacker* cb )
{
    mDynamicCastGet(uiIOObjSel*,cursel,cb)
    if ( !cursel ) { pErrMsg("Huh"); return; }

    cursel->commitInput();
    const IOObj* ioobj = cursel->ctxtIOObj().ioobj;
    if ( !ioobj ) return;
    int curidx = indexOf( sels, cursel );
    if ( curidx >= nrindir ) return;

    IOPar iopar;
    cursel->updateHistory( iopar );
    for ( int idx=0; idx<nrindir; idx++ )
    {
	uiIOObjSel& sel = *sels[idx];
	if ( &sel == cursel ) continue;
	sel.getHistory( iopar );
	if ( !sel.ctxtIOObj().ioobj )
	{
	    sel.ctxtIOObj().setObj( ioobj->clone() );
	    sel.updateInput();
	}
    }
}


bool acceptOK( CallBacker* )
{
    for ( int isel=0; isel<sels.size(); isel++ )
    {
	uiIOObjSel& sel = *sels[isel];
	sel.commitInput();
	const IOObj* ioobj = sel.ctxtIOObj().ioobj;
	if ( !ioobj )
	{
	    BufferString msg( "Please supply input for '" );
	    msg += sel.labelText(); msg += "'";
	    uiMSG().error( msg );
	    return false;
	}

	const DescID descid = attrset->getID( sel.labelText(), true );
	if ( !descid.isValid() )
	{
	    BufferString msg( "There is a problem importing '" );
	    msg += sel.labelText();
	    msg += "'.\nPlease contact support for possible workarounds.";
	    uiMSG().error( msg );
	    return false;
	}
	Desc& ad = *attrset->getDesc( descid );
	if ( ad.isStored() )
	{
//	    ad.setDefStr( ioobj->key(), false );
	}
	else
	{
	    const DescID inpid = attrset->getStoredID( ioobj->key(), 0, true );
	    if ( !inpid.isValid() ) return false;

	    for ( int iinp=0; iinp<ad.nrInputs(); iinp++ )
		ad.setInput( iinp, attrset->getDesc(inpid) );
	}
    }

    return true;
}


protected:

    DescSet*			attrset;
    ObjectSet<uiIOObjSel> 	sels;
    int				nrindir;
};


AttributeSetCreator::AttributeSetCreator( uiParent* p_,
					  const BufferStringSet& inps_,
					  DescSet* ads )
    : prnt(p_)
    , attrset(ads)
{
    for ( int idx=0; idx<inps_.size(); idx++ )
    {
	const BufferString& uref = *inps_[idx];
	Desc* ad = getDesc( uref );
	if ( !ad )
	    { attrset->removeAll( false ); attrset = 0; return; }

	if ( ad->isStored() )
	    directs += new BufferString( uref );
	else
	    indirects += new BufferString( uref );
    }
}


bool AttributeSetCreator::create()
{
    const int nrdescs = attrset->size();
    if ( !attrset || nrdescs < 1 )
    {
	uiMSG().error( "The attributes cannot be converted" );
	return false;
    }

    const Desc* stored = 0;
    for ( int idx=0; idx<nrdescs; idx++ )
    {
	const Desc& desc = *attrset->desc( idx );
	if ( desc.isHidden() )
	   continue;
	if ( desc.isStored() )
	{
	    if ( desc.getStoredID() == storhint_ )
	       stored = &desc;
	}
    }

    if ( !stored && !storhint_.isEmpty() )
    {
	DescID did = attrset->createStoredDesc( storhint_, 0,
						BufferString("") );
	stored = attrset->getDesc( did );
    }

    if ( !stored )
    {
	uiSelExternalAttrInps dlg( prnt, attrset, indirects, directs );
	return dlg.go();
    }
    else
    {
	for ( int idx=0; idx<nrdescs; idx++ )
	{
	    Desc& desc = *attrset->desc( idx );
	    if ( desc.isHidden() || desc.isStored() )
	       continue;
	    desc.setInput( 0, stored );
	}
    }

    return true;
}


static void addGate( BufferString& defstr, const char* extdesc )
{
    BufferString extstr( extdesc );
    char* ptr = strchr( extstr.buf(), '[' );
    BufferString gatestr( ptr ? ptr : "[-32,32]" );
    ptr = strchr( gatestr.buf(), ']' );
    if ( ptr ) *(ptr+1) = '\0';
    defstr += " gate=";
    defstr += gatestr;
}


Desc* AttributeSetCreator::getDesc( const char* extdesc )
{
    if ( ! extdesc || !*extdesc ) return 0;

    BufferString defstr;
    if ( matchStringCI("Energy",extdesc) )
    {
	defstr = "Energy";
	addGate( defstr, extdesc );
	defstr += "output=0";
    }
    else if ( matchStringCI("Reference time",extdesc) )
    {
	defstr = "Reference output=2";
    }
    else if ( matchStringCI("Sample",extdesc) )
    {
	BufferString offs = extdesc + 7;
	char* ptr = offs.buf();
	mSkipBlanks(ptr);
	offs = ptr;
	ptr = strchr( offs.buf(), ' ' );
	if ( ptr ) *ptr = '\0';
	defstr = "Shift pos=0,0 steering=No time=";
	defstr += offs;
    }
    else if ( matchStringCI("Similarity",extdesc) )
    {
	defstr = "Similarity steering=No";
	addGate( defstr, extdesc );
	BufferString work( extdesc );
	char* pos0ptr = strchr( work.buf(), '=' );
	if ( pos0ptr ) pos0ptr++;
	char* pos1ptr = pos0ptr ? strchr( pos0ptr, 'x' ) : 0;
	char* ampptr = pos1ptr ? strchr( pos1ptr, '&' ) : 0;

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
	uiMSG().error( "Cannot find attribute name" );
	return 0;
    }

    RefMan<Desc> desc = PF().createDescCopy( attribname );
    if ( !desc )
    {
	BufferString err = "Cannot find factory-entry for "; err += attribname;
	uiMSG().error( err );
	return 0;
    }

    if ( !desc->isStored() && !desc->parseDefStr(defstr) )
    {
	BufferString err = "Cannot parse: "; err += defstr;
	uiMSG().error( err );
	return 0;
    }

    desc->setUserRef( extdesc );
    attrset->addDesc( desc );
    return desc;
}


void AttributeSetCreator::setStorageHint( const MultiID& m )
{ storhint_ = m; }
