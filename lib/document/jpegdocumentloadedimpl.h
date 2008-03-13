// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
#ifndef JPEGDOCUMENTLOADEDIMPL_H
#define JPEGDOCUMENTLOADEDIMPL_H

// Qt

// KDE

// Local
#include <lib/document/documentloadedimpl.h>

class QByteArray;

namespace Gwenview {

class JpegContent;

class JpegDocumentLoadedImplPrivate;
class JpegDocumentLoadedImpl : public DocumentLoadedImpl {
public:
	JpegDocumentLoadedImpl(Document*, JpegContent*);
	~JpegDocumentLoadedImpl();
	virtual void setImage(const QImage&);
	virtual void applyTransformation(Orientation orientation);

protected:
	virtual bool saveInternal(QIODevice* device, const QByteArray& format);

private:
	JpegDocumentLoadedImplPrivate* const d;
};


} // namespace

#endif /* JPEGDOCUMENTLOADEDIMPL_H */
