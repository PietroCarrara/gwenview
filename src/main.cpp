/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2003 Aur�ien G�eau

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kimageio.h>
#include <klocale.h>

#include "gvmainwindow.h"
#include "qxcfi.h"

static KCmdLineOptions options[] = {
	{ "f", I18N_NOOP("Start in fullscreen mode"), 0 },
	{ "+[file or folder]", I18N_NOOP("A starting file or folder"), 0 },
	KCmdLineLastOption
};

static const char* version="CVS>=20030728";


int main (int argc, char *argv[]) {
    KAboutData aboutData("gwenview", I18N_NOOP("Gwenview" ),
                        version, I18N_NOOP("An image viewer for KDE"), KAboutData::License_GPL,
                        "Copyright 2000-2003 Aurélien Gâteau",0,"http://gwenview.sourceforge.net");

	KCmdLineArgs::init( argc, argv, &aboutData );
	KCmdLineArgs::addCmdLineOptions( options );

	KApplication kapplication;

	KImageIO::registerFormats();
	XCFImageFormat::registerFormat();

	if (kapplication.isRestored()) {
		RESTORE(GVMainWindow)
	} else {
		GVMainWindow *mainWindow = new GVMainWindow;
		mainWindow->show();
	}

	return kapplication.exec();
}
