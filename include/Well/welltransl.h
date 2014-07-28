#ifndef welltransl_h
#define welltransl_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id$
________________________________________________________________________


-*/

#include "wellmod.h"
#include "transl.h"

class Executor;
class DataPointSet;
class BufferStringSet;
class WellDataIOProvider;

namespace Well { class Data; };

/*!\brief Well TranslatorGroup */


mExpClass(Well) WellTranslatorGroup : public TranslatorGroup
{			    isTranslatorGroup(Well)
public:
			mDefEmptyTranslatorGroupConstructor(Well)
};


/*!  \brief Well Translator base class */

mExpClass(Well) WellTranslator : public Translator
{
public:
			mDefEmptyTranslatorBaseConstructor(Well)

    virtual const WellDataIOProvider&	getProv() const			= 0;
		//!< NEW in 5.0, will be the only one used in higher versions

/* DEPRECATED gone after 5.0 */
    virtual bool		read(Well::Data&,const IOObj&)		= 0;
    virtual bool		write(const Well::Data&,const IOObj&)	= 0;
/* END DEPRECATED */

};


/*!\brief WellTranslator for 'dGB' stored wells, OD's default well format. */

mExpClass(Well) odWellTranslator : public WellTranslator
{				isTranslator(od,Well)
public:
				mDefEmptyTranslatorConstructor(od,Well)

    const WellDataIOProvider&	getProv() const;

    virtual bool		implRemove(const IOObj*) const;
    virtual bool		implRename(const IOObj*,const char*,
					   const CallBack* cb=0) const;
    virtual bool		implSetReadOnly(const IOObj*,bool) const;

/* DEPRECATED gone after 5.0 */
    virtual bool	read(Well::Data&,const IOObj&);
    virtual bool	write(const Well::Data&,const IOObj&);
/* END DEPRECATED */

};


/* DEPRECATED gone after 5.0 */
#define dgbWellTranslator odWellTranslator
/* END DEPRECATED */

#endif

