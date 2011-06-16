/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          June 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uimanprops.cc,v 1.1 2011-06-16 15:10:06 cvsbert Exp $";

#include "uimanprops.h"
#include "uibuildlistfromlist.h"
#include "uigeninput.h"
#include "uiunitsel.h"
#include "uicolor.h"
#include "uimsg.h"
#include "propertyref.h"
#include "separstr.h"
#include "unitofmeasure.h"


class uiBuildPROPS : public uiBuildListFromList
{
public:
			uiBuildPROPS(uiParent*,const PropertyRefSet&);

    PropertyRefSet 	props_; // must be a copy

    virtual const char*	avFromDef(const char*) const;
    virtual void	editReq(bool);
    virtual void	removeReq();
    virtual void	itemSwitch(const char*,const char*);

};


uiBuildPROPS::uiBuildPROPS( uiParent* p, const PropertyRefSet& prs )
    : uiBuildListFromList(p,
	    uiBuildListFromList::Setup(true,"property type","property")
	    .withio(false).withtitles(true), "PropertyRef selection group")
    , props_(prs)
{
    const BufferStringSet dispnms( PropertyRef::StdTypeNames() );
    setAvailable( dispnms );
}


const char* uiBuildPROPS::avFromDef( const char* nm ) const
{
    const PropertyRef* pr = props_.find( nm );
    if ( !pr ) return 0;
    return PropertyRef::toString( pr->stdType() );
}


class uiEditPropRef : public uiDialog
{
public:

	    		uiEditPropRef(uiParent*,PropertyRef&,bool);
    bool		acceptOK(CallBacker*);

    PropertyRef&	pr_;

    uiGenInput*		namefld_;
    uiGenInput*		aliasfld_;
    uiGenInput*		rgfld_;
    uiUnitSel*		unfld_;
    uiColorInput*	colfld_;

};


uiEditPropRef::uiEditPropRef( uiParent* p, PropertyRef& pr, bool isadd )
    : uiDialog(p,uiDialog::Setup("Property definition",
		BufferString(isadd?"Add '":"Edit '",
		    PropertyRef::toString(pr.stdType()),"' property"),
		mTODOHelpID))
    , pr_(pr)
{
    namefld_ = new uiGenInput( this, "Name", StringInpSpec(pr.name()) );
    SeparString ss;
    for ( int idx=0; idx<pr_.aliases().size(); idx++ )
	ss += pr_.aliases().get(idx);
    aliasfld_ = new uiGenInput( this, "Aliases (e.g. 'abc, uvw*xyz')",
	   			StringInpSpec(ss.buf()) );
    aliasfld_->attach( alignedBelow, namefld_ );

    colfld_ = new uiColorInput( this,
	    			uiColorInput::Setup(pr_.disp_.color_),
				"Default display color" );
    colfld_->attach( alignedBelow, aliasfld_ );
    rgfld_ = new uiGenInput( this, "Typical value range",
			     FloatInpIntervalSpec() );
    rgfld_->attach( alignedBelow, colfld_ );
    unfld_ = new uiUnitSel( this, pr_.stdType() );
    unfld_->setUnit( pr_.disp_.unit_ );
    unfld_->attach( rightOf, rgfld_ );

    const UnitOfMeasure* un = unfld_->getUnit();
    Interval<float> vintv( pr_.disp_.range_ );
    if ( un )
    {
	if ( !mIsUdf(vintv.start) )
	    vintv.start = un->userValue( vintv.start );
	if ( !mIsUdf(vintv.stop) )
	    vintv.stop = un->userValue( vintv.stop );
    }
    rgfld_->setValue( vintv );
}


#define mErrRet(s) { uiMSG().error(s); return false; }

bool uiEditPropRef::acceptOK( CallBacker* )
{
    const BufferString newnm( namefld_->text() );
    if ( newnm.isEmpty() || !isalpha(newnm[0]) || newnm.size() < 2 )
	mErrRet("Please enter a valid name");

    pr_.setName( newnm );
    SeparString ss( aliasfld_->text() ); const int nral = ss.size();
    pr_.aliases().erase();
    for ( int idx=0; idx<nral; idx++ )
	pr_.aliases().add( ss[idx] );
    pr_.disp_.color_ = colfld_->color();
    const UnitOfMeasure* un = unfld_->getUnit();
    Interval<float> vintv( rgfld_->getFInterval() );
    if ( !un )
	pr_.disp_.unit_.setEmpty();
    else
    {
	pr_.disp_.unit_ = un->name();
	if ( !mIsUdf(vintv.start) )
	    vintv.start = un->internalValue( vintv.start );
	if ( !mIsUdf(vintv.stop) )
	    vintv.stop = un->internalValue( vintv.stop );
    }

    return true;
}


void uiBuildPROPS::editReq( bool isadd )
{
    const char* nm = isadd ? curAvSel() : curDefSel();
    if ( !nm || !*nm ) return;

    PropertyRef* pr = 0;
    if ( !isadd )
	pr = props_.find( nm );
    else
    {
	PropertyRef::StdType typ = PropertyRef::Other;
	PropertyRef::parseEnumStdType( nm, typ );
	pr = new PropertyRef( "", typ );
    }
    if ( !pr ) return;

    uiEditPropRef dlg( this, *pr, isadd );
    if ( dlg.go() )
    {
	if ( isadd )
	    props_ += pr;
	handleSuccessfullEdit( isadd, pr->name() );
    }
}


void uiBuildPROPS::removeReq()
{
    const char* prnm = curDefSel();
    if ( prnm && *prnm )
    {
	const int idx = props_.indexOf( prnm );
	if ( idx < 0 ) return;
	delete props_.remove( idx );
	removeItem();
    }
}


void uiBuildPROPS::itemSwitch( const char* nm1, const char* nm2 )
{
    const int idx1 = props_.indexOf( nm1 );
    const int idx2 = props_.indexOf( nm2 );
    if ( idx1 < 0 || idx2 < 0 ) { pErrMsg("Huh"); return; }
    props_.swap( idx1, idx2 );
}


uiManPROPS::uiManPROPS( uiParent* p )
    : uiDialog(p,uiDialog::Setup("Layer Properties",
				"Select layer properties to model",mTODOHelpID))
{
    buildfld_ = new uiBuildPROPS( this, PROPS() );
    static const char* strs[] = { "For this survey only",
				  "As default for all surveys",
				  "As default for my user ID only",
				  "No (just use now)", 0 };
    srcfld_ = new uiGenInput( this, "Store", StringListInpSpec(strs) );
    srcfld_->attach( alignedBelow, buildfld_ );
}


bool uiManPROPS::acceptOK( CallBacker* )
{
    ePROPS() = buildfld_->props_;
    const int isrc = srcfld_->getIntValue();
    const Repos::Source repsrc =   isrc == 0	? Repos::Survey
				: (isrc == 1	? Repos::Data
				: (isrc == 2	? Repos::User
				    		: Repos::Temp));
    if ( isrc != 3 &&!PROPS().save(repsrc) )
	uiMSG().warning( "Could not store the definitions to file."
			 "\nPlease check file/directory permissions." );
    return true;
}
