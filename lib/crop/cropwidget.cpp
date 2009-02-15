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
// Self
#include "cropwidget.moc"

// Qt
#include <QPushButton>

// KDE
#include "klocale.h"

// Local
#include "croptool.h"
#include "imageview.h"
#include "ui_cropwidget.h"

namespace Gwenview {


struct CropWidgetPrivate : public Ui_CropWidget {
	Document::Ptr mDocument;
	CropTool* mCropTool;
	bool mUpdatingFromCropTool;


	double cropRatio() const {
		int width = ratioWidthSpinBox->value();
		int height = ratioHeightSpinBox->value();
		return height / double(width);
	}


	void addRatioToComboBox(const QSize& size, const QString& _label = QString()) {
		QString label = _label;
		if (label.isEmpty()) {
			label = QString("%1 x %2").arg(size.width()).arg(size.height());
		}
		ratioComboBox->addItem(label, QVariant(size));
	}


	void addSeparatorToComboBox() {
		ratioComboBox->insertSeparator(ratioComboBox->count());
	}


	void initRatioComboBox() {
		QList<QSize> ratioList;
		ratioList
			<< QSize(3, 2)
			<< QSize(4, 3)
			<< QSize(5, 4)
			<< QSize(6, 4)
			<< QSize(7, 5)
			<< QSize(10, 8);

		addRatioToComboBox(QSize(1, 1), i18n("Square"));
		addSeparatorToComboBox();

		Q_FOREACH(const QSize& size, ratioList) {
			addRatioToComboBox(size);
		}
		addSeparatorToComboBox();
		Q_FOREACH(QSize size, ratioList) {
			size.transpose();
			addRatioToComboBox(size);
		}

		ratioComboBox->setMaxVisibleItems(ratioComboBox->count());
	}


	void initSpinBoxes() {
		QSize size = mDocument->size();
		leftSpinBox->setMaximum(size.width());
		widthSpinBox->setMaximum(size.width());
		topSpinBox->setMaximum(size.height());
		heightSpinBox->setMaximum(size.height());
	}
};


CropWidget::CropWidget(QWidget* parent, ImageView* imageView, CropTool* cropTool)
: QWidget(parent)
, d(new CropWidgetPrivate) {
	d->mDocument = imageView->document();
	d->mUpdatingFromCropTool = false;
	d->mCropTool = cropTool;
	d->setupUi(this);
	layout()->setMargin(0);

	connect(d->advancedCheckBox, SIGNAL(toggled(bool)),
		SLOT(slotToggleAdvancedWidget(bool)));
	d->advancedWidget->setVisible(false);

	d->initRatioComboBox();

	connect(d->mCropTool, SIGNAL(rectUpdated(const QRect&)),
		SLOT(setCropRect(const QRect&)) );

	connect(d->leftSpinBox, SIGNAL(valueChanged(int)),
		SLOT(slotPositionChanged()) );
	connect(d->topSpinBox, SIGNAL(valueChanged(int)),
		SLOT(slotPositionChanged()) );
	connect(d->widthSpinBox, SIGNAL(valueChanged(int)),
		SLOT(slotWidthChanged()) );
	connect(d->heightSpinBox, SIGNAL(valueChanged(int)),
		SLOT(slotHeightChanged()) );

	connect(d->cropButton, SIGNAL(clicked()),
		SIGNAL(cropRequested()) );
	
	connect(d->constrainRatioCheckBox, SIGNAL(toggled(bool)),
		SLOT(applyRatioConstraint()) );
	connect(d->ratioWidthSpinBox, SIGNAL(valueChanged(int)),
		SLOT(applyRatioConstraint()) );
	connect(d->ratioHeightSpinBox, SIGNAL(valueChanged(int)),
		SLOT(applyRatioConstraint()) );

	connect(d->ratioComboBox, SIGNAL(activated(int)),
		SLOT(setRatioConstraintFromComboBox()) );

	// Don't do this before signals are connected, otherwise the tool won't get
	// initialized
	d->initSpinBoxes();
}


CropWidget::~CropWidget() {
	delete d;
}


QRect CropWidget::cropRect() const {
	QRect rect(
		d->leftSpinBox->value(),
		d->topSpinBox->value(),
		d->widthSpinBox->value(),
		d->heightSpinBox->value()
		);
	return rect;
}


void CropWidget::setCropRect(const QRect& rect) {
	d->mUpdatingFromCropTool = true;
	d->leftSpinBox->setValue(rect.left());
	d->topSpinBox->setValue(rect.top());
	d->widthSpinBox->setValue(rect.width());
	d->heightSpinBox->setValue(rect.height());
	d->mUpdatingFromCropTool = false;
}


void CropWidget::slotPositionChanged() {
	const QSize size = d->mDocument->size();
	d->widthSpinBox->setMaximum(size.width() - d->leftSpinBox->value());
	d->heightSpinBox->setMaximum(size.height() - d->topSpinBox->value());

	if (d->mUpdatingFromCropTool) {
		return;
	}
	d->mCropTool->setRect(cropRect());
}


void CropWidget::slotWidthChanged() {
	d->leftSpinBox->setMaximum(d->mDocument->width() - d->widthSpinBox->value());

	if (d->mUpdatingFromCropTool) {
		return;
	}
	if (d->constrainRatioCheckBox->isChecked()) {
		int height = int(d->widthSpinBox->value() * d->cropRatio());
		d->heightSpinBox->setValue(height);
	}
	d->mCropTool->setRect(cropRect());
}


void CropWidget::slotHeightChanged() {
	d->topSpinBox->setMaximum(d->mDocument->height() - d->heightSpinBox->value());

	if (d->mUpdatingFromCropTool) {
		return;
	}
	if (d->constrainRatioCheckBox->isChecked()) {
		int width = int(d->heightSpinBox->value() / d->cropRatio());
		d->widthSpinBox->setValue(width);
	}
	d->mCropTool->setRect(cropRect());
}


void CropWidget::applyRatioConstraint() {
	if (!d->constrainRatioCheckBox->isChecked()) {
		d->mCropTool->setCropRatio(0);
		return;
	}

	double ratio = d->cropRatio();
	d->mCropTool->setCropRatio(ratio);

	QRect rect = cropRect();
	rect.setHeight(int(rect.width() * d->cropRatio()));
	d->mCropTool->setRect(rect);
}


void CropWidget::setRatioConstraintFromComboBox() {
	QVariant data = d->ratioComboBox->itemData(d->ratioComboBox->currentIndex());
	if (!data.isValid()) {
		return;
	}

	QSize size = data.toSize();
	d->ratioWidthSpinBox->blockSignals(true);
	d->ratioWidthSpinBox->setValue(size.width());
	d->ratioWidthSpinBox->blockSignals(false);
	d->ratioHeightSpinBox->setValue(size.height());
}


void CropWidget::slotToggleAdvancedWidget(bool visible) {
	const QSize delta = parentWidget()->size() - size();
	d->advancedWidget->setVisible(visible);
	parentWidget()->resize(minimumSizeHint() + delta);
}


} // namespace
