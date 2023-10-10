/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uistratbasiclayseqgendesc.h"

#include "uicombobox.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uilabel.h"
#include "uimanprops.h"
#include "uimenu.h"
#include "uimsg.h"
#include "uistratlaycontent.h"
#include "uistratselunits.h"
#include "uistratsimplelaymoddisp.h"
#include "uiunitsel.h"

#include "keyenum.h"
#include "mathproperty.h"
#include "od_helpids.h"
#include "stratcontent.h"
#include "stratlayermodel.h"
#include "stratlayseqgendesc.h"
#include "stratreftree.h"
#include "stratsinglaygen.h"
#include "stratunitrefiter.h"
#include "unitofmeasure.h"


mImplFactory2Param(uiLayerSequenceGenDesc,uiParent*,
	Strat::LayerSequenceGenDesc&,uiLayerSequenceGenDesc::factory)

// uiStratLayerContent

uiStratLayerContent::uiStratLayerContent( uiParent* p, bool isfinal,
				  const Strat::RefTree& srt )
    : uiGroup(p,"Layer content")
    , rt_(srt)
    , contentSelected(this)
{
    auto* lcb = new uiLabeledComboBox( this,
				isfinal ? tr("Content") : tr("Content zone") );
    fld_ = lcb->box();
    fld_->addItem( "-" );
    for ( int idx=0; idx<rt_.contents().size(); idx++ )
	fld_->addItem( rt_.contents()[idx]->name() );
    setHAlignObj( lcb );

    mAttachCB( fld_->selectionChanged, uiStratLayerContent::contSel );
}


uiStratLayerContent::~uiStratLayerContent()
{
    detachAllNotifiers();
}


void uiStratLayerContent::set( const Strat::Content& c )
{
    if ( c.isUnspecified() )
	fld_->setCurrentItem( 0 );
    else
	fld_->setCurrentItem( c.name().buf() );
}


void uiStratLayerContent::contSel( CallBacker* )
{
    contentSelected.trigger();
}


void uiStratLayerContent::setSelectedIndex( int selidx )
{
    return fld_->setCurrentItem( selidx );
}


int uiStratLayerContent::selectedIndex() const
{
    return fld_->currentItem();
}


int uiStratLayerContent::addOption( const char* nm )
{
    fld_->addItem( toUiString(nm) );
    return fld_->size() - 1;
}


const Strat::Content& uiStratLayerContent::get() const
{
    const int selidx = fld_->currentItem();
    if ( selidx < 1 || selidx > rt_.contents().size() )
	return Strat::Content::unspecified();

    return *rt_.contents()[selidx-1];
}


// uiLayerSequenceGenDesc

uiLayerSequenceGenDesc::uiLayerSequenceGenDesc( Strat::LayerSequenceGenDesc& d )
    : desc_(d)
{
    if ( desc_.propSelection().size() < 2 )
    {
	PropertyRefSelection prs( desc_.propSelection() );
	if ( !ePROPS().ensureHasElasticProps(false,true) )
	    return;

	const PropertyRefSet& props = PROPS();
	prs.add( props.getByMnemonic(Mnemonic::defDEN()) )
	   .add( props.getByMnemonic(Mnemonic::defPVEL()) )
	   .add( props.getByMnemonic(Mnemonic::defAI()) );
	desc_.setPropSelection( prs );
    }

}


uiLayerSequenceGenDesc::~uiLayerSequenceGenDesc()
{
}

#define mErrRet( msg ) \
{ uiMSG().error( msg ); return false; }


bool uiLayerSequenceGenDesc::isValidSelection(
				const PropertyRefSelection& props ) const
{
    if ( props.isEmpty() )
	mErrRet( tr("No property is selected.") )

    if ( !props.first()->isThickness() )
    {
	pErrMsg( "Thickness should always be the first property" );
	return false;
    }

    if ( !props.getByMnemonic(Mnemonic::defDEN()) )
	mErrRet( tr("No property of type 'Density' selected") )

    if ( !props.getByMnemonic(Mnemonic::defPVEL()) )
	mErrRet( tr("No property of type 'P-wave velocity' selected") )

    return true;
}

const Strat::LayerSequenceGenDesc& uiLayerSequenceGenDesc::currentDesc() const
{
    return !needSave() || !editedDesc() ? desc_ : *editedDesc();
}


bool uiLayerSequenceGenDesc::selProps()
{
    PropertyRefSelection prs( desc_.propSelection() );
    uiSelectPropRefs dlg( outerObj()->parent(), prs );
    const bool ret = dlg.go();
    if ( ret || dlg.structureChanged() )
    {
	if ( !isValidSelection(prs) )
	    return false;
	desc_.setPropSelection( prs );
	descHasChanged();
    }
    return ret;
}


// uiExtLayerSequenceGenDesc

uiExtLayerSequenceGenDesc::uiExtLayerSequenceGenDesc( uiParent* p,
					Strat::LayerSequenceGenDesc& dsc )
    : uiGraphicsView(p,"LayerSequence Gen Desc editor")
    , uiLayerSequenceGenDesc(dsc)
    , editdesc_(*new Strat::LayerSequenceGenDesc(dsc))
{
    setScrollBarPolicy( false, uiGraphicsViewBase::ScrollBarAsNeeded );
    border_.setTop( border_.top() + 10 );
    border_.setRight( border_.right() + 10 );
    setPrefWidth( 180 );
    setPrefHeight( 500 );
    mAttachCB( reSize, uiExtLayerSequenceGenDesc::reDraw );
    mAttachCB( reDrawNeeded, uiExtLayerSequenceGenDesc::reDraw );

    mAttachCB( getNavigationMouseEventHandler().wheelMove,
	       uiExtLayerSequenceGenDesc::wheelMoveCB );
    mAttachCB( getMouseEventHandler().buttonReleased,
	       uiExtLayerSequenceGenDesc::singClckCB );
    mAttachCB( getMouseEventHandler().doubleClick,
	       uiExtLayerSequenceGenDesc::dblClckCB );

    const uiString lbltxt = tr("top (%1)")
		.arg( PropertyRef::thickness().disp_.getUnitLbl() );
    topdepthfld_ = new uiGenInput( p, lbltxt, FloatInpSpec(dsc.startDepth()) );
    topdepthfld_->setElemSzPol( uiObject::Small );
    topdepthfld_->attach( rightBorder );

    mAttachCB( postFinalize(), uiExtLayerSequenceGenDesc::initView );

    this->attach( ensureBelow, topdepthfld_ );
}


uiExtLayerSequenceGenDesc::~uiExtLayerSequenceGenDesc()
{
    detachAllNotifiers();
    delete &editdesc_;
}


void uiExtLayerSequenceGenDesc::initView( CallBacker* )
{
    if ( !topdepthfld_->finalized() )
	topdepthfld_->preFinalize().trigger();

    topdepthfld_->setToolTip( tr("TVDSS depth assigned to the first layer "
				 "of each pseudowell") );
}


uiStratLayerModelDisp* uiExtLayerSequenceGenDesc::getLayModDisp(
	    uiStratLayModEditTools& lmt, Strat::LayerModelSuite& lms, int )
{
    return new uiStratSimpleLayerModelDisp( lmt, lms );
}


void uiExtLayerSequenceGenDesc::setDescID( const MultiID& dbky )
{
    descid_ = dbky;
    putTopDepthToScreen();
}


void uiExtLayerSequenceGenDesc::setEditDesc()
{
    editdesc_ = desc_;
}


void uiExtLayerSequenceGenDesc::setFromEditDesc()
{
    const IOPar descworkbencpars = desc_.getWorkBenchParams();
    desc_ = editdesc_;
    desc_.getWorkBenchParams() = descworkbencpars;
}


bool uiExtLayerSequenceGenDesc::selProps()
{
    PropertyRefSelection prs( editdesc_.propSelection() );
    uiSelectPropRefs dlg( outerObj()->parent(), prs );
    const bool ret = dlg.go();
    if ( ret || dlg.structureChanged() )
    {
	if ( !isValidSelection(prs) )
	    return false;
	editdesc_.setPropSelection( prs );
	descHasChanged();
    }
    return ret;
}


void uiExtLayerSequenceGenDesc::reDraw( CallBacker* )
{
    const uiRect scenerect = getSceneRect();
    uiRect& wr = const_cast<uiRect&>( workrect_ );
    wr.setLeft( border_.left() );
    wr.setRight( (int)(scenerect.width() - border_.right() + .5) );
    wr.setTop( border_.top() );
    wr.setBottom( (int)(scenerect.height() - border_.bottom() + .5) );

    if ( !outeritm_ )
    {
	outeritm_ = scene().addItem( new uiRectItem );
	outeritm_->setPenColor( OD::Color::Black() );
	outeritm_->setZValue( mUdf(int) );
    }
    outeritm_->setRect( workrect_.left(), workrect_.top(),
			workrect_.width(), workrect_.height() );
    putTopDepthToScreen();

    if ( editdesc_.isEmpty() )
    {
	if ( !emptyitm_ )
	{
	    emptyitm_ = scene().addItem( new uiTextItem( tr("<Click to add>"),
					mAlignment(HCenter,VCenter) ) );
	    emptyitm_->setPenColor( OD::Color::Black() );
	    emptyitm_->setPos( workrect_.centre() );
	}
    }
    else
    {
	deleteAndNullPtr( emptyitm_ );
	doDraw();
    }
}


void uiExtLayerSequenceGenDesc::putTopDepthToScreen()
{
    const float topz = editdesc_.startDepth();
    topdepthfld_->setValue( topz );
}


void uiExtLayerSequenceGenDesc::getTopDepthFromScreen()
{
    const float topz = topdepthfld_->getFValue();
    editdesc_.setStartDepth( topz );
}


void uiExtLayerSequenceGenDesc::wheelMoveCB( CallBacker* cb )
{
    MouseEventHandler& mevh = getNavigationMouseEventHandler();
    if ( !mevh.hasEvent() || mevh.isHandled() )
	return;
    uiRect scenerect = getSceneRect();
    const uiSize scnrectsz = scenerect.size();
    const MouseEvent& mev = mevh.event();
    uiPoint prevmousescnpos = mev.pos();
    const float relscnposy = mCast(float,(prevmousescnpos.y-scenerect.top()))/
			     mCast(float,scenerect.height());

    const double dheight = scnrectsz.height() * 0.2;
    uiSize dsize( 0, mNINT32(dheight) );
    if ( mev.angle() < 0 )
	scenerect += dsize;
    else
	scenerect -= dsize;
    setSceneRect( scenerect );
    const int newmousescnposy =
	scenerect.top()+(mNINT32(relscnposy*scenerect.height()));
    const int translate = newmousescnposy - prevmousescnpos.y;
    const int newcentery = scenerect.centre().y + translate;
    reDraw( 0 );
    centreOn( uiPoint(scenerect.centre().x,newcentery) );
}


void uiExtLayerSequenceGenDesc::hndlClick( CallBacker* cb, bool dbl )
{
    MouseEventHandler& mevh = getMouseEventHandler();
    if ( !mevh.hasEvent() || mevh.isHandled() )
	return;

    const MouseEvent& mev = mevh.event();
    const bool isright = OD::rightMouseButton( mev.buttonState() );
    const bool isempty = editdesc_.isEmpty();
    const bool needhandle = isempty || (dbl && !isright) || (!dbl && isright);
    if ( !needhandle )
	return;

    clickpos_ = mev.pos();
    mevh.setHandled( true );
    if ( workrect_.isOutside(clickpos_) )
	return;
    if ( (isempty || editdesc_.propSelection().size() < 2) && !selProps() )
	return;

    int mnuid = dbl ? 0 : (isempty ? 1 : -1);
    if ( !isempty && !dbl )
    {
	uiMenu mnu( parent(), uiStrings::sAction() );
	mnu.insertAction( new uiAction(
			m3Dots(uiStrings::sEdit())), 0 );
	mnu.insertAction( new uiAction(m3Dots(uiStrings::phrAdd(
						    uiStrings::sAbove()))), 1 );
	mnu.insertAction( new uiAction(m3Dots(uiStrings::phrAdd(
						    uiStrings::sBelow()))), 2 );
	if ( editdesc_.size() > 1 )
	{
	    mnu.insertSeparator();
	    mnu.insertAction( new uiAction(uiStrings::sRemove()), 3 );
	}
	mnuid = mnu.exec();
    }

    bool ischgd = false;
    if ( mnuid == 0 )
	ischgd = laygenEditReq();
    else if ( mnuid == 1 || mnuid == 2 )
	ischgd = newLayGenReq( mnuid == 1 );
    else if ( mnuid == 3 )
	ischgd = laygenRemoveReq();

    if ( ischgd )
	{ needsave_ = true; reDraw(0); }

    mevh.setHandled( true );
}


// uiBasicLayerSequenceGenDesc::DispUnit

uiBasicLayerSequenceGenDesc::DispUnit::DispUnit( uiGraphicsScene& scn,
				    const Strat::LayerGenerator& lg )
    : scene_(scn)
{
    mDynamicCastGet(const Strat::SingleLayerGenerator*,slg,&lg)
    if ( slg )
	gen_ = slg;
    else
    {
	auto* newgen = new Strat::SingleLayerGenerator;
	gen_ = newgen; genmine_ = true;
	Property& pr = *newgen->properties().first();
	mDynamicCastGet(ValueProperty*,vpr,&pr)
	vpr->val_ = lg.dispThickness();
    }

    nm_ = scene_.addItem( new uiTextItem( toUiString(gen_->name()),
			  mAlignment(HCenter,VCenter) ) );
    nm_->setPenColor( OD::Color::Black() );
    lithcol_ = scene_.addItem( new uiCircleItem );
    const OD::Color lithcolor( gen_->unit().dispColor(true) );
    lithcol_->setPenColor( lithcolor );
    lithcol_->setFillColor( lithcolor );
    top_ = scene_.addItem( new uiLineItem );
    top_->setPenStyle( OD::LineStyle(OD::LineStyle::Solid) );
    poly_ = scene_.addItem( new uiPolygonItem );
    const OD::LineStyle ls( OD::LineStyle::Solid, 2,
			    gen_->unit().dispColor(false) );
    poly_->setPenStyle( ls );
}


uiBasicLayerSequenceGenDesc::DispUnit::~DispUnit()
{
    if ( genmine_ )
	delete const_cast<Strat::SingleLayerGenerator*>(gen_);
    delete nm_; delete lithcol_; delete top_; delete poly_;
}


// uiBasicLayerSequenceGenDesc

uiBasicLayerSequenceGenDesc::uiBasicLayerSequenceGenDesc( uiParent* p,
				Strat::LayerSequenceGenDesc& d )
    : uiExtLayerSequenceGenDesc(p,d)
{
    rebuildDispUnits();
}


uiBasicLayerSequenceGenDesc::~uiBasicLayerSequenceGenDesc()
{
}


void uiBasicLayerSequenceGenDesc::rebuildDispUnits()
{
    deepErase( disps_ );
    for ( int idx=0; idx<editdesc_.size(); idx++ )
	insertDispUnit( *editdesc_[idx], idx );
}


void uiBasicLayerSequenceGenDesc::insertDispUnit(
			    const Strat::LayerGenerator& lgen, int newidx )
{
    DispUnit* newdisp = new DispUnit( scene(), lgen );
    if ( newidx < 0 || newidx >= disps_.size() )
	disps_ += newdisp;
    else
	disps_.insertAt( newdisp, newidx );
}


void uiBasicLayerSequenceGenDesc::doDraw()
{
    if ( disps_.isEmpty() )
	return;

    float totth = 0.f;
    for ( int idx=0; idx<disps_.size(); idx++ )
    {
	if ( disps_[idx]->gen_ )
	    totth += disps_[idx]->gen_->dispThickness();
    }
    if ( mIsZero(totth,mDefEps) )
	return;

    float curz = 0;
    for ( int idx=0; idx<disps_.size(); idx++ )
	fillDispUnit( idx, totth, curz );
}


void uiBasicLayerSequenceGenDesc::fillDispUnit( int idx, float totth,
						 float& curz )
{
    const float pixperm = workrect_.height() / totth;
    uiPoint midpt( workrect_.centre().x, 0 );
    uiPoint leftpt( workrect_.left(), 0 );
    uiPoint rightpt( workrect_.right(), 0 );

    DispUnit& disp = *disps_[idx];
    if ( disp.gen_->properties().isEmpty() )
	return;
    const Property& thprop = *disp.gen_->properties().first();
    if ( !thprop.ref().isThickness() )
    {
	pErrMsg( "Thickness should always be the first property" );
	return;
    }

    const float th0 = thprop.value( mPropertyEvalNew(0) );
    const float th1 = thprop.value( mPropertyEvalNew(1) );
    const bool growing = th1 > th0;
    const float& maxth = growing ? th1 : th0;
    const float& minth = growing ? th0 : th1;
    disp.topy_ = (int)(workrect_.top() + curz * pixperm + .5);
    curz += maxth;
    disp.boty_ = (int)(workrect_.top() + curz * pixperm + .5);

    BufferString dispnm( disp.gen_->name() );
    if ( !disp.gen_->content().isUnspecified() )
	dispnm.add( "[" ).add( disp.gen_->content().name() ).add( "]" );
    disp.nm_->setText(toUiString(getLimitedDisplayString(dispnm,25, false)));
    midpt.y = (disp.topy_ + disp.boty_) / 2;
    disp.nm_->setPos( mCast(float,midpt.x), mCast(float,midpt.y-2) );
	    // the 'y-2' makes the text more nicely centered in the box

    const uiSize txtsz( disp.nm_->getTextSize() );
    const int radius = txtsz.height()/7;
    disp.lithcol_->setRadius( radius );
    disp.lithcol_->setPos( mCast(float,midpt.x - txtsz.width()/2 - radius),
			   mCast(float,midpt.y) );

    leftpt.y = rightpt.y = disp.topy_;
    disp.top_->setLine( leftpt, rightpt );
    disp.top_->setZValue( 100 );

    uiRect polyrect( leftpt.x+1, disp.topy_+1, rightpt.x-1, disp.boty_-1 );
    TypeSet<uiPoint> pts;
    mDynamicCastGet(const RangeProperty*,rgprop,&thprop)
    if ( !rgprop || mIsEqual(th0,th1,1e-5) )
    {
	pts += polyrect.bottomLeft();
	pts += polyrect.topLeft();
	pts += polyrect.topRight();
	pts += polyrect.bottomRight();
	pts += polyrect.bottomLeft();
    }
    else
    {
	const int nypix = (int)(minth * pixperm + .5);
	const int nxpix = mNINT32((maxth/(maxth-minth)) * polyrect.width());
	if ( growing )
	{
	    pts += polyrect.topRight();
	    pts += polyrect.bottomRight();
	    if ( nypix < 0 )
		pts += uiPoint( polyrect.right() - nxpix, polyrect.top() );
	    else
	    {
		pts += uiPoint( polyrect.left(), polyrect.top() + nypix );
		pts += polyrect.topLeft();
	    }
	    pts += polyrect.topRight();
	}
	else
	{
	    pts += polyrect.bottomLeft();
	    pts += polyrect.topLeft();
	    if ( nypix < 0 )
		pts += uiPoint( polyrect.left() + nxpix, polyrect.top() );
	    else
	    {
		pts += polyrect.topRight();
		pts += uiPoint( polyrect.right(), polyrect.top() + nypix );
	    }
	    pts += polyrect.bottomLeft();
	}
    }

    disp.poly_->setPolygon( pts );
}


void uiBasicLayerSequenceGenDesc::descHasChanged()
{
    putTopDepthToScreen();
    rebuildDispUnits();
    reDraw(nullptr);
}


uiBasicLayerSequenceGenDesc::DispUnit* uiBasicLayerSequenceGenDesc::curUnit()
{
    const int idx = curUnitIdx();
    return idx < 0 ? 0 : disps_[idx];
}


int uiBasicLayerSequenceGenDesc::curUnitIdx()
{
    for ( int idx=0; idx<disps_.size(); idx++ )
    {
	if ( clickpos_.y < disps_[idx]->boty_ )
	    return idx;
    }
    return disps_.size() - 1;
}


class uiSimpPropertyEd : public uiGroup
{ mODTextTranslationClass(uiSimpPropertyEd);
public:

uiSimpPropertyEd( uiParent* p, const Property& prop )
    : uiGroup(p,prop.ref().name())
    , pruom_(prop.ref().unit())
{
    const PropertyRef& pr = prop.ref();

    const char* opts[] = { "Value", "Range", nullptr };
    typfld_ = new uiComboBox(this, opts, BufferString(pr.name(), " type"));
    typfld_->setHSzPol( uiObject::Medium );
    mAttachCB( typfld_->selectionChanged, uiSimpPropertyEd::updDisp );

    prelbl_ = new uiLabel( this, toUiString(pr.name()), typfld_ );
    valfld_ = new uiGenInput( this, uiString::emptyString(), FloatInpSpec() );
    rgfld_ = new uiGenInput( this, uiString::emptyString(), FloatInpSpec(),
			     FloatInpSpec() );
    uiUnitSel::Setup ussu( pr.stdType() );
    ussu.mode( uiUnitSel::Setup::SymbolsOnly );
    unfld_ = new uiUnitSel( this, ussu );
    unfld_->setUnit( prop.ref().unit() );
    mAttachCB( unfld_->selChange, uiSimpPropertyEd::unitChgCB );

    valfld_->attach( rightOf, typfld_ );
    rgfld_->attach( rightOf, typfld_ );
    unfld_->attach( rightOf, rgfld_ );

    setFrom( prop );
    setHAlignObj( valfld_ );

    mAttachCB( postFinalize(), uiSimpPropertyEd::updDisp );
}

~uiSimpPropertyEd()
{
    detachAllNotifiers();
}

bool isRg() const
{
    return typfld_->currentItem() == 1;
}

void updDisp( CallBacker* )
{
    const bool isrg = isRg();
    valfld_->display( !isrg );
    rgfld_->display( isrg );
}


void unitChgCB( CallBacker* cb )
{
    if ( !cb )
	return;

    mCBCapsuleUnpack(const UnitOfMeasure*,prevuom,cb);
    const UnitOfMeasure* curuom = unfld_->getUnit();
    if ( prevuom == curuom )
	return;

    float val = valfld_->getFValue();
    Interval<float> rg = rgfld_->getFInterval();
    convValue( val, prevuom, curuom );
    convValue( rg.start, prevuom, curuom );
    convValue( rg.stop, prevuom, curuom );

    valfld_->setValue( val );
    rgfld_->setValue( rg.start, 0 );
    rgfld_->setValue( rg.stop, 1 );
}


void setFrom( const Property& prop )
{
    const UnitOfMeasure* un = unfld_->getUnit();
    if ( prop.isRange() )
    {
	mDynamicCastGet(const RangeProperty*,rgprop,&prop)
	typfld_->setCurrentItem( 1 );
	setFldVal( rgfld_, rgprop->rg_.start, un, 0 );
	setFldVal( rgfld_, rgprop->rg_.stop, un, 1 );
	const float val =  mIsUdf(rgprop->rg_.start)	? rgprop->rg_.stop
			: (mIsUdf(rgprop->rg_.stop)	? rgprop->rg_.start
							: rgprop->rg_.center());
	setFldVal( valfld_, val, un, 0 );
    }
    else
    {
	typfld_->setCurrentItem( 0 );
	const float val = prop.value( mPropertyEvalAvg );
	setFldVal( rgfld_, val, un, 0 );
	setFldVal( rgfld_, val, un, 1 );
	setFldVal( valfld_, val, un, 0 );
    }
}


void setFldVal( uiGenInput* inp, float val, const UnitOfMeasure* un, int fldnr )
{
    const float newval = getConvertedValue( val, pruom_, un );
    inp->setValue( newval, fldnr );
}


bool getRange()
{
    const UnitOfMeasure* un = unfld_->getUnit();
    if ( isRg() )
    {
	Interval<float> rg = rgfld_->getFInterval();
	if ( mIsUdf(rg.start) || mIsUdf(rg.stop) )
	    return false;

	rg_.start = getConvertedValue( rg.start, un, pruom_ );
	rg_.stop = getConvertedValue( rg.stop, un, pruom_ );
    }
    else
    {
	const float val = valfld_->getFValue();
	if ( mIsUdf(val) )
	    return false;
	rg_.start = getConvertedValue( val, un, pruom_ );
    }

    return true;
}

bool setProp( PropertySet& props, int idx )
{
    if ( !getRange() )
	return false;

    const Property& oldprop = *props.get( idx );
    Property* newprop = isRg()
		? (Property*)new RangeProperty( oldprop.ref(), rg_ )
		: (Property*)new ValueProperty( oldprop.ref(), rg_.start );
    delete props.replace( idx, newprop );

    return true;
}

    uiLabel*		prelbl_;
    uiLabel*		postlbl_;
    uiComboBox*		typfld_;
    uiGenInput*		valfld_;
    uiGenInput*		rgfld_;
    uiUnitSel*		unfld_;
    const UnitOfMeasure* pruom_;

    Interval<float>	rg_;

};


class uiSingleLayerGeneratorEd : public uiDialog
{ mODTextTranslationClass(uiSingleLayerGeneratorEd);
public:

uiSingleLayerGeneratorEd( uiParent* p, Strat::LayerGenerator* inpun,
			  const Strat::RefTree& rt,
			  const PropertyRefSelection& proprefs,
			  const Strat::SingleLayerGenerator* nearun=0 )
    : uiDialog(p,uiDialog::Setup(inpun ? tr("Edit layer") : tr("Create layer"),
				 tr("Define layer generation"),
				 mODHelpKey(mSingleLayerGeneratorEdHelpID) ))
    , inpun_(inpun)
    , rt_(rt)
    , anychg_(true) //TODO really keep track of changes
{
    mDynamicCastGet(Strat::SingleLayerGenerator*,slg,inpun)
    if ( slg )
	edun_ = slg;
    else
	edun_ = new Strat::SingleLayerGenerator;

    uiStratSelUnits::Setup ssusu( uiStratSelUnits::Single,
				  Strat::UnitRefIter::Leaves );
    ssusu.fldtxt( "Layer" );
    unfld_ = new uiStratSelUnits( this, rt_, ssusu );
    if ( unfld_->isPresent(edun_->unit()) )
	unfld_->setCurrent( edun_->unit() );
    else
    {
	if ( nearun )
	    unfld_->setCurrent( nearun->unit() );
	unfld_->setExpanded();
    }

    uiGroup* propgrp = new uiGroup( this, "Property edit" );
    const PropertySet& props = edun_->properties();
    propflds_.allowNull( true );
    uiSimpPropertyEd* prevfld = 0;
    for ( int iprop=0; iprop<proprefs.size(); iprop++ )
    {
	const PropertyRef& pr = *proprefs[iprop];
	const Property* prop = props.getByName( pr.name(), false );
	if ( prop )
	    workprops_.add( prop->clone() );
	else if ( pr.hasFixedDef() )
	    workprops_.add( pr.fixedDef().clone() );
	else
	{
	    Property* toadd = nullptr;
	    if ( pr.disp_.defval_ )
	    {
		const BufferString typ = pr.disp_.defval_->type();
		if ( typ == ValueProperty::typeStr()
		  || typ == RangeProperty::typeStr() )
		    toadd = pr.disp_.defval_->clone();
	    }
	    if ( !toadd )
	    {
		float defval = pr.disp_.commonValue();
		if ( nearun )
		{
		    const Property* nearunprop =
			    nearun->properties().getByName( pr.name(), false );
		    if ( nearunprop )
		    {
			const float newdv =
					nearunprop->value( mPropertyEvalAvg );
			if ( !mIsUdf(newdv) )
			    defval = newdv;
		    }
		}
		toadd = new ValueProperty( pr, defval );
	    }
	    workprops_.add( toadd );
	}

	uiSimpPropertyEd* fld = nullptr;
	if ( !pr.hasFixedDef() )
	{
	    fld = new uiSimpPropertyEd( propgrp, *workprops_.get(iprop) );
	    if ( prevfld )
		fld->attach( alignedBelow, prevfld );
	    prevfld = fld;
	}
	propflds_ += fld;
    }

    contfld_ = new uiStratLayerContent( propgrp, false, rt_ );
    contfld_->set( edun_->content() );
    if ( prevfld )
	contfld_->attach( alignedBelow, prevfld );

    propgrp->attach( centeredRightOf, unfld_ );
}

bool rejectOK( CallBacker* ) override
{
    if ( inpun_ != edun_ )
	delete edun_;
    anychg_ = false;
    return true;
}

bool acceptOK( CallBacker* ) override
{
    for ( int idx=0; idx<workprops_.size(); idx++ )
    {
	uiSimpPropertyEd* fld = propflds_[idx];
	if ( fld && !fld->setProp(workprops_,idx) )
	{
	    const Property& prop = *workprops_.get(idx);
	    uiMSG().error(tr("Please fill the values for '%1'")
			.arg(prop.name()));
	    return false;
	}
    }

    const Strat::UnitRef* ur = unfld_->firstChosen();
    if ( !ur || !ur->isLeaf() )
	{ uiMSG().error(tr("Please select the layer")); return false; }

    edun_->setUnit( static_cast<const Strat::LeafUnitRef*>(ur) );
    edun_->properties() = workprops_;
    edun_->setContent( contfld_->get() );

    return true;
}

    uiStratSelUnits*		unfld_;
    ObjectSet<uiSimpPropertyEd>	propflds_;
    uiStratLayerContent*	contfld_;

    const Strat::LayerGenerator* inpun_;
    Strat::SingleLayerGenerator* edun_;
    const Strat::RefTree&	rt_;

    PropertySet			workprops_;
    bool			anychg_;

};


bool uiBasicLayerSequenceGenDesc::newLayGenReq( bool above )
{
    const int curunidx = curUnitIdx();
    uiSingleLayerGeneratorEd dlg( parent(), 0, editdesc_.refTree(),
				  editdesc_.propSelection(),
				  curunidx < 0 ? 0 : disps_[curunidx]->gen_ );
    if ( !dlg.go() )
	return false;

    const int curidx = curUnitIdx();
    const int newidx = above ? curidx : curidx + 1;
    if ( editdesc_.isEmpty() || newidx >= editdesc_.size() )
	editdesc_ += dlg.edun_;
    else
	editdesc_.insertAt( dlg.edun_, newidx );
    insertDispUnit( *dlg.edun_, newidx );

    return true;
}


bool uiBasicLayerSequenceGenDesc::laygenEditReq()
{
    const int curidx = curUnitIdx();
    if ( curidx < 0 ) return false;

    uiSingleLayerGeneratorEd dlg( parent(), editdesc_[curidx],
				  editdesc_.refTree(),
				  editdesc_.propSelection() );
    if ( !dlg.go() )
	return false;

    if ( dlg.anychg_ )
	rebuildDispUnits();
    return dlg.anychg_;
}


bool uiBasicLayerSequenceGenDesc::laygenRemoveReq()
{
    const int curidx = curUnitIdx();
    if ( curidx < 0 ) return false;

    delete editdesc_.removeSingle( curidx );
    delete disps_.removeSingle( curidx );
    return true;
}
