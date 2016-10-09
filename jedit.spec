###
### RPM spec file for jEdit
###

### This is a hack. For it to work, you must first install jEdit using
<<<<<<< HEAD
### the regular installer, then create a 'dummy' jedit31source.tar.gz
=======
### the regular installer, then create a 'dummy' jedit302source.tar.gz
>>>>>>> d5f8ea9e5f7b9c259ad11480490aa038259d1ee5
### file in the /usr/src/redhat/SOURCES directory.

### To create the RPM, invoke:
### rpm -ba jedit.spec --target=noarch

Summary: Programmer's text editor written in Java
Name: jedit
<<<<<<< HEAD
Version: 3.1
Release: 1
# REMIND: bump this with each RPM
Serial: 11
Copyright: GPL
Group: Application/Editors
Source0: http://download.sourceforge.net/jedit/jedit31source.tar.gz
=======
Version: 3.0.2
Release: 1
# REMIND: bump this with each RPM
Serial: 2
Copyright: GPL
Group: Application/Editors
Source0: http://download.sourceforge.net/jedit/jedit302source.tar.gz
>>>>>>> d5f8ea9e5f7b9c259ad11480490aa038259d1ee5
NoSource: 0
URL: http://www.jedit.org
Packager: Slava Pestov <slava@jedit.org>

%description
jEdit is an Open Source, cross platform text editor written in Java. It
has many advanced features that make text editing easier, such as syntax
highlighting, auto indent, abbreviation expansion, registers, macros,
regular expressions, and multiple file search and replace.

jEdit requires Java 2 (or Java 1.1 with Swing 1.1) in order to work.

%prep
<<<<<<< HEAD
rm -f /usr/doc/jedit-3.1
ln -sf ../share/jedit/3.1/doc /usr/doc/jedit-3.1
=======
rm -f /usr/doc/jedit-3.0.2
ln -sf ../share/jedit/3.0.2/doc /usr/doc/jedit-3.0.2
>>>>>>> d5f8ea9e5f7b9c259ad11480490aa038259d1ee5

%build

%install

<<<<<<< HEAD
%files
/usr/bin/jedit
/usr/doc/jedit-3.1
%docdir /usr/doc/jedit-3.1/
/usr/share/jedit/3.1/
=======
%post
# Create shell script here
mkdir -p ${RPM_INSTALL_PREFIX}/bin
echo "#!/bin/sh" > ${RPM_INSTALL_PREFIX}/bin/jedit
echo "# Java heap size, in megabytes (see doc/README.txt)" \
	>> ${RPM_INSTALL_PREFIX}/bin/jedit
echo "JAVA_HEAP_SIZE=16" >> ${RPM_INSTALL_PREFIX}/bin/jedit
echo 'exec java -mx${JAVA_HEAP_SIZE}m ${JEDIT} -classpath \
	"${CLASSPATH}:'${RPM_INSTALL_PREFIX}'/share/jedit/3.0.2/jedit.jar" \
	org.gjt.sp.jedit.jEdit $@' >> ${RPM_INSTALL_PREFIX}/bin/jedit
chmod 755 ${RPM_INSTALL_PREFIX}/bin/jedit

%preun
rm ${RPM_INSTALL_PREFIX}/bin/jedit

%files
/usr/doc/jedit-3.0.2
%docdir /usr/doc/jedit-3.0.2/
/usr/share/jedit/3.0.2/
>>>>>>> d5f8ea9e5f7b9c259ad11480490aa038259d1ee5
