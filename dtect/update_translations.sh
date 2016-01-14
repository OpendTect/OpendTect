#!/bin/sh
#_______________________________________________________________________________
#
# (C) dGB Beheer B.V.
# $Id$
#
#_______________________________________________________________________________

# The purpose of this script is to make a .pro file to tell lupdate which
# files to scan and for which languages a ts file has to be created.
# It will:
# - make  a copy of an od directory
# - replace mExpClass and mClass by class
# - replace *_static_tr( "myfunc", ... ) with static_func_myfunc( ... )
# - create lupdate.pro
# - run lupdate for plural (*en-us.ts) and non-plural translations (all others).
# - copy the new translations into the work-directory.
# - remove the temporary copy

if [ $# -lt 5 ]; then
    echo "Usage : $0 <sourcedir> <tsbasedir> <binarydir> <application> <lupdate>"
    exit 1
fi


if [ $?DTECT_SCRIPT_VERBOSE ]; then
    echo "$0  ++++++"
    echo "args: $*"
    verbose=yes
    echo=on
fi

scriptdir=`dirname $0`
nrcpu=`${scriptdir}/GetNrProc`

sourcedir=$1
tsbasedir=$2
binarydir=$3
application=$4
lupdate=$5
tmpoddir=/tmp/lupdate_tmp_$$

removetmpoddirsed="s/\/tmp\/"
removetmpoddirsed+=`basename $tmpoddir`
removetmpoddirsed+="\///g"

kernel=`uname -a | awk '{print $1}'`
if [ "${kernel}" == "Darwin" ]; then
    lupdate_dir=`dirname ${lupdate}`
    setenv DYLD_LIBRARY_PATH ${lupdate_dir}/../lib
fi

if [ -e $tmpoddir ]; then
    \rm -rf $tmpoddir
fi

mkdir $tmpoddir

#Copy all src, include and pluginfiles (excluding CMake-stuff) to temp folder
olddir=`pwd`
cd ${sourcedir}
if [ -e ${sourcedir}/src ]; then
    find src -depth -type f | grep -v CMake | \
	cpio --pass-through --preserve-modification-time \
	--make-directories --quiet ${tmpoddir}
fi
if [ -e ${sourcedir}/include ]; then
    find include -depth -type f | grep -v CMake | \
	cpio --pass-through --preserve-modification-time \
	--make-directories --quiet ${tmpoddir}
fi
if [ -e ${sourcedir}/plugins ]; then
    find plugins -depth -type f | grep -v CMake | \
	cpio --pass-through --preserve-modification-time \
	--make-directories --quiet ${tmpoddir}
fi

projectdir="${tmpoddir}/data/localizations/source"
mkdir -p ${projectdir}

cd ${olddir}

shopt -s nullglob
#Copy existing ts-files ot project dir
cp -a ${tsbasedir}/data/localizations/source/${application}*.ts ${projectdir}
if ( -d ${tsbasedir}/data/localizations/inprogress ) then
    cp -a ${tsbasedir}/data/localizations/inprogress/${application}*.ts ${projectdir}
endif

profnm=${projectdir}/normaltrans.pro

cd ${projectdir}

headers=`find $tmpoddir -path "*.h"`
dirfile=${tmpoddir}/dirs
for header in ${headers}; do
    dirname ${header} >> ${dirfile}
done

dirs=`sort -u ${dirfile}`

#Create file with all files
filelist="filelist.txt"

echo "" >> ${filelist}

echo -n "HEADERS = " >> ${filelist}
for fnm in ${headers} ; do
    echo " \\" >> ${filelist}
    echo -n "    $fnm" >> ${filelist}
done

echo "" >> ${filelist}

sources=`find ${tmpoddir} -path "*.cc"`

echo -n "SOURCES = " >> ${filelist}
for fnm in $sources ; do
    echo " \\" >> ${filelist}
    echo -n "    $fnm" >> ${filelist}
done

# Create a list of target .ts files for normal ts files
echo -n "TRANSLATIONS = " > ${profnm}

echo " \\" >> ${profnm}
echo -n "    ${application}_template.ts" >> ${profnm}

nonomatch=1
for fnm in ${application}*.ts ; do
    if [[ "${fnm}" =~ .*en-us.ts ]]; then
	continue
    fi

    if [[ "${fnm}" =~ .*_template.ts ]]; then
	continue
    fi

    echo " \\" >> ${profnm}
    echo -n "    $fnm" >> ${profnm}
done

cat ${filelist} >> ${profnm}

echo "" >> ${profnm}
echo -n "INCLUDEPATH += " >> ${profnm}

for dir in ${dirs} ; do
    echo " \\" >> ${profnm}
    echo -n "	${dir}" >>${profnm}
done

#Create a list of .ts files for plural operations
pluralpro=$projectdir/plural.pro
if [ -e ${application}_en-us.ts ]; then
    echo -n "TRANSLATIONS = " > ${pluralpro}
    echo " \\" >> ${pluralpro}
    echo -n "    ${application}_en-us.ts" >> ${pluralpro}
    cat ${filelist} >> ${pluralpro}

    echo "" >> ${pluralpro}
    echo -n "INCLUDEPATH += " >> ${pluralpro}

    for dir in ${dirs} ; do
	    echo " \\" >> ${pluralpro}
	    echo -n "	${dir}" >>${pluralpro}
    done
fi

#Remove the filelist
\rm -rf ${filelist}

#Filter the sources for patterns
echo ${sources} | xargs -P ${nrcpu} sed \
	-e 's/mODTextTranslationClass(.*)/Q_OBJECT/g' \
	-e 's/mdGBTextTranslationClass(.*)/Q_OBJECT/g' \
	-e 's/mTextTranslationClass(.*)/Q_OBJECT/g' \
	-e 's/mExpClass(.*)/class /g' \
	-e 's/mClass(.*)/class /g' \
	-e 's/mStruct(.*)/struct /g' \
	-e 's/mExpStruct(.*)/struct /g' \
	-e 's/mEnumTr/EnumDefImpl::tr /g' \
	-e 's/"static_func_"/""/g' \
	-e 's/[^( \t]*_static_tr([ \t]*/static_func___begquote/g' \
	-e 's/static_func_[^,]*/&__endquote::tr(/g' \
	-e 's/"[\t ]*__endquote/"__endquote/g' \
	-e 's/::tr(,/::tr(/g' \
	-e 's/__begquote"//g' \
	-e 's/"__endquote//g' -iTMP

#Filter the headers for patterns
echo ${headers} | xargs -P ${nrcpu} sed \
	-e 's/mODTextTranslationClass(.*)/Q_OBJECT/g' \
	-e 's/mdGBTextTranslationClass(.*)/Q_OBJECT/g' \
	-e 's/mTextTranslationClass(.*)/Q_OBJECT/g' \
	-e 's/mExpClass(.*)/class /g' \
	-e 's/mClass(.*)/class /g' \
	-e 's/mStruct(.*)/struct /g' \
	-e 's/mExpStruct(.*)/struct /g' \
	-e 's/mEnumTr/EnumDefImpl::tr /g' \
	-e 's/"static_func_"/""/g' \
	-e 's/[^( \t]*_static_tr([ \t]*/static_func___begquote/g' \
        -e 's/static_func_[^,]*/&__endquote::tr(/g' \
	-e 's/"[\t ]*__endquote/"__endquote/g' \
        -e 's/::tr(,/::tr(/g' \
        -e 's/__begquote"//g' \
        -e 's/"__endquote//g' -iTMP

result=0
#Run lupdate
errfile=${tmpoddir}/stderr
${lupdate} -silent -locations relative ${profnm} 2> ${errfile}
errors=`cat ${tmpoddir}/stderr`
if [ "${errors}" != "" ]; then
    echo "Errors during non-plural processing:"
    cat ${tmpoddir}/stderr | sed -e $removetmpoddirsed
    result=1
fi

if [ -e ${pluralpro} ]; then
    ${lupdate} -silent -locations relative -pluralonly ${pluralpro} 2> ${errfile}
    errors=`cat ${tmpoddir}/stderr`
    if [ "${errors}" != "" ]; then
	echo "Errors during plural processing:"
	cat ${tmpoddir}/stderr | sed -e $removetmpoddirsed
	result=1
    fi
fi

#Copy results back
rsync --checksum *.ts ${binarydir}/data/localizations/generated

#Remvoe temporary dir
\rm -rf  ${tmpoddir}

#Go back to starting dir
cd ${olddir}


exit ${result}
