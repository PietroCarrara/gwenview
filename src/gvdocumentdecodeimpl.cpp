// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�lien G�teau
 
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

// Qt
#include <qbuffer.h>
#include <qfile.h>
#include <qguardedptr.h>
#include <qimage.h>
#include <qmemarray.h>
#include <qstring.h>
#include <qtimer.h>
#include <qdatetime.h>

// KDE
#include <kdebug.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <ktempfile.h>
#include <kurl.h>

// Local
#include "gvdocumentloadedimpl.h"
#include "gvdocumentjpegloadedimpl.h"
#include "gvdocumentdecodeimpl.moc"
#include "gvcache.h"

const unsigned int DECODE_CHUNK_SIZE=4096;


//---------------------------------------------------------------------
//
// GVDecoderThread
//
//---------------------------------------------------------------------
void GVDecoderThread::run() {
	QMutexLocker locker(&mMutex);
	kdDebug() << k_funcinfo << endl;
	QImageIO imageIO;
	
	QBuffer buffer(mRawData);
	buffer.open(IO_ReadOnly);
	imageIO.setIODevice(&buffer);
	bool ok=imageIO.read();
		
	if (!ok) {
		kdDebug() << k_funcinfo << " failed" << endl;
		postSignal( SIGNAL(failed()) );
		return;
	}
	
	kdDebug() << k_funcinfo << " succeeded" << endl;
	mImage=imageIO.image();
	
	kdDebug() << k_funcinfo << " succeeded, emitting signal" << endl;
	postSignal( SIGNAL(succeeded()) );
}


void GVDecoderThread::setRawData(const QByteArray& data) {
	QMutexLocker locker(&mMutex);
	mRawData=data.copy();
}


QImage GVDecoderThread::popLoadedImage() {
	QMutexLocker locker(&mMutex);
	QImage img=mImage;
	mImage=QImage();
	return img;
}
	


//---------------------------------------------------------------------
//
// GVDocumentDecodeImplPrivate
//
//---------------------------------------------------------------------
class GVDocumentDecodeImplPrivate {
public:
	GVDocumentDecodeImplPrivate(GVDocumentDecodeImpl* impl)
	: mReadSize( 0 ), mDecoder(impl), mSuspended( false ) {}

	bool mUpdatedDuringLoad;
	QByteArray mRawData;
	unsigned int mReadSize;
	QImageDecoder mDecoder;
	QTimer mDecoderTimer;
	QRect mLoadChangedRect;
	QTime mTimeSinceLastUpdate;
	QGuardedPtr<KIO::Job> mJob;
	bool mSuspended;
	bool mUseThread;
	QDateTime mTimestamp;
	GVDecoderThread mDecoderThread;
};


//---------------------------------------------------------------------
//
// GVDocumentDecodeImpl
//
//---------------------------------------------------------------------
GVDocumentDecodeImpl::GVDocumentDecodeImpl(GVDocument* document) 
: GVDocumentImpl(document) {
	kdDebug() << k_funcinfo << endl;
	d=new GVDocumentDecodeImplPrivate(this);
	d->mUpdatedDuringLoad=false;

	connect(&d->mDecoderTimer, SIGNAL(timeout()), this, SLOT(decodeChunk()) );

	connect(&d->mDecoderThread, SIGNAL(succeeded()),
		this, SLOT(slotDecoderThreadSucceeded()) );
	connect(&d->mDecoderThread, SIGNAL(failed()),
		this, SLOT(slotDecoderThreadFailed()) );
	
	QTimer::singleShot(0, this, SLOT(start()) );
}


GVDocumentDecodeImpl::~GVDocumentDecodeImpl() {
	if (d->mDecoderThread.running()) {
		d->mDecoderThread.wait();
	}
	delete d;
}

void GVDocumentDecodeImpl::start() {
	d->mLoadChangedRect=QRect();
	d->mTimestamp = GVCache::instance()->timestamp( mDocument->url() );
	d->mJob=KIO::stat( mDocument->url(), false);
	connect(d->mJob, SIGNAL(result(KIO::Job*)),
		this, SLOT(slotStatResult(KIO::Job*)) );
}

void GVDocumentDecodeImpl::slotStatResult(KIO::Job*job) {
	// Get modification time of the original file
	KIO::UDSEntry entry = static_cast<KIO::StatJob*>(job)->statResult();
	KIO::UDSEntry::ConstIterator it= entry.begin();
	QDateTime urlTimestamp;
	for (; it!=entry.end(); it++) {
		if ((*it).m_uds == KIO::UDS_MODIFICATION_TIME) {
			urlTimestamp.setTime_t( (*it).m_long );
			break;
		}
	}
	QImage image;
	QCString format;
	if( urlTimestamp <= d->mTimestamp ) {
		image = GVCache::instance()->image( mDocument->url(), format );
	}
	if( !image.isNull()) {
		setImageFormat(format);
		finish(image);
	} else {
		QByteArray data = GVCache::instance()->file( mDocument->url() );
		if( !data.isNull()) {
			d->mJob = NULL;
			d->mRawData = data;
			d->mReadSize=0;
			d->mUseThread = false;
			d->mTimeSinceLastUpdate.start();
			d->mDecoderTimer.start(0, false);
		} else {
			d->mTimestamp = urlTimestamp;
			startLoading();
		}
	}
}

void GVDocumentDecodeImpl::startLoading() {
	d->mJob=KIO::get( mDocument->url(), false, false);

	connect(d->mJob, SIGNAL(data(KIO::Job*, const QByteArray&)),
		this, SLOT(slotDataReceived(KIO::Job*, const QByteArray&)) );
	
	connect(d->mJob, SIGNAL(result(KIO::Job*)),
		this, SLOT(slotResult(KIO::Job*)) );

	d->mRawData.resize(0);
	d->mReadSize=0;
	d->mUseThread = false;
	d->mTimeSinceLastUpdate.start();
}
    

void GVDocumentDecodeImpl::slotResult(KIO::Job* job) {
	kdDebug() << k_funcinfo << " loading finished:" << job->error() << endl;
	if( job->error() != 0 ) {
		// failed
		emit finished(false);
		switchToImpl(new GVDocumentImpl(mDocument));
	}
}


void GVDocumentDecodeImpl::slotDataReceived(KIO::Job*, const QByteArray& chunk) {
	kdDebug() << k_funcinfo << " size:" << chunk.size() << endl;
	if (chunk.size()<=0) return;

	int oldSize=d->mRawData.size();
	d->mRawData.resize(oldSize + chunk.size());
	memcpy(d->mRawData.data()+oldSize, chunk.data(), chunk.size() );

	// Decode the received data
	if( !d->mDecoderTimer.isActive()) d->mDecoderTimer.start(0);
}


void GVDocumentDecodeImpl::decodeChunk() {
	if( d->mSuspended ) {
		d->mDecoderTimer.stop();
		return;
	}

	if (!d->mUseThread) {
		int chunkSize = QMIN(DECODE_CHUNK_SIZE, int(d->mRawData.size())-d->mReadSize);
		if (chunkSize> 0) {
			int decodedSize = d->mDecoder.decode(
				(const uchar*)(d->mRawData.data()+d->mReadSize),
				chunkSize);
			kdDebug() << k_funcinfo << "decodedSize:" << decodedSize << endl;

			if (decodedSize>0) {
				// There's more to come
				d->mReadSize+=decodedSize;
				return;
			}

			if (decodedSize<0) {
				// We can't use async decoding, switch to decoder thread 
				d->mUseThread=true;
				return;
			}
		}
	} else {
		// We are in threaded decoding mode, et's wait until the get job has finished
		if (!d->mJob.isNull()) return;
	}

	// If we reach this part, we have the full image in d->mRawData and it has
	// either been fully decoded, or async decoding failed
	d->mDecoderTimer.stop();
	
	// Set image format
	QBuffer buffer(d->mRawData);
	buffer.open(IO_ReadOnly);
	const char* format = QImageIO::imageFormat(&buffer);
	buffer.close();

	if (!format) {
		// Unknown format, no need to go further
		emit finished(false);
		switchToImpl(new GVDocumentImpl(mDocument));
	}
	setImageFormat( format );

	// If async decoding failed, start threaded decoding
	if( d->mUseThread ) {
		kdDebug() << k_funcinfo << "starting threaded decoding\n";
		d->mDecoderThread.setRawData(d->mRawData);
		d->mDecoderThread.start();
		return;
	}
	
	// If we reach this part, async decoding has succeeded
	kdDebug() << k_funcinfo << "async decoding succeeded\n";

	GVCache::instance()->addFile( mDocument->url(), d->mRawData, d->mTimestamp );

	QImage image=d->mDecoder.image();
	finish(image);
}



void GVDocumentDecodeImpl::slotDecoderThreadFailed() {
	kdDebug() << k_funcinfo << endl;
	// Image can't be loaded, let's switch to an empty implementation
	emit finished(false);
	switchToImpl(new GVDocumentImpl(mDocument));
}


void GVDocumentDecodeImpl::slotDecoderThreadSucceeded() {
	kdDebug() << k_funcinfo << endl;
	GVCache::instance()->addFile( mDocument->url(), d->mRawData, d->mTimestamp );

	QImage image=d->mDecoderThread.popLoadedImage();
	finish( image);
}



void GVDocumentDecodeImpl::finish( QImage& image) {
	kdDebug() << k_funcinfo << endl;
	
	// Convert depth if necessary
	// (32 bit depth is necessary for alpha-blending)
	if (image.depth()<32 && image.hasAlphaBuffer()) {
		image=image.convertDepth(32);
		d->mUpdatedDuringLoad=false;
	}

	GVCache::instance()->addImage( mDocument->url(), image, mDocument->imageFormat(), d->mTimestamp );

	// The decoder did not cause the sizeUpdated or rectUpdated signals to be
	// emitted, let's do it now
	if (!d->mUpdatedDuringLoad) {
		setImage(image);
		emit sizeUpdated(image.width(), image.height());
		emit rectUpdated( QRect(QPoint(0,0), image.size()) );
	}
	
	// Now we switch to a loaded implementation
	if (qstrcmp(mDocument->imageFormat(), "JPEG")==0) {
		// We want a local copy of the file for the comment editor
		QString tempFilePath;
		if (!mDocument->url().isLocalFile()) {
			KTempFile tempFile;
			tempFile.dataStream()->writeRawBytes(d->mRawData.data(), d->mRawData.size());
			tempFile.close();
			tempFilePath=tempFile.name();
		}
		switchToImpl(new GVDocumentJPEGLoadedImpl(mDocument, d->mRawData, tempFilePath));
	} else {
		switchToImpl(new GVDocumentLoadedImpl(mDocument));
	}
}


void GVDocumentDecodeImpl::suspendLoading() {
	d->mDecoderTimer.stop();
	d->mSuspended = true;
}

void GVDocumentDecodeImpl::resumeLoading() {
	d->mSuspended = false;
	if(d->mReadSize < d->mRawData.size()) {
		d->mDecoderTimer.start(0, false);
	}
}


//---------------------------------------------------------------------
//
// QImageConsumer
//
//---------------------------------------------------------------------
void GVDocumentDecodeImpl::end() {
	if( !d->mLoadChangedRect.isNull()) {
		emit rectUpdated(d->mLoadChangedRect);
	}
	kdDebug() << k_funcinfo << endl;
}

void GVDocumentDecodeImpl::changed(const QRect& rect) {
//	kdDebug() << k_funcinfo << " " << rect.left() << "-" << rect.top() << " " << rect.width() << "x" << rect.height() << endl;
	if (!d->mUpdatedDuringLoad) {
		setImage(d->mDecoder.image());
		d->mUpdatedDuringLoad=true;
	}
	d->mLoadChangedRect |= rect;
	if( d->mTimeSinceLastUpdate.elapsed() > 100 ) {
		kdDebug() << k_funcinfo << " " << d->mLoadChangedRect.left() << "-" << d->mLoadChangedRect.top()
			<< " " << d->mLoadChangedRect.width() << "x" << d->mLoadChangedRect.height() << "\n";
		emit rectUpdated(d->mLoadChangedRect);
		d->mLoadChangedRect = QRect();
		d->mTimeSinceLastUpdate.start();
	}
}

void GVDocumentDecodeImpl::frameDone() {
	kdDebug() << k_funcinfo << endl;
}

void GVDocumentDecodeImpl::frameDone(const QPoint& /*offset*/, const QRect& /*rect*/) {
	kdDebug() << k_funcinfo << endl;
}

void GVDocumentDecodeImpl::setLooping(int) {
	kdDebug() << k_funcinfo << endl;
}

void GVDocumentDecodeImpl::setFramePeriod(int /*milliseconds*/) {
	kdDebug() << k_funcinfo << endl;
}

void GVDocumentDecodeImpl::setSize(int width, int height) {
	kdDebug() << k_funcinfo << " " << width << "x" << height << endl;
	// FIXME: There must be a better way than creating an empty image
	setImage(QImage(width, height, 32));
	emit sizeUpdated(width, height);
}

