/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          December 2004
 RCS:           $Id: uiscalingattrib.cc,v 1.1 2005-05-31 12:33:55 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uiscalingattrib.h"
#include "scalingattrib.h"
#include "attribdesc.h"
#include "uigeninput.h"
#include "uitable.h"
#include "uiattrsel.h"
#include "survinfo.h"

using namespace Attrib;

static const int initnrrows = 5;
static const int startcol = 0;
static const int stopcol = 1;
static const int factcol = 2;


uiScalingAttrib::uiScalingAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld = getInpFld();

    typefld = new uiGenInput( this, "Type", 
	    		StringListInpSpec(ScalingAttrib::ScalingTypeNames) );
    typefld->valuechanged.notify( mCB(this,uiScalingAttrib,typeSel) );
    typefld->attach( alignedBelow, inpfld );

    nfld = new uiGenInput( this, "n", FloatInpSpec() );
    nfld->attach( alignedBelow, typefld );

    statsfld = new uiGenInput( this, "Basis",
			StringListInpSpec(ScalingAttrib::StatsTypeNames) );
    statsfld->attach( alignedBelow, typefld );
    statsfld->valuechanged.notify( mCB(this,uiScalingAttrib,statsSel) );

    table = new uiTable( this, uiTable::Setup().rowdesc("Gate")
					       .rowcangrow()
					       .defrowlbl()
					       .fillcol()
					       .maxrowhgt(1) );

    BufferString lblstart = "Start "; lblstart += SI().getZUnit();
    BufferString lblstop = "Stop "; lblstop += SI().getZUnit();
    const char* collbls[] = { lblstart.buf(), lblstop.buf(), "Scale value", 0 };
    table->setColumnLabels( collbls );
    table->setNrRows( initnrrows );
    table->setColumnStretchable( startcol, true );
    table->setColumnStretchable( stopcol, true );
    table->attach( alignedBelow, statsfld );
    table->setStretch( 2, 0 );
    table->setToolTip( "Right-click to add, insert or remove a gate" );

    typeSel(0);
    statsSel(0);

    setHAlignObj( inpfld );
}


void uiScalingAttrib::typeSel( CallBacker* )
{
    const int typeval = typefld->getIntValue();
    nfld->display( !typeval );

    statsfld->display( typeval==1 );
    table->display( typeval==1 );
}


void uiScalingAttrib::statsSel( CallBacker* )
{
    const int statstype = statsfld->getIntValue();
    table->hideColumn( 2, statstype!=3 );
}


bool uiScalingAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Scaling::attribName()) )
	return false;

    mIfGetEnum( Scaling::typeStr(), scalingtype,
	        typefld->setValue(scalingtype) );
    mIfGetFloat( Scaling::powerStr(), powerval, nfld->setValue(powerval) );
    mIfGetEnum( Scaling::statsStr(), statstype, statsfld->setValue(statstype) );

    table->clearTable();
    /*
    const int nrtgs = pars.timegates.size();
    while ( nrtgs > table->nrRows() )
	table->insertRows(0);
    
    while ( nrtgs < table->nrRows() && table->nrRows() > initnrrows )
	table->removeRow(0);

    mIfHaveValFor(timegates)
    {
	for ( int idx=0; idx<pars.timegates.size(); idx++ )
	{
	    table->setValue( uiTable::RowCol(idx,startcol), 
		    	     pars.timegates[idx].start );
	    table->setValue( uiTable::RowCol(idx,stopcol), 
		    	     pars.timegates[idx].stop );
	}
    }

    mIfHaveValFor(scalefactors)
    {
	for ( int idx=0; idx< pars.scalefactors.size(); idx++ )
	    table->setValue( uiTable::RowCol(idx,factcol),
		    	     pars.scalefactors[idx] );
    }
    */

    typeSel(0);
    statsSel(0);
    return true;
}


bool uiScalingAttrib::setInput( const Desc& desc )
{
    putInp( inpfld, desc, 0 );
    return true;
}


bool uiScalingAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Scaling::attribName()) )
	return false;

    mGetEnum( Scaling::typeStr(), typefld->getIntValue() );
    mGetFloat( Scaling::powerStr(), nfld->getfValue() );
    mGetEnum( Scaling::statsStr(), statsfld->getIntValue() );

    /*
    TypeSet<TimeGate> tgs;
    TypeSet<float> factors;
    for ( int idx=0; idx<table->nrRows(); idx++ )
    {
	int start = table->getIntValue( uiTable::RowCol(idx,startcol) );
	int stop = table->getIntValue( uiTable::RowCol(idx,stopcol) );
	if ( mIsUndefInt(start) && mIsUndefInt(stop) ) continue;
	
	tgs += TimeGate(start,stop );

	if ( statsfld->getIntValue() == 3 )
	{
	    const char* fact = table->text( uiTable::RowCol(idx,factcol) );
	    factors += fact && *fact ? atof(fact) : 1;
	}
    }

    pars.timegates.setSize( tgs.size() );
    for ( int idx=0; idx<tgs.size(); idx++ )
	chtr.update( pars.timegates[idx], tgs[idx] );

    pars.scalefactors.setSize( factors.size() );
    for ( int idx=0; idx<factors.size(); idx++ )
	chtr.update( pars.scalefactors[idx], factors[idx] );
    */

    return true;
}


bool uiScalingAttrib::getInput( Desc& desc )
{
    fillInp( inpfld, desc, 0 );
    return true;
}
