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
// Self
#include "importer.moc"

// Qt

// KDE
#include <kdatetime.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <klocale.h>
#include <ktempdir.h>
#include <kurl.h>
#include <kio/copyjob.h>
#include <kio/job.h>
#include <kio/jobuidelegate.h>
#include <kio/netaccess.h>
#include <kstandarddirs.h>

// stdc++
#include <memory>

// Local
#include <fileutils.h>
#include <filenameformater.h>
#include <lib/timeutils.h>

namespace Gwenview {

struct ImporterPrivate {
	Importer* q;
	KUrl::List mUrlList;
	KUrl mCurrentUrl;
	KUrl::List mImportedUrlList;
	QWidget* mAuthWindow;
	std::auto_ptr<FileNameFormater> mFileNameFormater;
	std::auto_ptr<KTempDir> mDestImportDir;
	QMap<KUrl, FileUtils::RenameResult> mRenameWarnings;
	int mProgress;
	int mJobProgress;

	void emitError(const QString& message) {
		QMetaObject::invokeMethod(q, "error", Q_ARG(QString, message));
	}

	bool createImportDir(const KUrl url) {
		Q_ASSERT(url.isLocalFile());
		// FIXME: Support remote urls

		if (!KStandardDirs::makeDir(url.toLocalFile())) {
			emitError(i18n("Could not create destination folder."));
			return false;
		}
		mDestImportDir.reset(new KTempDir(url.toLocalFile() + "/.gwenview_importer-"));
		mDestImportDir->setAutoRemove(false);
		if (mDestImportDir->status() != 0) {
			const QString message = QString::fromLocal8Bit(::strerror(mDestImportDir->status()));
			emitError(i18n("Could not create temporary upload folder:\n%1", message));
			return false;
		}
		return true;
	}

	void importNext() {
		if (mUrlList.empty()) {
			q->finalizeImport();
			return;
		}
		mCurrentUrl = mUrlList.takeFirst();
		KUrl dst = KUrl(mDestImportDir->name());
		dst.addPath(mCurrentUrl.fileName());
		KIO::Job* job = KIO::copy(mCurrentUrl, dst, KIO::HideProgressInfo);
		if (job->ui()) {
			job->ui()->setWindow(mAuthWindow);
		}
		QObject::connect(job, SIGNAL(result(KJob*)),
			q, SLOT(slotResult(KJob*)));
		QObject::connect(job, SIGNAL(percent(KJob*, unsigned long)),
			q, SLOT(slotPercent(KJob*, unsigned long)));
	}

	void renameImportedUrl(const KUrl& src) {
		KUrl dst = src;
		dst.cd("..");
		QString fileName;
		if (mFileNameFormater.get()) {
			KFileItem item(KFileItem::Unknown, KFileItem::Unknown, src, true /* delayedMimeTypes */);
			KDateTime dateTime = TimeUtils::dateTimeForFileItem(item);
			fileName = mFileNameFormater->format(src, dateTime);
		} else {
			fileName = src.fileName();
		}
		dst.setFileName(fileName);

		FileUtils::RenameResult result = FileUtils::rename(src, dst, mAuthWindow);
		if (result != FileUtils::RenamedOK) {
			mRenameWarnings.insert(mCurrentUrl, result);
		}
		if (result != FileUtils::RenameFailed) {
			mImportedUrlList << mCurrentUrl;
		}
		q->advance();
		importNext();
	}
};


Importer::Importer(QWidget* parent)
: QObject(parent)
, d(new ImporterPrivate) {
	d->q = this;
	d->mAuthWindow = parent;
}

Importer::~Importer() {
	delete d;
}

void Importer::setAutoRenameFormat(const QString& format) {
	if (format.isEmpty()) {
		d->mFileNameFormater.reset(0);
	} else {
		d->mFileNameFormater.reset(new FileNameFormater(format));
	}
}

void Importer::start(const KUrl::List& list, const KUrl& destination) {
	d->mProgress = 0;
	d->mJobProgress = 0;
	d->mUrlList = list;
	d->mImportedUrlList.clear();
	d->mRenameWarnings.clear();

	emitProgressChanged();
	maximumChanged(d->mUrlList.count() * 100);

	if (!d->createImportDir(destination)) {
		kWarning() << "Could not create import dir";
		return;
	}
	d->importNext();
}

void Importer::slotResult(KJob* _job) {
	KIO::CopyJob* job = static_cast<KIO::CopyJob*>(_job);
	KUrl url = job->destUrl();
	if (job->error()) {
		kWarning() << "FIXME: What do we do with failed urls?";
		advance();
		d->importNext();
		return;
	}

	d->renameImportedUrl(url);
}

void Importer::finalizeImport() {
	if (!d->mRenameWarnings.isEmpty()) {
		kWarning() << "FIXME: Handle rename failures:" << d->mRenameWarnings;
	} else {
		d->mDestImportDir->unlink();
	}
	importFinished();
}

void Importer::advance() {
	++d->mProgress;
	d->mJobProgress = 0;
	emitProgressChanged();
}

void Importer::slotPercent(KJob*, unsigned long percent) {
	d->mJobProgress = percent;
	emitProgressChanged();
}

void Importer::emitProgressChanged() {
	progressChanged(d->mProgress * 100 + d->mJobProgress);
}

KUrl::List Importer::importedUrlList() const {
	return d->mImportedUrlList;
}

} // namespace