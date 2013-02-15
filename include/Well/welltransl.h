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
#include "position.h"

class Executor;
class DataPointSet;
class BufferStringSet;

namespace Well { class Data; };

/*!
\brief Well TranslatorGroup
*/

mExpClass(Well) WellTranslatorGroup : public TranslatorGroup
{			    isTranslatorGroup(Well)
public:
    			mDefEmptyTranslatorGroupConstructor(Well)
    const char*		defExtension() const { return "well"; }
};


/*!
\brief Well Translator
*/

mExpClass(Well) WellTranslator : public Translator
{
public:
    			mDefEmptyTranslatorBaseConstructor(Well)

    virtual bool	read(Well::Data&,const IOObj&)		= 0;
    virtual bool	write(const Well::Data&,const IOObj&)	= 0;

    virtual bool	implRemove(const IOObj*) const;
    virtual bool	implRename(const IOObj*,const char*,
	    			   const CallBack* cb=0) const;
    virtual bool	implSetReadOnly(const IOObj*,bool) const;

    			//!< Implemented using TrackSampler
    static Executor*	createDataPointSets(const BufferStringSet&,
					    const IOPar&,bool for2d,
					    ObjectSet<DataPointSet>&,
					    bool zvalsintime);
};


/*!
\brief dgb WellTranslator
*/

mExpClass(Well) dgbWellTranslator : public WellTranslator
{			  isTranslator(dgb,Well)
public:
    			mDefEmptyTranslatorConstructor(dgb,Well)

    virtual bool	read(Well::Data&,const IOObj&);
    virtual bool	write(const Well::Data&,const IOObj&);

};


#endif

