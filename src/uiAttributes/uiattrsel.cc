/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          May 2005
 RCS:           $Id: uiattrsel.cc,v 1.25 2008-01-31 19:06:39 cvskris Exp $
________________________________________________________________________

-*/

#include "uiattrsel.h"
#include "attribdescset.h"
#include "attribdesc.h"
#include "attribfactory.h"
#include "attribparam.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "hilbertattrib.h"

#include "ioman.h"
#include "iodir.h"
#include "ioobj.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "datainpspec.h"
#include "ptrman.h"
#include "seistrctr.h"
#include "linekey.h"
#include "cubesampling.h"
#include "survinfo.h"

#include "nlamodel.h"
#include "nladesign.h"

#include "uibutton.h"
#include "uibuttongroup.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uimsg.h"

using namespace Attrib;


uiAttrSelDlg::uiAttrSelDlg( uiParent* p, const char* seltxt,
			    const uiAttrSelData& atd, 
			    bool is2d, DescID ignoreid )
	: uiDialog(p,Setup("",0,atd.nlamodel?"":"101.1.1"))
	, attrdata_(atd)
	, selgrp_(0)
	, storoutfld_(0)
	, attroutfld_(0)
	, attr2dfld_(0)
	, nlafld_(0)
	, nlaoutfld_(0)
    	, zdomainfld_(0)
	, zdomoutfld_(0)
	, in_action_(false)
{
    attrinf_ = new SelInfo( atd.attrset, atd.nlamodel, is2d, ignoreid );
    if ( attrinf_->ioobjnms.isEmpty() )
    {
	new uiLabel( this, "No seismic data available.\n"
			   "Please import data first" );
	setCancelText( "Ok" );
	setOkText( "" );
	return;
    }

    const bool havenlaouts = attrinf_->nlaoutnms.size();
    const bool haveattribs = attrinf_->attrnms.size();

    BufferString nm( "Select " ); nm += seltxt;
    setName( nm );
    setCaption( "Select" );
    setTitleText( nm );

    createSelectionButtons();
    createSelectionFields();

    int seltyp = havenlaouts ? 2 : (haveattribs ? 1 : 0);
    int storcur = -1, attrcur = -1, nlacur = -1;
    if ( attrdata_.nlamodel && attrdata_.outputnr >= 0 )
    {
	seltyp = 2;
	nlacur = attrdata_.outputnr;
    }
    else
    {
	const Desc* desc = attrdata_.attribid < 0 ? 0 :
	    		attrdata_.attrset->getDesc( attrdata_.attribid );
	if ( desc )
	{
	    seltyp = desc->isStored() ? 0 : 1;
	    if ( seltyp == 1 )
		attrcur = attrinf_->attrnms.indexOf( desc->userRef() );
	    else if ( storoutfld_ )
		storcur = attrinf_->ioobjnms.indexOf( desc->userRef() );
	}
	else
	{
	    // Defaults are the last ones added to attrib set
	    for ( int idx=attrdata_.attrset->nrDescs()-1; idx!=-1; idx-- )
	    {
		const DescID attrid = attrdata_.attrset->getID( idx );
		const Desc& ad = *attrdata_.attrset->getDesc( attrid );
		if ( ad.isStored() && storcur == -1 )
		    storcur = attrinf_->ioobjnms.indexOf( ad.userRef() );
		else if ( !ad.isStored() && attrcur == -1 )
		    attrcur = attrinf_->attrnms.indexOf( ad.userRef() );
		if ( storcur != -1 && attrcur != -1 ) break;
	    }
	}
    }

    if ( storcur == -1 )		storcur = 0;
    if ( attrcur == -1 )		attrcur = attrinf_->attrnms.size()-1;
    if ( nlacur == -1 && havenlaouts )	nlacur = 0;

    if ( storoutfld_  )			storoutfld_->setCurrentItem( storcur );
    if ( attroutfld_ && attrcur != -1 )	attroutfld_->setCurrentItem( attrcur );
    if ( nlaoutfld_ && nlacur != -1 )	nlaoutfld_->setCurrentItem( nlacur );

    if ( seltyp == 0 )		storfld_->setChecked(true);
    else if ( seltyp == 1 )	attrfld_->setChecked(true);
    else if ( nlafld_ )		nlafld_->setChecked(true);

    finaliseStart.notify( mCB( this,uiAttrSelDlg,doFinalise) );
}


uiAttrSelDlg::~uiAttrSelDlg()
{
    delete selgrp_;
    delete attrinf_;
}


void uiAttrSelDlg::doFinalise( CallBacker* )
{
    selDone(0);
    in_action_ = true;
}


void uiAttrSelDlg::createSelectionButtons()
{
    const bool havenlaouts = attrinf_->nlaoutnms.size();
    const bool haveattribs = attrinf_->attrnms.size();

    selgrp_ = new uiButtonGroup( this, "Input selection" );
    storfld_ = new uiRadioButton( selgrp_, "Stored" );
    storfld_->activated.notify( mCB(this,uiAttrSelDlg,selDone) );
    storfld_->setSensitive( attrdata_.shwcubes );

    attrfld_ = new uiRadioButton( selgrp_, "Attributes" );
    attrfld_->setSensitive( haveattribs );
    attrfld_->activated.notify( mCB(this,uiAttrSelDlg,selDone) );

    if ( havenlaouts )
    {
	nlafld_ = new uiRadioButton( selgrp_,
				     attrdata_.nlamodel->nlaType(false) );
	nlafld_->setSensitive( havenlaouts );
	nlafld_->activated.notify( mCB(this,uiAttrSelDlg,selDone) );
    }

    if ( !attrdata_.zdomainkey.isEmpty() )
    {
	zdomainfld_ = new uiRadioButton( selgrp_, attrdata_.zdomainkey);
	zdomainfld_->activated.notify( mCB(this,uiAttrSelDlg,selDone) );
    }
}


void uiAttrSelDlg::createSelectionFields()
{
    const bool havenlaouts = attrinf_->nlaoutnms.size();
    const bool haveattribs = attrinf_->attrnms.size();

    if ( attrdata_.shwcubes )
    {
	storoutfld_ = new uiListBox( this, attrinf_->ioobjnms );
	storoutfld_->setHSzPol( uiObject::Wide );
	storoutfld_->selectionChanged.notify( mCB(this,uiAttrSelDlg,cubeSel) );
	storoutfld_->doubleClicked.notify( mCB(this,uiAttrSelDlg,accept) );
	storoutfld_->attach( rightOf, selgrp_ );
	attr2dfld_ = new uiGenInput( this, "Stored Attribute",
				    StringListInpSpec() );
	attr2dfld_->attach( alignedBelow, storoutfld_ );
	filtfld_ = new uiGenInput( this, "Filter", "*" );
	filtfld_->attach( alignedBelow, storoutfld_ );
	filtfld_->valuechanged.notify( mCB(this,uiAttrSelDlg,filtChg) );
    }

    if ( haveattribs )
    {
	attroutfld_ = new uiListBox( this, attrinf_->attrnms );
	attroutfld_->setHSzPol( uiObject::Wide );
	attroutfld_->doubleClicked.notify( mCB(this,uiAttrSelDlg,accept) );
	attroutfld_->attach( rightOf, selgrp_ );
    }

    if ( havenlaouts )
    {
	nlaoutfld_ = new uiListBox( this, attrinf_->nlaoutnms );
	nlaoutfld_->setHSzPol( uiObject::Wide );
	nlaoutfld_->doubleClicked.notify( mCB(this,uiAttrSelDlg,accept) );
	nlaoutfld_->attach( rightOf, selgrp_ );
    }

    if ( !attrdata_.zdomainkey.isEmpty() )
    {
	BufferStringSet nms;
	SelInfo::getSpecialItems( attrdata_.zdomainkey, nms );
	zdomoutfld_ = new uiListBox( this, nms );
	zdomoutfld_->setHSzPol( uiObject::Wide );
	zdomoutfld_->doubleClicked.notify( mCB(this,uiAttrSelDlg,accept) );
	zdomoutfld_->attach( rightOf, selgrp_ );
    }

}


int uiAttrSelDlg::selType() const
{
    if ( attrfld_->isChecked() )
	return 1;
    if ( nlafld_ && nlafld_->isChecked() )
	return 2;
    if ( zdomainfld_ && zdomainfld_->isChecked() )
	return 3;
    return 0;
}


void uiAttrSelDlg::selDone( CallBacker* c )
{
    if ( !selgrp_ ) return;

    mDynamicCastGet(uiRadioButton*,but,c);
   
    bool dosrc, docalc, donla; 
    if ( but == storfld_ )
    { dosrc = true; docalc = donla = false; }
    else if ( but == attrfld_ )
    { docalc = true; dosrc = donla = false; }
    else if ( but == nlafld_ )
    { donla = true; docalc = dosrc = false; }

    const int seltyp = selType();
    if ( attroutfld_ ) attroutfld_->display( seltyp == 1 );
    if ( nlaoutfld_ ) nlaoutfld_->display( seltyp == 2 );
    if ( zdomoutfld_ ) zdomoutfld_->display( seltyp == 3 );
    if ( storoutfld_ )
    {
	storoutfld_->display( seltyp == 0 );
	filtfld_->display( seltyp == 0 );
    }

    cubeSel(0);
}


void uiAttrSelDlg::filtChg( CallBacker* c )
{
    if ( !storoutfld_ || !filtfld_ ) return;

    attrinf_->fillStored( filtfld_->text() );
    storoutfld_->empty();
    storoutfld_->addItems( attrinf_->ioobjnms );
    if ( attrinf_->ioobjnms.size() )
	storoutfld_->setCurrentItem( 0 );

    cubeSel( c );
}


void uiAttrSelDlg::cubeSel( CallBacker* c )
{
    const int seltyp = selType();
    if ( seltyp )
    {
	attr2dfld_->display( false );
	return;
    }

    int selidx = storoutfld_->currentItem();
    bool is2d = false;
    BufferString ioobjkey;
    if ( selidx >= 0 )
    {
	ioobjkey = attrinf_->ioobjids.get( storoutfld_->currentItem() );
	is2d = SelInfo::is2D( ioobjkey.buf() );
    }
    attr2dfld_->display( is2d );
    filtfld_->display( !is2d );
    if ( is2d )
    {
	BufferStringSet nms;
	SelInfo::getAttrNames( ioobjkey.buf(), nms );
	attr2dfld_->newSpec( StringListInpSpec(nms), 0 );
    }
}


bool uiAttrSelDlg::getAttrData( bool needattrmatch )
{
    attrdata_.attribid = DescID::undef();
    attrdata_.outputnr = -1;
    zdomainkey_ = "";
    if ( !selgrp_ || !in_action_ ) return true;

    int selidx = -1;
    const int seltyp = selType();
    if ( seltyp==1 )		selidx = attroutfld_->currentItem();
    else if ( seltyp==2 )	selidx = nlaoutfld_->currentItem();
    else if ( seltyp==3 )	selidx = zdomoutfld_->currentItem();
    else			selidx = storoutfld_->currentItem();
    if ( selidx < 0 )
	return false;

    if ( seltyp == 1 )
	attrdata_.attribid = attrinf_->attrids[selidx];
    else if ( seltyp == 2 )
	attrdata_.outputnr = selidx;
    else if ( seltyp == 3 )
    {
	BufferStringSet nms;
	SelInfo::getSpecialItems( attrdata_.zdomainkey, nms );
	IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Seis)->id));
	PtrMan<IOObj> ioobj = IOM().getLocal( nms.get(selidx) );
	if ( !ioobj ) return false;

	LineKey linekey( ioobj->key() );
	DescSet& as = const_cast<DescSet&>( *attrdata_.attrset );
	attrdata_.attribid = as.getStoredID( linekey, 0, true );
	zdomainkey_ = attrdata_.zdomainkey;
    }
    else
    {
	const char* ioobjkey = attrinf_->ioobjids.get(selidx);
	LineKey linekey( ioobjkey );
	if ( SelInfo::is2D(ioobjkey) )
	{
	    attrdata_.outputnr = attr2dfld_->getIntValue();
	    BufferStringSet nms;
	    SelInfo::getAttrNames( ioobjkey, nms );
	    if ( nms.isEmpty() )
	    {
		uiMSG().error( "No data available" );
		return false;
	    }
	    const char* attrnm = attrdata_.outputnr >= nms.size() ? 0
				    : nms.get(attrdata_.outputnr).buf();
	    if ( needattrmatch )
		linekey.setAttrName( attrnm );
	}

	DescSet& as = const_cast<DescSet&>( *attrdata_.attrset );
	attrdata_.attribid = as.getStoredID( linekey, 0, true );
	if ( needattrmatch && attrdata_.attribid < 0 )
	{
	    BufferString msg( "Could not find the seismic data " );
	    msg += attrdata_.attribid == DescID::undef() ? "in object manager"
							 : "on disk";	
	    uiMSG().error( msg );
	    return false;
	}
    }

    return true;
}


bool uiAttrSelDlg::acceptOK( CallBacker* )
{
    return getAttrData(true);
}


uiAttrSel::uiAttrSel( uiParent* p, const DescSet* ads, bool is2d,
		      const char* txt, DescID curid )
    : uiIOSelect(p,mCB(this,uiAttrSel,doSel),txt?txt:"Input Data")
    , attrdata_(ads)
    , ignoreid(DescID::undef())
    , is2d_(is2d)
{
    attrdata_.attribid = curid;
    updateInput();
}


uiAttrSel::uiAttrSel( uiParent* p, const char* txt, 
		      const uiAttrSelData& ad, bool is2d )
    : uiIOSelect(p,mCB(this,uiAttrSel,doSel),txt?txt:"Input Data")
    , attrdata_(ad)
    , ignoreid(DescID::undef())
    , is2d_(is2d)
{
    updateInput();
}


void uiAttrSel::setDescSet( const DescSet* ads )
{
    attrdata_.attrset = ads;
    if ( ads)
	is2d_ = ads->is2D();
}


void uiAttrSel::setNLAModel( const NLAModel* mdl )
{
    attrdata_.nlamodel = mdl;
}


void uiAttrSel::setDesc( const Desc* ad )
{
    attrdata_.attrset = ad ? ad->descSet() : 0;
    if ( !ad ) return;

    const char* inp = ad->userRef();
    if ( inp[0] == '_' || (ad->isStored() && ad->dataType() == Seis::Dip) )
	return;

    attrdata_.attribid = !attrdata_.attrset ? DescID::undef() : ad->id();
    updateInput();
}


void uiAttrSel::setIgnoreDesc( const Desc* ad )
{
    ignoreid = DescID::undef();
    if ( !ad ) return;
    if ( !attrdata_.attrset ) attrdata_.attrset = ad->descSet();
    ignoreid = ad->id();
}


void uiAttrSel::updateInput()
{
    BufferString bs;
    bs = attrdata_.attribid.asInt();
    bs += ":";
    bs += attrdata_.outputnr;
    setInput( bs );
}


const char* uiAttrSel::userNameFromKey( const char* txt ) const
{
    if ( !attrdata_.attrset || !txt || !*txt ) return "";

    BufferString buf( txt );
    char* outnrstr = strchr( buf.buf(), ':' );
    if ( outnrstr ) *outnrstr++ = '\0';
    const DescID attrid( atoi(buf.buf()), true );
    const int outnr = outnrstr ? atoi( outnrstr ) : 0;
    if ( attrid < 0 )
    {
	if ( !attrdata_.nlamodel || outnr < 0 )
	    return "";
	if ( outnr >= attrdata_.nlamodel->design().outputs.size() )
	    return "<error>";

	const char* nm = attrdata_.nlamodel->design().outputs[outnr]->buf();
	return IOObj::isKey(nm) ? IOM().nameOf(nm) : nm;
    }

    const Desc* ad = attrdata_.attrset->getDesc( attrid );
    usrnm = ad ? ad->userRef() : "";
    return usrnm.buf();
}


void uiAttrSel::getHistory( const IOPar& iopar )
{
    uiIOSelect::getHistory( iopar );
    updateInput();
}


bool uiAttrSel::getRanges( CubeSampling& cs ) const
{
    if ( attrdata_.attribid < 0 || !attrdata_.attrset )
	return false;

    const Desc* desc = attrdata_.attrset->getDesc( attrdata_.attribid );
    if ( !desc->isStored() ) return false;

    const ValParam* keypar = 
		(ValParam*)desc->getParam( StorageProvider::keyStr() );
    const MultiID mid( keypar->getStringValue() );
    return SeisTrcTranslator::getRanges( mid, cs,
					 desc->is2D() ? getInput() : 0 );
}


void uiAttrSel::doSel( CallBacker* )
{
    if ( !attrdata_.attrset ) return;

    uiAttrSelDlg dlg( this, inp_->label()->text(), attrdata_, is2d_, ignoreid );
    if ( dlg.go() )
    {
	attrdata_.attribid = dlg.attribID();
	attrdata_.outputnr = dlg.outputNr();
	updateInput();
	selok_ = true;
    }
}


void uiAttrSel::processInput()
{
    if ( !attrdata_.attrset ) return;

    BufferString inp = getInput();
    char* attr2d = strchr( inp.buf(), '|' );
    DescSet& as = const_cast<DescSet&>( *attrdata_.attrset );
    attrdata_.attribid = as.getID( inp, true );
    attrdata_.outputnr = -1;
    if ( attrdata_.attribid >= 0 && attr2d )
    {
	const Desc& ad = *attrdata_.attrset->getDesc(attrdata_.attribid);
	BufferString defstr; ad.getDefStr( defstr );
	BufferStringSet nms;
	SelInfo::getAttrNames( defstr, nms );
	attrdata_.outputnr = nms.indexOf( attr2d );
    }
    else if ( attrdata_.attribid < 0 && attrdata_.nlamodel )
    {
	const BufferStringSet& outnms( attrdata_.nlamodel->design().outputs );
	const BufferString nodenm = IOObj::isKey(inp) ? IOM().nameOf(inp)
	    						: inp.buf();
	for ( int idx=0; idx<outnms.size(); idx++ )
	{
	    const BufferString& outstr = *outnms[idx];
	    const char* desnm = IOObj::isKey(outstr) ? IOM().nameOf(outstr)
						     : outstr.buf();
	    if ( nodenm == desnm )
		{ attrdata_.outputnr = idx; break; }
	}
    }

    updateInput();
}


void uiAttrSel::fillSelSpec( SelSpec& as ) const
{
    const bool isnla = attrdata_.attribid < 0 && attrdata_.outputnr >= 0;
    if ( isnla )
	as.set( 0, DescID(attrdata_.outputnr,true), true, "" );
    else
	as.set( 0, attrdata_.attribid, false, "" );

    if ( isnla && attrdata_.nlamodel )
	as.setRefFromID( *attrdata_.nlamodel );
    else
	as.setRefFromID( *attrdata_.attrset );

    if ( is2D() )
	as.set2DFlag();
}


const char* uiAttrSel::getAttrName() const
{
    static BufferString ret;

    ret = getInput();
    if ( is2d_ )
    {
	const Desc* ad = attrdata_.attrset->getDesc( attrdata_.attribid );
	if ( (ad && ad->isStored()) || strchr(ret.buf(),'|') )
	    ret = LineKey( ret ).attrName();
    }

    return ret.buf();
}


bool uiAttrSel::checkOutput( const IOObj& ioobj ) const
{
    if ( attrdata_.attribid < 0 && attrdata_.outputnr < 0 )
    {
	uiMSG().error( "Please select the input" );
	return false;
    }

    if ( is2d_ && !SeisTrcTranslator::is2D(ioobj) )
    {
	uiMSG().error( "Can only store this in a 2D line set" );
	return false;
    }

    //TODO this is pretty difficult to get right
    if ( attrdata_.attribid < 0 )
	return true;

    const Desc& ad = *attrdata_.attrset->getDesc( attrdata_.attribid );
    bool isdep = false;
/*
    if ( !is2d_ )
	isdep = ad.isDependentOn(ioobj,0);
    else
    {
	// .. and this too
	if ( ad.isStored() )
	{
	    LineKey lk( ad.defStr() );
	    isdep = ioobj.key() == lk.lineName();
	}
    }
    if ( isdep )
    {
	uiMSG().error( "Cannot output to an input" );
	return false;
    }
*/

    return true;
}


// **** uiImagAttrSel ****

DescID uiImagAttrSel::imagID() const
{
    if ( !attrdata_.attrset )
    {
	pErrMsg( "No attribdescset set");
	return DescID::undef();
    }

    const DescID selattrid = attribID();
    TypeSet<DescID> attribids;
    attrdata_.attrset->getIds( attribids );
    for ( int idx=0; idx<attribids.size(); idx++ )
    {
	const Desc* desc = attrdata_.attrset->getDesc( attribids[idx] );

	if ( strcmp(desc->attribName(),Hilbert::attribName()) )
	    continue;

	const Desc* inputdesc = desc->getInput( 0 );
	if ( !inputdesc || inputdesc->id() != selattrid )
	    continue;

	return attribids[idx];
    }

    DescSet* descset = const_cast<DescSet*>(attrdata_.attrset);

    Desc* inpdesc = descset->getDesc( selattrid );
    Desc* newdesc = PF().createDescCopy( Hilbert::attribName() );
    if ( !newdesc || !inpdesc ) return DescID::undef();

    newdesc->selectOutput( 0 );
    newdesc->setInput( 0, inpdesc );
    newdesc->setHidden( true );

    BufferString usrref = "_"; usrref += inpdesc->userRef(); usrref += "_imag";
    newdesc->setUserRef( usrref );

    return descset->addDesc( newdesc );
}
