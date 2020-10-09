/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "uistratlaycontent.h"
#include "uistratbasiclayseqgendesc.h"
#include "uistratsimplelaymoddisp.h"
#include "uimanprops.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uigraphicsview.h"
#include "uistratselunits.h"
#include "uidialog.h"
#include "uilineedit.h"
#include "uimenu.h"
#include "uigeninput.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiunitsel.h"
#include "keystrs.h"
#include "keyenum.h"
#include "mathproperty.h"
#include "od_helpids.h"
#include "stratlayermodel.h"
#include "stratsinglaygen.h"
#include "stratlayseqgendesc.h"
#include "stratreftree.h"
#include "stratcontent.h"
#include "stratunitrefiter.h"
#include "unitofmeasure.h"


mImplClassFactory( uiLayerSequenceGenDesc, factory )


uiStratLayerContent::uiStratLayerContent( uiParent* p, bool isfinal,
				  const RefTree& srt )
    : uiGroup(p,"Layer content")
    , rt_(srt)
    , contentSelected(this)
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this,
			isfinal ? uiStrings::sContent() : tr("Content zone") );
    fld_ = lcb->box();
    fld_->addItem( toUiString("-") );
    for ( int idx=0; idx<rt_.contents().size(); idx++ )
	fld_->addItem( toUiString(rt_.contents()[idx]->name()) );
    setHAlignObj( lcb );
    fld_->selectionChanged.notify( mCB(this,uiStratLayerContent,contSel) );
}


void uiStratLayerContent::set( const Content& c )
{
    if ( c.isUnspecified() )
	fld_->setCurrentItem( 0 );
    else
	fld_->setCurrentItem( c.name() );
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
	return Content::unspecified();

    return *rt_.contents()[selidx-1];
}



uiLayerSequenceGenDesc::uiLayerSequenceGenDesc( Strat::LayerSequenceGenDesc& d )
    : desc_(d)
    , needsave_(false)
{
    if ( desc_.propSelection().size() < 2 )
    {
	PropertyRefSelection prs( desc_.propSelection() );
	int pidx = PROPS().indexOf( PropertyRef::Den );
	if ( pidx >= 0 ) prs += PROPS()[pidx];
	pidx = PROPS().indexOf( PropertyRef::Vel );
	if ( pidx >= 0 ) prs += PROPS()[pidx];
	pidx = PROPS().indexOf( PropertyRef::Imp );
	if ( pidx >= 0 ) prs += PROPS()[pidx];
	desc_.setPropSelection( prs );
    }

}

#define mErrRet( msg ) \
{ gUiMsg(getUiParent()).error( msg ); return false; }


bool uiLayerSequenceGenDesc::isValidSelection(
	const PropertyRefSelection& props ) const
{
    if ( props.isEmpty() )
	mErrRet( tr("No property is selected.") )
    if ( !props[0]->isThickness() )
    {
	pErrMsg( "Thickness should always be the first property" );
	return false;
    }

    PropertyRefSelection densityprops = props.subselect( PropertyRef::Den );
    if ( densityprops.isEmpty() )
	mErrRet( tr("No property of type 'Density' selected") )
    PropertyRefSelection velocityprops = props.subselect( PropertyRef::Vel );
    if ( velocityprops.isEmpty() )
	mErrRet( tr("No property of type 'Velocity' selected") )
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


uiExtLayerSequenceGenDesc::uiExtLayerSequenceGenDesc( uiParent* p,
					Strat::LayerSequenceGenDesc& dsc )
    : uiGroup(p,"LayerSequence Gen Desc editor")
    , uiLayerSequenceGenDesc(dsc)
    , border_(10)
    , outeritm_(0)
    , emptyitm_(0)
    , editdesc_(*new Strat::LayerSequenceGenDesc(dsc))
    , zinft_(SI().depthsInFeet())
{
    nmfld_ = new uiLineEdit( this, StringInpSpec(), sKey::Name() );
    nmfld_->setHSzPol( uiObject::Small );
    nmfld_->setSensitive( false );

    const uiString lbltxt = uiStrings::sTop().withSurvXYUnit();
    topdepthfld_ = new uiGenInput( this, lbltxt, FloatInpSpec(0) );
    topdepthfld_->setElemSzPol( uiObject::Small );
    topdepthfld_->attach( rightOf, nmfld_ );

    gv_ = new uiGraphicsView( this, "LaySeq GenDesc Viewer" );
    gv_->setScrollBarPolicy( false, uiGraphicsViewBase::ScrollBarAsNeeded );
    gv_->setPrefWidth( 200 );
    gv_->setPrefHeight( 500 );

    gv_->reSize.notify( mCB(this,uiExtLayerSequenceGenDesc,reDrawCB) );
    gv_->reDrawNeeded.notify( mCB(this,uiExtLayerSequenceGenDesc,reDrawCB) );

    gv_->getNavigationMouseEventHandler().wheelMove.notify(
			    mCB(this,uiExtLayerSequenceGenDesc,wheelMoveCB) );
    gv_->getMouseEventHandler().buttonReleased.notify(
			    mCB(this,uiExtLayerSequenceGenDesc,singClckCB) );
    gv_->getMouseEventHandler().doubleClick.notify(
			    mCB(this,uiExtLayerSequenceGenDesc,dblClckCB) );
    gv_->attach( ensureBelow, nmfld_ );
}


uiStratLayerModelDisp* uiExtLayerSequenceGenDesc::getLayModDisp(
	    uiStratLayModEditTools& lmt, Strat::LayerModelSuite& lms, int )
{
    return new uiStratSimpleLayerModelDisp( lmt, lms );
}


void uiExtLayerSequenceGenDesc::setDescID( const DBKey& dbky )
{
    descid_ = dbky;
    putTopInfo();
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


uiGraphicsScene& uiExtLayerSequenceGenDesc::scene()
{
    return gv_->scene();
}


void uiExtLayerSequenceGenDesc::reDrawCB( CallBacker* )
{
    const uiRect scenerect = gv_->getSceneRect();
    uiRect& wr = workrect_;
    wr.setLeft( border_.left() );
    wr.setRight( (int)(scenerect.width() - border_.right() + .5) );
    wr.setTop( border_.top() );
    wr.setBottom( (int)(scenerect.height() - border_.bottom() + .5) );

    if ( !outeritm_ )
    {
	outeritm_ = scene().addItem( new uiRectItem );
	outeritm_->setPenColor( Color::Black() );
	outeritm_->setZValue( mUdf(int) );
    }
    outeritm_->setRect( workrect_.left(), workrect_.top(),
			workrect_.width(), workrect_.height() );
    putTopInfo();

    if ( editdesc_.isEmpty() )
    {
	if ( !emptyitm_ )
	{
	    emptyitm_ = scene().addItem( new uiTextItem( tr("<Click to add>"),
					mAlignment(HCenter,VCenter) ) );
	    emptyitm_->setPenColor( Color::Black() );
	    emptyitm_->setPos( workrect_.centre() );
	}
    }
    else
    {
	delete emptyitm_; emptyitm_ = 0;
	doDraw();
    }
}


void uiExtLayerSequenceGenDesc::putTopInfo()
{
    float topz = editdesc_.startDepth();
    if ( zinft_ )
	topz *= mToFeetFactorF;
    topdepthfld_->setValue( topz );

    BufferString descnm( descid_.name() );
    if ( descnm.isEmpty() )
	descnm.set( "<new>" );
    nmfld_->setText( descnm );
    nmfld_->setToolTip( toUiString(descnm) );
}


void uiExtLayerSequenceGenDesc::setDescStartDepth()
{
    float topz = topdepthfld_->getFValue();
    if ( mIsUdf(topz) )
	return;

    if ( zinft_ )
	topz *= mFromFeetFactorF;

    editdesc_.setStartDepth( topz );
}


void uiExtLayerSequenceGenDesc::wheelMoveCB( CallBacker* cb )
{
    MouseEventHandler& mevh = gv_->getNavigationMouseEventHandler();
    if ( !mevh.hasEvent() || mevh.isHandled() )
	return;
    uiRect scenerect = gv_->getSceneRect();
    const uiSize scnrectsz = scenerect.size();
    const MouseEvent& mev = mevh.event();
    uiPoint prevmousescnpos = mev.pos();
    const float relscnposy = mCast(float,(prevmousescnpos.y_-scenerect.top()))/
			     mCast(float,scenerect.height());

    const double dheight = scnrectsz.height() * 0.2;
    uiSize dsize( 0, mNINT32(dheight) );
    if ( mev.angle() < 0 )
	scenerect += dsize;
    else
	scenerect -= dsize;
    gv_->setSceneRect( scenerect );
    const int newmousescnposy =
	scenerect.top()+(mNINT32(relscnposy*scenerect.height()));
    const int translate = newmousescnposy - prevmousescnpos.y_;
    const int newcentery = scenerect.centre().y_ + translate;
    reDrawCB( 0 );
    gv_->centreOn( uiPoint(scenerect.centre().x_,newcentery) );
}


void uiExtLayerSequenceGenDesc::hndlClick( CallBacker* cb, bool dbl )
{
    MouseEventHandler& mevh = gv_->getMouseEventHandler();
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
	uiMenu mnu( this, uiStrings::sAction() );
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
	{ needsave_ = true; reDrawCB(0); }

    mevh.setHandled( true );
}


uiBasicLayerSequenceGenDesc::DispUnit::DispUnit( uiGraphicsScene& scn,
				    const Strat::LayerGenerator& lg )
    : nm_(0)
    , scene_(scn)
    , genmine_(false)
{
    mDynamicCastGet(const Strat::SingleLayerGenerator*,slg,&lg)
    if ( slg )
	gen_ = slg;
    else
    {
	Strat::SingleLayerGenerator* newgen = new Strat::SingleLayerGenerator;
	gen_ = newgen; genmine_ = true;
	Property& pr = newgen->properties().get(0);
	mDynamicCastGet(ValueProperty*,vpr,&pr)
	vpr->val_ = lg.dispThickness();
     }

    nm_ = scene_.addItem( new uiTextItem( toUiString(gen_->name()),
					   mAlignment(HCenter,VCenter) ) );
    nm_->setPenColor( Color::Black() );
    lithcol_ = scene_.addItem( new uiCircleItem );
    const Color lithcolor( gen_->unit().dispColor( true ) );
    lithcol_->setPenColor( lithcolor );
    lithcol_->setFillColor( lithcolor );
    top_ = scene_.addItem( new uiLineItem );
    top_->setPenStyle( OD::LineStyle(OD::LineStyle::Solid) );
    poly_ = scene_.addItem( new uiPolygonItem );
    const OD::LineStyle
	ls( OD::LineStyle::Solid, 2, gen_->unit().dispColor(false) );;
    poly_->setPenStyle( ls );
}


uiBasicLayerSequenceGenDesc::DispUnit::~DispUnit()
{
    if ( genmine_ )
	delete const_cast<Strat::SingleLayerGenerator*>(gen_);
    delete nm_; delete lithcol_; delete top_; delete poly_;
}


uiBasicLayerSequenceGenDesc::uiBasicLayerSequenceGenDesc( uiParent* p,
				Strat::LayerSequenceGenDesc& d )
    : uiExtLayerSequenceGenDesc(p,d)
{
    rebuildDispUnits();
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
    float totth = 0;
    for ( int idx=0; idx<disps_.size(); idx++ )
	totth += disps_[idx]->gen_->dispThickness();
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
    uiPoint midpt( workrect_.centre().x_, 0 );
    uiPoint leftpt( workrect_.left(), 0 );
    uiPoint rightpt( workrect_.right(), 0 );

    DispUnit& disp = *disps_[idx];
    if ( disp.gen_->properties().isEmpty() )
	return;
    const Property& thprop = disp.gen_->properties().get( 0 );
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
    midpt.y_ = (disp.topy_ + disp.boty_) / 2;
    disp.nm_->setPos( mCast(float,midpt.x_), mCast(float,midpt.y_-2) );
	    // the 'y-2' makes the text more nicely centered in the box

    const uiSize txtsz( disp.nm_->getTextSize() );
    const int radius = txtsz.height()/7;
    disp.lithcol_->setRadius( radius );
    disp.lithcol_->setPos( mCast(float,midpt.x_ - txtsz.width()/2 - radius),
			   mCast(float,midpt.y_) );

    leftpt.y_ = rightpt.y_ = disp.topy_;
    disp.top_->setLine( leftpt, rightpt );
    disp.top_->setZValue( 100 );

    uiRect polyrect( leftpt.x_+1, disp.topy_+1, rightpt.x_-1, disp.boty_-1 );
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
    putTopInfo();
    rebuildDispUnits();
    reDrawCB(0);
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
	if ( clickpos_.y_ < disps_[idx]->boty_ )
	    return idx;
    }
    return disps_.size() - 1;
}


class uiSimpPropertyEd : public uiGroup
{ mODTextTranslationClass(uiSimpPropertyEd);
public:

uiSimpPropertyEd( uiParent* p, const Property& prop )
    : uiGroup(p,prop.ref().name())
{
    const PropertyRef& pr = prop.ref();

    uiStringSet opts;
    opts.add( uiStrings::sValue() ).add( uiStrings::sRange() );
    typfld_ = new uiComboBox(this, opts, BufferString(pr.name()," type"));
    typfld_->selectionChanged.notify( mCB(this,uiSimpPropertyEd,updDisp) );
    typfld_->setHSzPol( uiObject::Small );
    prelbl_ = new uiLabel( this, toUiString(pr.name()), typfld_ );
    valfld_ = new uiGenInput( this, uiString::empty(), FloatInpSpec() );
    rgfld_ = new uiGenInput( this, uiString::empty(), FloatInpSpec(),
                             FloatInpSpec() );
    uiUnitSel::Setup ussu( pr.stdType() ); ussu.withnone( true );
    unfld_ = new uiUnitSel( this, ussu );

    valfld_->attach( rightOf, typfld_ );
    rgfld_->attach( rightOf, typfld_ );
    unfld_->attach( rightOf, rgfld_ );

    setFrom( prop );
    postFinalise().notify( mCB(this,uiSimpPropertyEd,updDisp) );
    setHAlignObj( valfld_ );
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


void setFldVal( uiGenInput* inp, float val, const UnitOfMeasure* un, int fldnr )
{
    const bool isudf = mIsUdf(val);
    if ( un && !isudf )
	val = un->getUserValueFromSI( val );
    inp->setValue( val, fldnr );
}


void setFrom( const Property& prop )
{
    const UnitOfMeasure* un = unfld_->getUnit();
    mDynamicCastGet(const RangeProperty*,rgprop,&prop)
    if ( rgprop )
    {
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
	float val = prop.value( mPropertyEvalAvg );
	setFldVal( rgfld_, val, un, 0 );
	setFldVal( rgfld_, val, un, 1 );
	setFldVal( valfld_, val, un, 0 );
    }
}

bool getRange()
{
    const UnitOfMeasure* un = unfld_->getUnit();
    if ( !isRg() )
    {
	const float val = valfld_->getFValue();
	if ( mIsUdf(val) )
	    return false;
	rg_.start = !un ? val : un->getSIValue( val );
    }
    else
    {
	Interval<float> rg = rgfld_->getFInterval();
	if ( mIsUdf(rg.start) || mIsUdf(rg.stop) )
	    return false;
	rg_ = rg;
	if ( un )
	{
	    rg_.start = un->getSIValue( rg_.start );
	    rg_.stop = un->getSIValue( rg_.stop );
	}
    }
    return true;
}

bool setProp( PropertySet& props, int idx )
{
    if ( !getRange() ) return false;

    const Property& oldprop = props.get( idx );
    Property* newprop = isRg()
		? (Property*)new RangeProperty( oldprop.ref(), rg_ )
		: (Property*)new ValueProperty( oldprop.ref(), rg_.start );
    props.replace( idx, newprop );

    return true;
}

    uiLabel*		prelbl_;
    uiLabel*		postlbl_;
    uiComboBox*		typfld_;
    uiGenInput*		valfld_;
    uiGenInput*		rgfld_;
    uiUnitSel*		unfld_;

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
    propflds_.setNullAllowed( true );
    uiSimpPropertyEd* prevfld = 0;
    for ( int iprop=0; iprop<proprefs.size(); iprop++ )
    {
	const PropertyRef& pr = *proprefs[iprop];

	const int idxof = props.indexOf( pr );
	if ( idxof >= 0 )
	    workprops_.add( props.get(idxof).clone() );
	else if ( pr.hasFixedDef() )
	    workprops_.add( pr.fixedDef().clone() );
	else
	{
	    Property* toadd = 0;
	    if ( pr.defval_ )
	    {
		const BufferString typ = pr.defval_->type();
		if ( typ == ValueProperty::typeStr()
		  || typ == RangeProperty::typeStr() )
		    toadd = pr.defval_->clone();
	    }
	    if ( !toadd )
	    {
		float defval = pr.commonValue();
		if ( nearun )
		{
		    const int nidxof = nearun->properties().indexOf( pr );
		    if ( nidxof >= 0 )
		    {
			const float newdv = nearun->properties().get(nidxof)
						.value( mPropertyEvalAvg );
			if ( !mIsUdf(newdv) )
			    defval = newdv;
		    }
		}
		toadd = new ValueProperty( pr, defval );
	    }
	    workprops_.add( toadd );
	}

	uiSimpPropertyEd* fld = 0;
	if ( !pr.hasFixedDef() )
	{
	    fld = new uiSimpPropertyEd( propgrp, workprops_.get(iprop) );
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

bool rejectOK()
{
    if ( inpun_ != edun_ )
	delete edun_;
    anychg_ = false;
    return true;
}

bool acceptOK()
{
    for ( int idx=0; idx<workprops_.size(); idx++ )
    {
	uiSimpPropertyEd* fld = propflds_[idx];
	if ( fld && !fld->setProp(workprops_,idx) )
	{
	    const Property& prop = workprops_.get(idx);
	    uiMSG().error(tr("Please fill the values for '%1'")
			.arg(prop.name()));
	    return false;
	}
    }

    const Strat::UnitRef* ur = unfld_->firstChosen();
    if ( !ur || !ur->isLeaf() )
	{ uiMSG().error(uiStrings::phrSelect(tr("the layer"))); return false; }

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
