/*
Copyright 2004 Jonathan Riddell <jr@jriddell.org>

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
#ifndef __gvdirpart_h__
#define __gvdirpart_h__

#include <kparts/part.h>
#include <kparts/browserextension.h>

// Forward declarations
class QSplitter;
class KAboutData;
class KAction;
class GVScrollPixmapView;
class GVFileViewStack;
class GVPixmap;

class GVDirPart;

/**
 * The browser extension is an attribute of GVImagePart and provides
 * some services to Konqueror.  All Konqueror KParts have one.
 */
class GVDirPartBrowserExtension: public KParts::BrowserExtension {
	Q_OBJECT

 public:
	GVDirPartBrowserExtension(GVDirPart* viewPart, const char* name=0L);
	~GVDirPartBrowserExtension();

//protected slots:
 public slots:
//  void selected(TreeMapItem*);
//  void contextMenu(TreeMapItem*,const QPoint&);
	void contextMenu();

	void updateActions();
	void refresh();

	void copy();
	void cut();
	void trash();
	void del();
	void editMimeType();

	void directoryChanged(const KURL& dirURL);
 private:
	GVDirPart* m_gvDirPart;
};

/**
 * A Read Only KPart to browse directories and their images using Gwenview
 */
class GVDirPart : public KParts::ReadOnlyPart {
	Q_OBJECT
 public:
	GVDirPart(QWidget*, const char*, QObject*, const char*, const QStringList &);
	virtual ~GVDirPart();

	/**
	 * Return information about the part
	 */
	static KAboutData* createAboutData();

	/**
	 * Sets Konqueror's caption with setWindowCaption()
	 */
	void setKonquerorWindowCaption(const QString& url);

	/**
	 * Returns the name of the current file in the pixmap
	 */
	KURL pixmapURL();
 protected:

	/**
	 * Unused because openURL() is implemented but required to be
	 * implemented.
	 */
	virtual bool openFile();
	
	/**
	 * Tell the widgets the URL to browse.  Sets the window
	 * caption and saves URL to m_url (important for history and
	 * others).
	 */
	virtual bool openURL(const KURL& url);

 protected slots:
	void slotExample();

 protected:
        /**
	 * The component's widget, contains the files view on the left
	 * and scroll view on the right.
	 */
	QSplitter* m_splitter;

        /**
	 * Scroll widget
	 */
	GVScrollPixmapView* m_pixmapView;

	/**
	 * Holds the image
	 */
	GVPixmap* m_gvPixmap;

	/**
	 * Shows the directory's files and folders
	 */

	GVFileViewStack* m_filesView;

	// An example action to which we need to keep a pointer
	KAction* m_exampleAction;

	/**
	 * This inherits from KParts::BrowserExtention and supplies
	 * some extra functionality to Konqueror.
	 */
	GVDirPartBrowserExtension* m_browserExtension;
};

#endif
