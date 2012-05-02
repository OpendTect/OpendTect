/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id: initstrat.cc,v 1.6 2012-05-02 15:11:52 cvskris Exp $";

#include "moddepmgr.h"
#include "stratsinglaygen.h"
#include "strattransl.h"

mDefModInitFn(Strat)
{
    mIfNotFirstTime( return );
    
    StratLayerSequenceAttribSetTranslatorGroup::initClass();
    StratLayerSequenceGenDescTranslatorGroup::initClass();
    
    odStratLayerSequenceGenDescTranslator::initClass();
    odStratLayerSequenceAttribSetTranslator::initClass();
    
    Strat::SingleLayerGenerator::initClass();
}
