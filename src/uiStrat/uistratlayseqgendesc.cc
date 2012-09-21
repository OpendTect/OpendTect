/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";

#include "uistratlaycontent.h"
#include "uistratbasiclayseqgendesc.h"
#include "uistratsimplelaymoddisp.h"
#include "uimanprops.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uistratselunits.h"
#include "uidialog.h"
#include "uimenu.h"
#include "uigeninput.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiunitsel.h"
#include "stratlayermodel.h"
#include "stratsinglaygen.h"
#include "stratlayseqgendesc.h"
#include "stratreftree.h"
#include "stratcontent.h"
#include "stratunitrefiter.h"
#include "unitofmeasure.h"
#include "property.h"
#include "keyenum.h"


mImplFactory2Param(uiLayerSequenceGenDesc,uiParent*,
	Strat::LayerSequenceGenDesc&,uiLayerSequenceGenDesc::factory)


uiStratLayerContent::uiStratLayerContent( uiParent* p,
					  const Strat::RefTree* srt )
    : uiGroup(p,"Layer content")
    , rt_(srt ? *srt : Strat::RT())
    , contentSelected(this)
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Content" );
    fld_ = lcb->box();
    const Strat::RefTree& rt = srt ? *srt : Strat::RT();
    fld_->addItem( "-" );
    for ( int idx=0; idx<rt.contents().size(); idx++ )
	fld_->addItem( rt.contents()[idx]->name() );
    setHAlignObj( lcb );
    fld_->selectionChanged.notify( mCB(this,uiStratLayerContent,contSel) );
}


void uiStratLayerContent::set( const Strat::Content& c )
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
    fld_->addItem( nm );
    return fld_->size() - 1;
}


const Strat::Content& uiStratLayerContent::get() const
{
    const int selidx = fld_->currentItem();
    if ( selidx < 1 || selidx > rt_.contents().size() )
	return Strat::Content::unspecified();

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


bool uiLayerSequenceGenDesc::selProps()
{
    PropertyRefSelection prs( desc_.propSelection() );
    uiSelectPropRefs dlg( outerObj()->parent(), prs );
    const bool ret = dlg.go();
    if ( ret || dlg.structureChanged() )
	desc_.setPropSelection( prs );
    return ret;
}


uiExtLayerSequenceGenDesc::uiExtLayerSequenceGenDesc( uiParent* p,
					Strat::LayerSequenceGenDesc& dsc )
    : uiGraphicsView(p,"LayerSequence Gen Desc editor")
    , uiLayerSequenceGenDesc(dsc)
    , border_(10)
    , outeritm_(0)
    , emptyitm_(0)
{
    setPrefWidth( 200 );
    setPrefHeight( 500 );
    reSize.notify( mCB(this,uiExtLayerSequenceGenDesc,reDraw) );
    reDrawNeeded.notify( mCB(this,uiExtLayerSequenceGenDesc,reDraw) );

    getMouseEventHandler().buttonReleased.notify(
	    			mCB(this,uiExtLayerSequenceGenDesc,singClckCB) );
    getMouseEventHandler().doubleClick.notify(
	    			mCB(this,uiExtLayerSequenceGenDesc,dblClckCB) );
}


uiStratLayerModelDisp* uiExtLayerSequenceGenDesc::getLayModDisp(
			uiStratLayModEditTools& lmt, Strat::LayerModel& lm )
{
    return new uiStratSimpleLayerModelDisp( lmt, lm );
}


void uiExtLayerSequenceGenDesc::reDraw( CallBacker* )
{
    uiRect& wr = const_cast<uiRect&>( workrect_ );
    wr.setLeft( border_.left() );
    wr.setRight( (int)(width() - border_.right() + .5) );
    wr.setTop( border_.top() );
    wr.setBottom( (int)(height() - border_.bottom() + .5) );

    if ( !outeritm_ )
    {
	outeritm_ = scene().addItem( new uiRectItem );
	outeritm_->setPenColor( Color::Black() );
	outeritm_->setZValue( mUdf(int) );
    }
    outeritm_->setRect( workrect_.left(), workrect_.top(),
	    		workrect_.width(), workrect_.height() );

    if ( desc_.isEmpty() )
    {
	if ( !emptyitm_ )
	{
	    emptyitm_ = scene().addItem( new uiTextItem( "<Click to add>",
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


void uiExtLayerSequenceGenDesc::hndlClick( CallBacker* cb, bool dbl )
{
    MouseEventHandler& mevh = getMouseEventHandler();
    if ( !mevh.hasEvent() || mevh.isHandled() )
	return;

    const MouseEvent& mev = mevh.event();
    const bool isright = OD::rightMouseButton( mev.buttonState() );
    const bool isempty = desc_.isEmpty();
    const bool needhandle = isempty || (dbl && !isright) || (!dbl && isright);
    if ( !needhandle )
	return;

    clickpos_ = mev.pos();
    mevh.setHandled( true );
    if ( workrect_.isOutside(clickpos_) )
	return;
    if ( (isempty || desc_.propSelection().size() < 2) && !selProps() )
	return;

    int mnuid = dbl ? 0 : (isempty ? 1 : -1);
    if ( !isempty && !dbl )
    {
	uiPopupMenu mnu( parent(), "Action" );
	mnu.insertItem( new uiMenuItem("&Edit ..."), 0 );
	mnu.insertItem( new uiMenuItem("Add &Above ..."), 1 );
	mnu.insertItem( new uiMenuItem("Add &Below ..."), 2 );
	if ( desc_.size() > 1 )
	{
	    mnu.insertSeparator();
	    mnu.insertItem( new uiMenuItem("&Remove"), 3 );
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

    nm_ = scene_.addItem( new uiTextItem( gen_->name(),
			  mAlignment(HCenter,VCenter) ) );
    nm_->setPenColor( Color::Black() );
    lithcol_ = scene_.addItem( new uiCircleItem );
    const Color lithcolor( gen_->unit().dispColor( true ) );
    lithcol_->setPenColor( lithcolor );
    lithcol_->setFillColor( lithcolor );
    top_ = scene_.addItem( new uiLineItem );
    top_->setPenStyle( LineStyle(LineStyle::Solid) );
    poly_ = scene_.addItem( new uiPolygonItem );
    const LineStyle ls( LineStyle::Solid, 2, gen_->unit().dispColor(false) );;
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
    for ( int idx=0; idx<desc_.size(); idx++ )
	insertDispUnit( *desc_[idx], idx );
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
    if ( disps_.isEmpty() ) return;
    float totth = 0;
    for ( int idx=0; idx<disps_.size(); idx++ )
	totth += disps_[idx]->gen_->dispThickness();
    if ( mIsZero(totth,mDefEps) ) return;

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
    const Property& prop = disp.gen_->properties().get(0);
    const float th0 = prop.value( mPropertyEvalNew(0) );
    const float th1 = prop.value( mPropertyEvalNew(1) );
    const bool growing = th1 > th0;
    const float& maxth = growing ? th1 : th0;
    const float& minth = growing ? th0 : th1;
    disp.topy_ = (int)(workrect_.top() + curz * pixperm + .5);
    curz += maxth;
    disp.boty_ = (int)(workrect_.top() + curz * pixperm + .5);

    BufferString dispnm( disp.gen_->name() );
    if ( !disp.gen_->content().isUnspecified() )
	dispnm.add( "[" ).add( disp.gen_->content().name() ).add( "]" );
    disp.nm_->setText( getLimitedDisplayString(dispnm,25,false) );
    midpt.y = (disp.topy_ + disp.boty_) / 2;
    disp.nm_->setPos( midpt.x, midpt.y-2 );
	    // the 'y-2' makes the text more nicely centered in the box

    const uiSize txtsz( disp.nm_->getTextSize() );
    const int radius = txtsz.height()/7;
    disp.lithcol_->setRadius( radius );
    disp.lithcol_->setPos( midpt.x - txtsz.width()/2 - radius, midpt.y );

    leftpt.y = rightpt.y = disp.topy_;
    disp.top_->setLine( leftpt, rightpt );
    disp.top_->setZValue( 100 );

    uiRect polyrect( leftpt.x+1, disp.topy_+1, rightpt.x-1, disp.boty_-1 );
    TypeSet<uiPoint> pts;
    mDynamicCastGet(const RangeProperty*,rgprop,&prop)
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
    rebuildDispUnits();
    reDraw(0);
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
{
public:

uiSimpPropertyEd( uiParent* p, const Property& prop )
    : uiGroup(p,prop.ref().name())
{
    const PropertyRef& pr = prop.ref();

    static const char* opts[] = { "Value", "Range", 0 };
    typfld_ = new uiComboBox( this, opts, BufferString(pr.name()," type") );
    typfld_->selectionChanged.notify( mCB(this,uiSimpPropertyEd,updDisp) );
    typfld_->setHSzPol( uiObject::Small );
    prelbl_ = new uiLabel( this, pr.name(), typfld_ );
    valfld_ = new uiGenInput( this, "", FloatInpSpec() );
    rgfld_ = new uiGenInput( this, "", FloatInpSpec(), FloatInpSpec() );
    unfld_ = new uiUnitSel( this, pr.stdType(), 0, true );
    unfld_->setUnit( pr.disp_.unit_ );

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
	const float val = valfld_->getfValue();
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
{
public:

uiSingleLayerGeneratorEd( uiParent* p, Strat::LayerGenerator* inpun,
       			  const Strat::RefTree& rt,
			  const PropertyRefSelection& proprefs,
			  const Strat::SingleLayerGenerator* nearun=0 )
    : uiDialog(p,uiDialog::Setup(inpun?"Edit layer":"Create layer",
				"Define layer generation","110.2.0"))
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
    for ( int idx=0; idx<proprefs.size(); idx++ )
    {
	const PropertyRef& pr = *proprefs[idx];
	const int idxof = props.indexOf( pr );
	if ( idxof >= 0 )
	    workprops_.add( props.get(idxof).clone() );
	else
	{
	    Property* toadd = 0;
	    if ( pr.disp_.defval_ )
	    {
		const char* typ = pr.disp_.defval_->type();
		if ( !strcmp(typ,ValueProperty::typeStr())
		  || !strcmp(typ,RangeProperty::typeStr()) )
		    toadd = pr.disp_.defval_->clone();
	    }
	    if ( !toadd )
	    {
		float defval = pr.disp_.range_.center();
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

	uiSimpPropertyEd* fld = new uiSimpPropertyEd( propgrp,
						      workprops_.get(idx) );
	if ( idx > 0 )
	    fld->attach( alignedBelow, propflds_[idx-1] );
	propflds_ += fld;
    }

    contfld_ = new uiStratLayerContent( propgrp, &rt_ );
    contfld_->set( edun_->content() );
    if ( !propflds_.isEmpty() )
	contfld_->attach( alignedBelow, propflds_[ propflds_.size()-1 ] );
    propgrp->attach( centeredRightOf, unfld_ );
}

bool rejectOK( CallBacker* )
{
    if ( inpun_ != edun_ )
	delete edun_;
    anychg_ = false;
    return true;
}

bool acceptOK( CallBacker* )
{
    for ( int idx=0; idx<workprops_.size(); idx++ )
    {
	if ( !propflds_[idx]->setProp(workprops_,idx) )
	{
	    const Property& prop = workprops_.get(idx);
	    uiMSG().error( "Please fill the values for '", prop.name(), "'" );
	    return false;
	}
    }

    const Strat::UnitRef* ur = unfld_->firstSelected();
    if ( !ur || !ur->isLeaf() )
    {
	uiMSG().error( "Please select the layer" );
	return false;
    }
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
    uiSingleLayerGeneratorEd dlg( parent(), 0, desc_.refTree(),
	    			  desc_.propSelection(),
				  curunidx < 0 ? 0 : disps_[curunidx]->gen_ );
    if ( !dlg.go() )
	return false;

    const int curidx = curUnitIdx();
    const int newidx = above ? curidx : curidx + 1;
    if ( desc_.isEmpty() || newidx >= desc_.size() )
	desc_ += dlg.edun_;
    else
	desc_.insertAt( dlg.edun_, newidx );
    insertDispUnit( *dlg.edun_, newidx );

    return true;
}


bool uiBasicLayerSequenceGenDesc::laygenEditReq()
{
    const int curidx = curUnitIdx();
    if ( curidx < 0 ) return false;

    uiSingleLayerGeneratorEd dlg( parent(), desc_[curidx], desc_.refTree(),
				  desc_.propSelection() );
    const bool chgd = dlg.go() ? dlg.anychg_ : false;
    if ( chgd )
	rebuildDispUnits();
    return chgd;
}


bool uiBasicLayerSequenceGenDesc::laygenRemoveReq()
{
    const int curidx = curUnitIdx();
    if ( curidx < 0 ) return false;

    delete desc_.remove( curidx );
    delete disps_.remove( curidx );
    return true;
}
