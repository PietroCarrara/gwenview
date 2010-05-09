// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef DOCUMENTJOB_H
#define DOCUMENTJOB_H

#include <lib/gwenviewlib_export.h>

// Qt

// KDE
#include <kcompositejob.h>

// Local
#include <lib/document/document.h>

namespace Gwenview {


class DocumentJobPrivate;

/**
 * Represent an asynchronous task to be executed on a document
 * The task must be created on the heap and passed to an instance of Document
 * via Document::enqueueJob()
 *
 * The task behavior must be implemented in run()
 *
 * Tasks are always started from the GUI thread, and are never parallelized.
 * You can of course use threading inside your task implementation to speed it
 * up.
 */
class GWENVIEWLIB_EXPORT DocumentJob : public KCompositeJob {
	Q_OBJECT
public:
	DocumentJob();
	~DocumentJob();

	Document::Ptr document() const;

	virtual void start();

protected Q_SLOTS:
	/**
	 * Implement this method to provide the task behavior.
	 * You *must* emit the done() signal when your work is finished, but it
	 * does not have to be finished when run() returns.
	 * If you are not emitting it from the GUI thread, then use a queued
	 * connection to emit it.
	 */
	virtual void doStart() = 0;

	/**
	 * slot-ification of emitResult()
	 */
	void emitResult() {
		KJob::emitResult();
	}

private:
	void setDocument(const Document::Ptr&);

	DocumentJobPrivate* const d;

	friend class Document;
};


} // namespace

#endif /* DOCUMENTJOB_H */