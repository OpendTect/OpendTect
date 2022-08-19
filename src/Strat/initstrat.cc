/*+
________________________________________________________________________

 Copyright:	(C) 1995-2022 dGB Beheer B.V.
 License:	https://dgbes.com/licensing
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
