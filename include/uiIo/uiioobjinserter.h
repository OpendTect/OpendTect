#ifndef uiioobjinserter_h
#define uiioobjinserter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		July 2014
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiiomod.h"
#include "factory.h"
#include "transl.h"

class uiToolButtonSetup;


/*!\brief inserts a new (external) object into the OD data store */

mExpClass(uiIo) uiIOObjInserter : public CallBacker
{
public:

			uiIOObjInserter( const Translator& trl )
			    : objectInserted(this)
			    , transl_(trl)		{}
    virtual		~uiIOObjInserter()		{}

    mDefineFactoryInClasswKW( uiIOObjInserter, factory,
				transl_.getDisplayName() )

    static bool		isPresent(const TranslatorGroup&);
    static bool		isPresent( const Translator& t )
			{ return factory().hasName(t.getDisplayName()); }

    static uiIOObjInserter* create( const Translator& t )
			{ return factory().create(t.getDisplayName()); }

    virtual uiToolButtonSetup*	getButtonSetup() const		= 0;
    bool		hasTranslator( const Translator& trl ) const
			{ return &transl_ == &trl; }

    Notifier<uiIOObjInserter>	objectInserted;
				//!< pass a CBCapsule<MultiID> in trigger(),

protected:

    const Translator&	transl_;

};


#endif
