/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

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
