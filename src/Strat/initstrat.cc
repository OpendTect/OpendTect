/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2010
________________________________________________________________________

-*/

#include "moddepmgr.h"
#include "stratsinglaygen.h"
#include "strattransl.h"
#include "strattreetransl.h"
#include "stratreftree.h"

mDefModInitFn(Strat)
{
    mIfNotFirstTime( return );

    StratTreeTranslatorGroup::initClass();
    StratLayerModelsTranslatorGroup::initClass();
    StratLayerSequenceAttribSetTranslatorGroup::initClass();
    StratLayerSequenceGenDescTranslatorGroup::initClass();

    odStratTreeTranslator::initClass();
    odStratLayerModelsTranslator::initClass();
    odStratLayerSequenceAttribSetTranslator::initClass();
    odStratLayerSequenceGenDescTranslator::initClass();

    Strat::SingleLayerGenerator::initClass();

    Strat::init();
}
