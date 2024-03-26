TEMPLATE = aux

PKGDIR = $${PWD}/../../../app/qtapp/packaging/

# installer
INSTALLER = RtkLibInstaller

INPUT = $$PWD/config/config.xml $$PWD/packages
installer.input = INPUT
installer.output = $$INSTALLER
installer.commands = $(QTDIR)/../../TOOLS/QtInstallerFramework/4.7/bin/binarycreator.exe -c $$PWD/config/config.xml -p $$PWD/packages ${QMAKE_FILE_OUT}
installer.CONFIG += target_predeps no_link combine
installer.depends = deploy license_copy

QMAKE_EXTRA_COMPILERS += installer

# deployment (we should actually call the deployment programm for all applications)
deploy.commands = $(QTDIR)/bin/windeployqt $$shell_path($$PKGDIR/packages/com.rtklib.rtkplot_qt/data/rtkplot_qt.exe) --dir $$shell_path($$PKGDIR/packages/com.rtklib/data/)

license_copy.commands = $$QMAKE_COPY $$shell_path($${PWD}/../../../license.txt) $$shell_path($$PKGDIR/packages/com.rtklib/meta/)

QMAKE_EXTRA_TARGETS += deploy license_copy
