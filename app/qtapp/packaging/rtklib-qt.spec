# RPM spec file optimized for Open SuSE Build Service
%define version %(echo $RTKLIB_VERSION)
%define release %(echo $RTKLIB_RELEASE)

Name:           rtklib-qt
Version:        %{version}
Release:        %{release}
Summary:        GUI for GNSS processing
Group:          Productivity/Scientific/Other
License:        BSD-2-Clause
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-root
Url:            http://www.rtklib.com
Vendor:         T. Takasu
Requires:       libtool
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig

%if 0%{?is_opensuse} && ( 0%{?sle_version} <= 150500 && 0%{?sle_version} != 0 )
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5WebEngine)
BuildRequires:  pkgconfig(Qt5Widgets)
BuildRequires:  pkgconfig(Qt5Xml)
BuildRequires:  pkgconfig(Qt5Concurrent)
BuildRequires:  pkgconfig(Qt5SerialPort)
%define qt_version 5 
%endif

%if (0%{?centos_version} <= 700 && 0%{?centos_version} != 0 ) || (0%{?rhel_version} <= 700 && 0%{?rhel_version} != 0 )
BuildRequires:  qt5-qtbase
BuildRequires:  qt5-qtwebengine
BuildRequires:  qt5-qtbase-gui
BuildRequires:  qt5-qtserialport
BuildRequires:  qt5-qtsvg
BuildRequires:  qt5-linguist
%define qt_version 5
%endif 

%if 0%{?fedora_version} || 0%{?fedora} || ( 0%{?sle_version} > 150500 && 0%{?is_opensuse} ) || 0%{?centos_version} >= 800 || 0%{?suse_version} >= 1600
BuildRequires:  pkgconfig(Qt6WebEngineWidgets)
BuildRequires:  pkgconfig(Qt6Widgets)
BuildRequires:  pkgconfig(Qt6Xml)
BuildRequires:  pkgconfig(Qt6Concurrent)
BuildRequires:  pkgconfig(Qt6SerialPort)
%define qt_version 6
%endif

%if 0%{?fedora_version} || 0%{?fedora}
BuildRequires:  qt6-linguist
BuildRequires:  gcc-gfortran
BuildRequires:  desktop-file-utils
%endif

%if 0%{?fedora_version} == 40 || 0%{?fedora}
BuildRequires:  oneVPL
%endif

%if 0%{?suse_version}
BuildRequires:  gcc-fortran
BuildRequires:  update-desktop-files
%endif

%if %{undefined qt_version}
%define qt_version 6
%endif


%description
Library and Qt frontend for GNSS (Global Navigation Satellite Systems) raw data processing, visualisation and
analysis. It support both, real-time kinematik (RTK) and precise point positioning (PPP) processing.

%prep
%setup -q -n %{name}-%{version}

%build
cd bin

%if 0%{?sle_version} >= 150600 
%qmake6 -r ../app/qtapp CONFIG+=packaging QMAKE_CFLAGS+="%optflags" QMAKE_CXXFLAGS+="%optflags" QMAKE_STRIP="/bin/true"
%else
%if 0%{qt_version} == 6
qmake -r ../app/qtapp CONFIG+=packaging QMAKE_CFLAGS+="%optflags" QMAKE_CXXFLAGS+="%optflags" QMAKE_STRIP="/bin/true"
%else
%{qmake5} -r ../app/qtapp CONFIG+=packaging QMAKE_CFLAGS+="%optflags" QMAKE_CXXFLAGS+="%optflags" QMAKE_STRIP="/bin/true"
%endif
%endif
make %{?_smp_mflags}

%post
ldconfig

%install
cd bin
make install INSTALL_ROOT=%{buildroot}

%if 0%{?suse}
%suse_update_desktop_file %{name}
%endif
%if 0%{?fedora_version}
desktop-file-validate %{buildroot}/%{_datadir}/applications/rtkconv_qt.desktop
desktop-file-validate %{buildroot}/%{_datadir}/applications/rtkget_qt.desktop
desktop-file-validate %{buildroot}/%{_datadir}/applications/rtklaunch_qt.desktop
desktop-file-validate %{buildroot}/%{_datadir}/applications/rtkpost_qt.desktop
desktop-file-validate %{buildroot}/%{_datadir}/applications/rtkplot_qt.desktop
desktop-file-validate %{buildroot}/%{_datadir}/applications/rtknavi_qt.desktop
desktop-file-validate %{buildroot}/%{_datadir}/applications/srctblbrows_qt.desktop
desktop-file-validate %{buildroot}/%{_datadir}/applications/strsvr_qt.desktop
%endif


%postun
ldconfig

%files
%{_bindir}/*
/usr/lib/libRTKLib.*
/usr/share/applications/*
/usr/share/pixmaps/*

%changelog
   * Mon Feb 26 2024 Jens Reimann
   - Initial build
