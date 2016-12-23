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

mDefModInitFn(Strat)
{
    mIfNotFirstTime( return );

    odStratTreeTranslator::initClass();
    StratTreeTranslatorGroup::initClass();

    StratLayerSequenceAttribSetTranslatorGroup::initClass();
    StratLayerSequenceGenDescTranslatorGroup::initClass();

    odStratLayerSequenceGenDescTranslator::initClass();
    odStratLayerSequenceAttribSetTranslator::initClass();

    Strat::SingleLayerGenerator::initClass();
}
