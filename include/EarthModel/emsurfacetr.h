#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "emsurfaceiodata.h"
#include "transl.h"
#include "uistring.h"

class Executor;
class IOObj;

namespace EM
{
    class dgbSurfaceReader;
    class FaultStickSet;
    class Fault3D;
    class FaultSet3D;
    class Horizon;
    class Horizon2D;
    class Horizon3D;
    class Surface;
}

typedef EM::Horizon3D		EMHorizon3D;
typedef EM::Horizon2D		EMHorizon2D;
typedef EM::Horizon		EMAnyHorizon;
typedef EM::Fault3D		EMFault3D;
typedef EM::FaultSet3D		EMFaultSet3D;
typedef EM::FaultStickSet	EMFaultStickSet;


/*!
\brief TranslatorGroup for EM::Horizon3D. Reads/writes 3D EM::Horizon3D to
storage.
*/

mExpClass(EarthModel) EMHorizon3DTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(EMHorizon3D);
    mODTextTranslationClass(EMHorizon3DTranslatorGroup);
public:
				mDefEmptyTranslatorGroupConstructor(EMHorizon3D)

    const char*			defExtension() const	{ return "hor"; }
};


/*!
\brief TranslatorGroup for EM::Horizon2D.
*/

mExpClass(EarthModel) EMHorizon2DTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(EMHorizon2D);
    mODTextTranslationClass(EMHorizon2DTranslatorGroup);
public:
				mDefEmptyTranslatorGroupConstructor(EMHorizon2D)

    const char*			defExtension() const	{ return "2dh"; }
};


/*!
\brief TranslatorGroup for EM::Horizon.
*/

mExpClass(EarthModel) EMAnyHorizonTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(EMAnyHorizon);
    mODTextTranslationClass(EMAnyHorizonTranslatorGroup);
public:
			    mDefEmptyTranslatorGroupConstructor(EMAnyHorizon)
};


/*!
\brief TranslatorGroup for EM::Fault3D.
*/

mExpClass(EarthModel) EMFault3DTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(EMFault3D);
    mODTextTranslationClass(EMFault3DTranslatorGroup);;
public:
			mDefEmptyTranslatorGroupConstructor(EMFault3D)

    const char*		defExtension() const { return "flt"; }
};


/*!
\brief TranslatorGroup for EM::FaultSet3D.
*/
mExpClass(EarthModel) EMFaultSet3DTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(EMFaultSet3D);
public:
			mDefEmptyTranslatorGroupConstructor(EMFaultSet3D)
};


/*!
\brief TranslatorGroup for EM::FaultStickSet.
*/

mExpClass(EarthModel) EMFaultStickSetTranslatorGroup : public TranslatorGroup
{   isTranslatorGroup(EMFaultStickSet);
    mODTextTranslationClass(EMFaultStickSetTranslatorGroup);;
public:
			mDefEmptyTranslatorGroupConstructor(EMFaultStickSet)

    const char*		defExtension() const { return "fss"; }
};


/*!
\brief Translator for EM::Surface.
*/

mExpClass(EarthModel) EMSurfaceTranslator : public Translator
{
public:
				EMSurfaceTranslator(const char* nm,
						    const char* unm)
				    : Translator(nm,unm)
				    , ioobj_(0)
				    , surface_(0)
				    , sels_(sd_)	{}

    virtual			~EMSurfaceTranslator();

    bool			startRead(const IOObj&);
    bool			startWrite(const EM::Surface&);

    EM::SurfaceIODataSelection& selections()		{ return sels_; }

    virtual Executor*		reader(EM::Surface&)	{ return 0; }
				/*!< Executor is managed by client. */
    Executor*			writer(const IOObj&,bool fullimplremove=true);
				/*!< Executor is managed by client. */
    virtual Executor*		getAuxdataReader(EM::Surface&,int)
				{ return 0; }
    virtual Executor*		getAuxdataWriter(const EM::Surface&,int,
							      bool overwt=false)
				{ return 0; }

    uiString			errMsg() const		{ return errmsg_; }

    virtual bool		implRemove(const IOObj*) const;
    virtual bool		implRename(const IOObj*,const char*,
					   const CallBack* cb=0) const;
    virtual bool		implSetReadOnly(const IOObj*,bool) const;

    static bool			getBinarySetting();

    void			setCreatedFrom( const char* src )
							{ crfrom_ = src; }

protected:

    IOObj*			ioobj_;
    EM::Surface*		surface_;
    uiString			errmsg_;
    EM::SurfaceIOData		sd_;
    EM::SurfaceIODataSelection	sels_;
    BufferString		crfrom_;

    virtual bool		prepRead()		{ return true; }
    virtual bool		prepWrite()		{ return true; }
    virtual Executor*		getWriter()		{ return 0; }

    void			init(const EM::Surface*,const IOObj*);
    void			setIOObj(const IOObj*);
};


/*!
\brief dgb EMSurfaceTranslator
*/

mExpClass(EarthModel) dgbEMSurfaceTranslator : public EMSurfaceTranslator
{
public:
				dgbEMSurfaceTranslator(const char*,const char*);
    virtual			~dgbEMSurfaceTranslator();

    virtual Executor*		reader(EM::Surface&);

protected:

    static BufferString		createHOVName(const char* base,int idx);
    bool			setSurfaceTransform(const IOPar&);
    void			getSels(StepInterval<int>&,StepInterval<int>&);

    bool			prepRead();
    Executor*			getWriter();

    virtual bool		readOnlyZ() const		{ return true; }
    virtual bool		writeOnlyZ() const		{ return true; }
    virtual bool		hasRangeSelection() const	{ return true; }

    EM::dgbSurfaceReader*	reader_;
};


/*!
\brief dgbEMSurfaceTranslator for EM::Horizon3D.
*/

mExpClass(EarthModel) dgbEMHorizon3DTranslator : public dgbEMSurfaceTranslator
{	isTranslator(dgb,EMHorizon3D)
	mODTextTranslationClass(dgbEMHorizon3DTranslator)
public:
				dgbEMHorizon3DTranslator(const char* unm,
						       const char* nm)
				    : dgbEMSurfaceTranslator(unm,nm)	{}
    virtual			~dgbEMHorizon3DTranslator()		{}
    virtual Executor*		getAuxdataReader(EM::Surface&,int);
    virtual Executor*		getAuxdataWriter(const EM::Surface&,int,
						    bool ovwrt=false);


protected:
    virtual bool		readOnlyZ() const		{ return true; }
    virtual bool		writeOnlyZ() const		{ return true; }
    virtual bool		hasRangeSelection() const	{ return true; }
};


/*!
\brief dgbEMSurfaceTranslator for EM::Horizon2D.
*/

mExpClass(EarthModel) dgbEMHorizon2DTranslator : public dgbEMSurfaceTranslator
{				 isTranslator(dgb,EMHorizon2D)
public:
				dgbEMHorizon2DTranslator(const char* unm,
						       const char* nm)
				    : dgbEMSurfaceTranslator(unm,nm)	{}
    virtual			~dgbEMHorizon2DTranslator()		{}

protected:
    virtual bool		readOnlyZ() const		{return false;}
    virtual bool		writeOnlyZ() const		{return true;}
    virtual bool		hasRangeSelection() const	{return false;}
};


/*!
\brief dgbEMSurfaceTranslator for EM::Fault3D.
*/

mExpClass(EarthModel) dgbEMFault3DTranslator : public dgbEMSurfaceTranslator
{			       isTranslator(dgb,EMFault3D)
public:
				dgbEMFault3DTranslator(const char* unm,
						       const char* nm)
				    : dgbEMSurfaceTranslator(unm,nm)	{}
    virtual			~dgbEMFault3DTranslator()		{}

protected:
    virtual bool		readOnlyZ() const		{ return false;}
    virtual bool		writeOnlyZ() const		{ return false;}
    virtual bool		hasRangeSelection() const	{ return false;}
};


/*!
\brief dgbEMSurfaceTranslator for EM::FaultStickSet.
*/

mExpClass(EarthModel) dgbEMFaultStickSetTranslator :
				public dgbEMSurfaceTranslator
{				     isTranslator(dgb,EMFaultStickSet)
public:
				dgbEMFaultStickSetTranslator(const char* unm,
							     const char* nm)
				    : dgbEMSurfaceTranslator(unm,nm)	{}
    virtual			~dgbEMFaultStickSetTranslator()		{}

protected:
    virtual bool		readOnlyZ() const		{ return false;}
    virtual bool		writeOnlyZ() const		{ return false;}
    virtual bool		hasRangeSelection() const	{ return false;}
};


/*!
\brief Translator for EM::FaultSet3D.
*/
mExpClass(EarthModel) EMFaultSet3DTranslator : public Translator
{
public:
				EMFaultSet3DTranslator(const char* unm,
						       const char* nm)
				    : Translator(unm,nm)	{}
    virtual			~EMFaultSet3DTranslator()		{}
    virtual Executor*		writer(const EM::FaultSet3D&,const IOObj&) = 0;
    virtual Executor*		reader(EM::FaultSet3D&,const IOObj&)	= 0;
};


mExpClass(EarthModel) dgbEMFaultSet3DTranslator : public EMFaultSet3DTranslator
{			       isTranslator(dgb,EMFaultSet3D)
public:
				dgbEMFaultSet3DTranslator(const char* unm,
						       const char* nm)
				    : EMFaultSet3DTranslator(unm,nm)	{}
    virtual			~dgbEMFaultSet3DTranslator()		{}
    virtual Executor*		writer(const EM::FaultSet3D&,const IOObj&);
    virtual Executor*		reader(EM::FaultSet3D&,const IOObj&);
};

