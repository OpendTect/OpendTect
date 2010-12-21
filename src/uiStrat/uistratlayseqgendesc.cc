/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uistratlayseqgendesc.cc,v 1.15 2010-12-21 15:01:25 cvsbert Exp $";

#include "uistratsinglayseqgendesc.h"
#include "uigraphicsitemimpl.h"
#include "uigraphicsscene.h"
#include "uidialog.h"
#include "uimenu.h"
#include "uigeninput.h"
#include "uicombobox.h"
#include "uilabel.h"
#include "uimsg.h"
#include "stratlayermodel.h"
#include "stratsinglaygen.h"
#include "stratreftree.h"
#include "stratunitrefiter.h"
#include "survinfo.h"
#include "propertyimpl.h"
#include "keyenum.h"


mImplFactory2Param(uiLayerSequenceGenDesc,uiParent*,
	Strat::LayerSequenceGenDesc&,uiLayerSequenceGenDesc::factory)


uiLayerSequenceGenDesc::uiLayerSequenceGenDesc( uiParent* p,
					Strat::LayerSequenceGenDesc& desc )
    : uiGraphicsView(p,"LayerSequence Gen Desc editor")
    , desc_(desc)
    , border_(10)
    , outeritm_(0)
    , emptyitm_(0)
    , needsave_(false)
{
    setPrefWidth( 200 );
    setPrefHeight( 500 );
    reSize.notify( mCB(this,uiLayerSequenceGenDesc,reDraw) );
    reDrawNeeded.notify( mCB(this,uiLayerSequenceGenDesc,reDraw) );

    getMouseEventHandler().buttonReleased.notify(
	    			mCB(this,uiLayerSequenceGenDesc,singClckCB) );
    getMouseEventHandler().doubleClick.notify(
	    			mCB(this,uiLayerSequenceGenDesc,dblClckCB) );
}


void uiLayerSequenceGenDesc::hndlClick( CallBacker* cb, bool dbl )
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
    if ( workrect_.isOutside(clickpos_) )
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
	ischgd = descEditReq();
    else if ( mnuid == 1 || mnuid == 2 )
	ischgd = newDescReq( mnuid == 1 );
    else if ( mnuid == 3 )
	ischgd = descRemoveReq();

    if ( ischgd )
	{ needsave_ = true; reDraw(0); }

    mevh.setHandled( true );
}


void uiLayerSequenceGenDesc::reDraw( CallBacker* )
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


uiSingleLayerSequenceGenDesc::DispUnit::DispUnit( uiGraphicsScene& scn,
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
    top_ = scene_.addItem( new uiLineItem );
    top_->setPenStyle( LineStyle(LineStyle::Solid) );
    poly_ = scene_.addItem( new uiPolygonItem );

    LineStyle ls;
    if ( gen_->unit().upNode() )
	ls.color_ = gen_->unit().upNode()->color();
    poly_->setPenStyle( ls );
}


uiSingleLayerSequenceGenDesc::DispUnit::~DispUnit()
{
    if ( genmine_ )
	delete const_cast<Strat::SingleLayerGenerator*>(gen_);
    delete nm_; delete top_; delete poly_;
}


uiSingleLayerSequenceGenDesc::uiSingleLayerSequenceGenDesc( uiParent* p,
	Strat::LayerSequenceGenDesc& d )
    : uiLayerSequenceGenDesc(p,d)
{
    props_ += &Strat::Layer::thicknessRef();
    int idx = ePROPS().ensurePresent( PropertyRef::Vel, "Velocity", "vel" );
    props_ += PROPS()[idx];
    idx = ePROPS().ensurePresent( PropertyRef::Den, "Density", "den", "RohB" );
    props_ += PROPS()[idx];
    idx = ePROPS().ensurePresent( PropertyRef::AI, "Acoustic Impedance", "AI" );
    props_ += PROPS()[idx];

    rebuildDispUnits();
}


void uiSingleLayerSequenceGenDesc::rebuildDispUnits()
{
    for ( int idx=0; idx<desc_.size(); idx++ )
	insertDispUnit( *desc_[idx], idx );
}


void uiSingleLayerSequenceGenDesc::insertDispUnit(
			    const Strat::LayerGenerator& lgen, int newidx )
{
    DispUnit* newdisp = new DispUnit( scene(), lgen );
    if ( newidx < 0 || newidx >= disps_.size() )
	disps_ += newdisp;
    else
	disps_.insertAt( newdisp, newidx );
}


void uiSingleLayerSequenceGenDesc::doDraw()
{
    if ( disps_.isEmpty() ) return;
    float totth = 0;
    for ( int idx=0; idx<disps_.size(); idx++ )
	totth += disps_[idx]->gen_->dispThickness();
    if ( mIsZero(totth,mDefEps) ) return;

    float curz = 0;
    const float pixperm = workrect_.height() / totth;
    uiPoint midpt( workrect_.centre().x, 0 );
    uiPoint leftpt( workrect_.left(), 0 );
    uiPoint rightpt( workrect_.right(), 0 );
    for ( int idx=0; idx<disps_.size(); idx++ )
    {
	DispUnit& disp = *disps_[idx];
	const Property& prop = disp.gen_->properties().get(0);
	const float th0 = prop.value( Property::EvalOpts(false,0) );
	const float th1 = prop.value( Property::EvalOpts(false,1) );
	const float maxth = th0 > th1 ? th0 : th1;
	disp.topy_ = (int)(workrect_.top() + curz * pixperm);
	disp.boty_ = (int)(workrect_.top() + (curz+maxth) * pixperm);

	midpt.y = (disp.topy_ + disp.boty_) / 2;
	disp.nm_->setPos( midpt );
	disp.nm_->setText( disp.gen_->name() );

	leftpt.y = rightpt.y = disp.topy_;
	disp.top_->setLine( leftpt, rightpt );

	uiRect polyrect( leftpt.x+1, disp.topy_+1, rightpt.x-1, disp.boty_-1 );
	TypeSet<uiPoint> pts;
	pts += polyrect.bottomLeft();
	pts += polyrect.topLeft();
	mDynamicCastGet(const RangeProperty*,rgprop,&prop)
	if ( !rgprop )
	{
	    pts += polyrect.topRight();
	    pts += polyrect.bottomRight();
	}
	else
	{
	    if ( th1 > th0 )
	    {
		pts += polyrect.topRight();
		pts += uiPoint( polyrect.right(),
				(int)(workrect_.top() + (curz+th0)*pixperm) );;
	    }
	    else
	    {
		pts += uiPoint( polyrect.right(),
			    (int)(workrect_.top() + (curz+th0-th1)*pixperm) );
		pts += polyrect.bottomRight();
	    }
	    pts += polyrect.bottomLeft();
	}

	disp.poly_->setPolygon( pts );
	curz += maxth;
    }
}


void uiSingleLayerSequenceGenDesc::descHasChanged()
{
    deepErase( disps_ );
    rebuildDispUnits();
    reDraw(0);
}


uiSingleLayerSequenceGenDesc::DispUnit* uiSingleLayerSequenceGenDesc::curUnit()
{
    const int idx = curUnitIdx();
    return idx < 0 ? 0 : disps_[idx];
}


int uiSingleLayerSequenceGenDesc::curUnitIdx()
{
    if ( disps_.isEmpty() )
	return -1;

    for ( int idx=0; idx<disps_.size(); idx++ )
    {
	DispUnit* disp = disps_[idx];
	if ( clickpos_.y < disps_[idx]->boty_ )
	    return idx;
    }
    return disps_.size() - 1;;
}


class uiSimpPropertyEd : public uiGroup
{
public:

uiSimpPropertyEd( uiParent* p, const PropertyRef& pr )
    : uiGroup(p,pr.name())
{
    const bool zinft = SI().depthsInFeetByDefault();
    fac_ = pr.hasType( PropertyRef::Den ) ? 1000 : (zinft?mFromFeetFactor:1);
    static const char* opts[] = { "Constant", "Range", 0 };
    const char* unnm = pr.hasType( PropertyRef::Den ) ? "g/cm3"
	: (zinft ? ( pr.hasType( PropertyRef::Vel ) ? "ft/s" : "ft")
		 : ( pr.hasType( PropertyRef::Vel ) ? "m/s" : "m"));

    typfld_ = new uiComboBox( this, opts, BufferString(pr.name()," type") );
    typfld_->selectionChanged.notify( mCB(this,uiSimpPropertyEd,updDisp) );
    typfld_->setPrefWidthInChar( 14 );
    prelbl_ = new uiLabel( this, pr.name(), typfld_ );
    valfld_ = new uiGenInput( this, "", FloatInpSpec() );
    rgfld_ = new uiGenInput( this, "", FloatInpSpec(), FloatInpSpec() );
    postlbl_ = new uiLabel( this, BufferString("(",unnm,")") );

    valfld_->attach( rightOf, typfld_ );
    rgfld_->attach( rightOf, typfld_ );
    postlbl_->attach( rightOf, rgfld_ );

    finaliseDone.notify( mCB(this,uiSimpPropertyEd,updDisp) );
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


void setFrom( const Property& prop, bool alwaysconst )
{
    mDynamicCastGet(const RangeProperty*,rgprop,&prop)
    if ( rgprop && !alwaysconst )
    {
	typfld_->setCurrentItem( 1 );
	Interval<float> rg( rgprop->rg_ );
	const bool startudf = mIsUdf(rg.start);
	const bool stopudf = mIsUdf(rg.stop);
	if ( !startudf ) rg.start /= fac_;
	if ( !stopudf ) rg.stop /= fac_;
	rgfld_->setValue( rg );
	if ( startudf || stopudf )
	valfld_->setValue( startudf || stopudf ? mUdf(float) : rg.center() );
    }
    else
    {
	typfld_->setCurrentItem( 0 );
	float val = prop.value(Property::EvalOpts(true));
	const bool isudf = mIsUdf(val);
	if ( !mIsUdf(val) ) val /= fac_;
	valfld_->setValue( val );
	rgfld_->setValue( Interval<float>(val,val) );
    }
}

bool getRange()
{
    if ( !isRg() )
    {
	const float val = valfld_->getfValue();
	if ( mIsUdf(val) ) return false;
	rg_.start = val * fac_;
    }
    else
    {
	Interval<float> rg( rgfld_->getfValue(0), rgfld_->getfValue(1) );
	if ( mIsUdf(rg.start) || mIsUdf(rg.stop) ) return false;
	rg_.start = rg.start * fac_; rg_.stop = rg.stop * fac_;
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

    uiLabel*	prelbl_;
    uiLabel*	postlbl_;
    uiComboBox*	typfld_;
    uiGenInput*	valfld_;
    uiGenInput*	rgfld_;

    float		fac_;
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
				"Define layer generation",mTODOHelpID))
    , inpun_(inpun)
    , thref_(proprefs[0])
    , velref_(proprefs[1])
    , denref_(proprefs[2])
    , airef_(proprefs[3])
    , rt_(rt)
{
    mDynamicCastGet(Strat::SingleLayerGenerator*,slg,inpun)
    if ( slg )
	edun_ = slg;
    else
	edun_ = new Strat::SingleLayerGenerator;

    PropertySet& props = edun_->properties();
    int velidx = props.indexOf( *velref_ );
    if ( velidx < 0 )
    {
	velidx = props.size();
	props.add( new ValueProperty(*velref_,velref_->disp_.possibleValue()) );
    }
    int denidx = props.indexOf( *denref_ );
    if ( denidx < 0 )
    {
	denidx = props.size();
	props.add( new ValueProperty(*denref_,denref_->disp_.possibleValue()) );
    }
    int aiidx = props.indexOf( *airef_ );
    if ( aiidx < 0 )
    {
	aiidx = props.size();
	BufferString velvarnm( velref_->name() );
	BufferString denvarnm( denref_->name() );
	MathProperty::ensureGoodVariableName( velvarnm.buf() );
	MathProperty::ensureGoodVariableName( denvarnm.buf() );
	const BufferString exprstr( velvarnm, " * ", denvarnm );
	MathProperty* mp = new MathProperty( *airef_, exprstr );
	props.add( mp );
    }

    BufferStringSet unnms;
    Strat::UnitRefIter it( rt_, Strat::UnitRefIter::Leaves );
    while ( it.next() )
	unnms.add( it.unit()->fullCode() );

    unfld_ = new uiGenInput( this, "Layer", StringListInpSpec(unnms) );
    thfld_ = new uiSimpPropertyEd( this, *thref_ );
    velfld_ = new uiSimpPropertyEd( this, *velref_ );
    denfld_ = new uiSimpPropertyEd( this, *denref_ );

    thfld_->attach( alignedBelow, unfld_ );
    velfld_->attach( alignedBelow, thfld_ );
    denfld_->attach( alignedBelow, velfld_ );

    const Strat::SingleLayerGenerator* useun
				= inpun_ || !nearun ? edun_ : nearun;
    unfld_->setText( useun->unit().fullCode() );
    const PropertySet& useprops = useun->properties();
    thfld_->setFrom( useprops.get( 0 ), useun != edun_ );
    velfld_->setFrom( useprops.get( props.indexOf(*velref_) ), useun != edun_ );
    denfld_->setFrom( useprops.get( props.indexOf(*denref_) ), useun != edun_ );
}

bool rejectOK( CallBacker* )
{
    if ( inpun_ != edun_ )
	delete edun_;
    return true;
}

bool acceptOK( CallBacker* )
{
    PropertySet& props = edun_->properties();
    if ( !thfld_->setProp(edun_->properties(),0)
      || !velfld_->setProp(edun_->properties(),
	  			edun_->properties().indexOf(*velref_))
      || !denfld_->setProp(edun_->properties(),
	  			edun_->properties().indexOf(*denref_)) )
    {
	uiMSG().error( "Please fill all values" );
	return false;
    }
    const Strat::UnitRef* ur = rt_.find( unfld_->text() );
    edun_->setUnit( static_cast<const Strat::LeafUnitRef*>(ur) );
    return true;
}

    uiGenInput*		unfld_;
    uiSimpPropertyEd*	thfld_;
    uiSimpPropertyEd*	velfld_;
    uiSimpPropertyEd*	denfld_;

    const Strat::LayerGenerator* inpun_;
    Strat::SingleLayerGenerator* edun_;
    const PropertyRef*		thref_;
    const PropertyRef*		velref_;
    const PropertyRef*		denref_;
    const PropertyRef*		airef_;
    const Strat::RefTree&	rt_;

};


bool uiSingleLayerSequenceGenDesc::newDescReq( bool above )
{
    const int curunidx = curUnitIdx();
    uiSingleLayerGeneratorEd dlg( parent(), 0, desc_.refTree(), props_,
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


bool uiSingleLayerSequenceGenDesc::descEditReq()
{
    const int curidx = curUnitIdx();
    if ( curidx < 0 ) return false;

    uiSingleLayerGeneratorEd dlg( parent(), desc_[curidx], desc_.refTree(),
				  props_ );
    return dlg.go();
}


bool uiSingleLayerSequenceGenDesc::descRemoveReq()
{
    const int curidx = curUnitIdx();
    if ( curidx < 0 ) return false;

    delete desc_.remove( curidx );
    delete disps_.remove( curidx );
    return true;
}
