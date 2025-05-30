/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
________________________________________________________________________

-*/

#include "uiwellwriteopts.h"

#include "filepath.h"
#include "iostrm.h"
#include "settings.h"
#include "welltransl.h"
#include "wellodwriter.h"

#include "uigeninput.h"
#include "uistrings.h"

#define mODWellTranslInstance mTranslTemplInstance(Well,od)

static int cDefBinIdx = 0;
static int cDefAsciiIdx = 1;


static const int& cDefIdx()
{
    static int defidx = -1;
    if ( defidx < 0 )
    {
	bool defbinwrite = true;
	mSettUse(getYN,"dTect.Well logs","Binary format",defbinwrite);
	defidx = defbinwrite ? cDefBinIdx : cDefAsciiIdx;
    }

    return defidx;
}


uiODWellWriteOpts::uiODWellWriteOpts( uiParent* p )
    : uiIOObjTranslatorWriteOpts(p,mODWellTranslInstance)
{
    wrlogbinfld_ = new uiGenInput( this, tr("%1 storage")
		       .arg(uiStrings::sWellLog()),
			BoolInpSpec(cDefIdx() == cDefBinIdx,
				uiStrings::sBinary(),uiStrings::sASCII()) );

    setHAlignObj( wrlogbinfld_ );
}


uiODWellWriteOpts::~uiODWellWriteOpts()
{}


void uiODWellWriteOpts::usePar( const IOPar& iop )
{
    BufferString res;
    if ( iop.get(Well::odWriter::sKeyLogStorage(),res) )
    {
	const bool binwr = !res.isEmpty() && res.firstChar() != 'A';
	wrlogbinfld_->setValue( binwr );
    }
    else
	wrlogbinfld_->setValue( true );
}


bool uiODWellWriteOpts::fillPar( IOPar& iop ) const
{
    const bool wantbin = wrlogbinfld_->getBoolValue();
    iop.set( Well::odWriter::sKeyLogStorage(), wantbin ? "Binary" : "Ascii" );

    return true;
}


uiIOObjTranslatorWriteOpts* uiODWellWriteOpts::create( uiParent* p )
{
    return new uiODWellWriteOpts( p );
}


void uiODWellWriteOpts::initClass()
{
    factory().addCreator( create, mODWellTranslInstance.getDisplayName() );
}
