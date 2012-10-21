/****************************************************************************
 ** Hyne Final Fantasy VIII Save Editor
 ** Copyright (C) 2009-2012 Arzel J�r�me <myst6re@gmail.com>
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#include "SavecardView.h"
#include "Parameters.h"
#include "HeaderDialog.h"

SavecardView::SavecardView(SavecardWidget *parent) :
	QWidget(parent), cursorID(-1), blackID(-1),
	dropIndicatorID(-1), isExternalDrag(false),
	_dragStart(-1), notify(true), _data(0), _parent(parent),
	mouseMove(0), lastDropData(0)
{
	setPalette(QPalette(Qt::black));
//	saveIcon = new SaveIcon(saveData->saveIcon());
//	connect(saveIcon, SIGNAL(nextIcon(QPixmap)), SLOT(refreshIcon()));
	setMouseTracking(true);
	setAcceptDrops(true);
}

SavecardView::~SavecardView()
{
	if(lastDropData)	delete lastDropData;
}

void SavecardView::clear()
{
	if(_data) {
		_data = 0;
		update();
	}
}

SavecardData *SavecardView::savecard() const
{
	return _data;
}

void SavecardView::setSavecard(SavecardData *save)
{
	_data = save;

	if(_data) {
		setFixedSize(sizeHint());
		connect(_data->watcher(), SIGNAL(fileChanged(QString)), SLOT(notifyFileChanged(QString)));
	}
}

void SavecardView::notifyFileChanged(const QString &path)
{
	if(!notify)	return;
	notify = false;
	if(!QFile::exists(path))
	{
		QMessageBox::warning(this->parentWidget(), tr("Fichier supprim�"),
							 tr("Le fichier '%1' a �t� supprim� par un programme externe !").arg(path));
	}
	else
	{
		QMessageBox::warning(this->parentWidget(), tr("Fichier modifi�"),
							 tr("Le fichier '%1' a �t� modifi� par un programme externe.").arg(path));
	}
	notify = true;
}

void SavecardView::moveCursor(int saveID)
{
	cursorID = saveID;
	update();
}

void SavecardView::moveDraggedSave(int saveID)
{
	if(!_data)	return;
	if(_dragStart < saveID) {
		saveID--;
	}

	if(_dragStart != saveID
			&& _dragStart >= 0 && _dragStart < _data->getSaves().size()) {
		QList<SaveData *> saves = _data->getSaves();
		SaveData *saveData = saves.takeAt(_dragStart);
		saves.insert(saveID, saveData);

		saveID = 0;
		foreach(saveData, saves) {
			saveData->setId(saveID);
			saveID++;
		}

		_data->setSaves(saves);

		update();

		emit changed();
	}

	_dragStart = -1;
}

void SavecardView::replaceSaveData(int saveID, const QByteArray &mimeData)
{
	if(!_data)	return;
	if(saveID >=0 && saveID < _data->getSaves().size()
			&& !mimeData.isEmpty()) {
		SaveData *saveData = _data->getSave(saveID);

		if(!saveData->isDelete()) {
			QMessageBox::StandardButton answer = QMessageBox::question(this, tr("�craser"), tr("Tout le contenu de la sauvegarde sera �cras�.\nContinuer ?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
			if(answer != QMessageBox::Yes) {
				return;
			}
		}

		if(mimeData.size() > SAVE_SIZE) {
			saveData->open(mimeData.left(SAVE_SIZE), mimeData.mid(SAVE_SIZE));
		} else if(mimeData.size() == SAVE_SIZE) {
			QByteArray MCHeader;
			if(saveData->hasMCHeader()) {
				HeaderDialog dialog(saveData, this, HeaderDialog::CreateView);

				if(dialog.exec() != QDialog::Accepted) {
					return;
				}
				MCHeader = saveData->MCHeader();
			}
			saveData->open(mimeData, MCHeader);
		} else {
			return;
		}

		_data->setModified(true);
		saveData->setModified(true);
		emit changed();
	}
}

void SavecardView::setDropIndicator(int saveID)
{
	dropIndicatorID = saveID;
	update();
}

void SavecardView::edit(int saveID)
{
	if(!_data)	return;

	if(saveID == -1) {
		saveID = this->saveID(startPos);
	}

	SaveData *saveData = _data->getSave(saveID);
	if(!saveData)	return;

	if(saveData->isFF8() && !saveData->isDelete())
		emit released(saveData);
}

void SavecardView::exportPC(int saveID)
{
	if(!_data)	return;

	if(saveID == -1) {
		saveID = this->saveID(startPos);
	}

	SaveData *saveData = _data->getSave(saveID);
	if(!saveData)	return;

	QString path;
	int index;

	if(Config::value("savePath").isEmpty())
	{
		index = _data->path().lastIndexOf('/');
		if(index != -1)		path = _data->path().left(index+1);
	}
	else
	{
		path = Config::value("savePath") % "/";
	}

	path = QFileDialog::getSaveFileName(this, tr("Exporter"), path % QString("save%1").arg(saveData->id()+1, 2, 10, QChar('0')), tr("FF8 PC save (*)"));
	if(path.isEmpty())		return;

	index = path.lastIndexOf('/');
	Config::value("savePath") = index == -1 ? path : path.left(index);

	if(!saveData->exportPC(path))
		QMessageBox::warning(this, tr("�chec"), tr("Enregistrement �chou�, v�rifiez que le fichier cible n'est pas utilis�."));
}

void SavecardView::newGame(int saveID)
{
	if(!_data)	return;

	if(saveID == -1) {
		saveID = this->saveID(startPos);
	}

	SaveData *saveData = _data->getSave(saveID);
	if(!saveData)	return;

	if(!saveData->isDelete()) {
		QMessageBox::StandardButton b = QMessageBox::question(this, tr("Nouvelle partie"), tr("Tout le contenu de la sauvegarde sera remplac� par une nouvelle partie.\nContinuer ?"), QMessageBox::Yes | QMessageBox::No);
		if(b != QMessageBox::Yes) {
			return;
		}
	}

	QFile newGameFile(":/data/newGame");
	if(!newGameFile.open(QIODevice::ReadOnly)) {
		qWarning() << "failed to open data/newGame" << newGameFile.errorString();
		return;
	}

	if(saveData->hasMCHeader() && (!saveData->isFF8() || saveData->isDelete())) {
		HeaderDialog dialog(saveData, this, HeaderDialog::RestoreView);
		if(dialog.exec()==QDialog::Accepted)
		{
			saveData->restore();
		}
		else
		{
			return;
		}
	}

	saveData->open(newGameFile.readAll(), saveData->MCHeader());
	saveData->setModified(true);
	emit changed();
	newGameFile.close();

	update();
}

void SavecardView::removeSave(int saveID)
{
	if(!_data)	return;

	if(saveID == -1) {
		saveID = this->saveID(startPos);
	}

	SaveData *saveData = _data->getSave(saveID);
	if(!saveData)	return;

	if(!saveData->isDelete()) {
		QMessageBox::StandardButton b = QMessageBox::question(this, tr("Vider"), tr("Tout le contenu de la sauvegarde sera vid�.\nContinuer ?"), QMessageBox::Yes | QMessageBox::No);
		if(b != QMessageBox::Yes) {
			return;
		}
	}

	saveData->remove();
	emit changed();

	update();
}

void SavecardView::properties(int saveID)
{
	if(!_data)	return;

	if(saveID == -1) {
		saveID = this->saveID(startPos);
	}

	SaveData *saveData = _data->getSave(saveID);
	if(!saveData)	return;

	HeaderDialog dialog(saveData, this);

	if(dialog.exec() == QDialog::Accepted
			&& saveData->isModified()) {
		emit changed();
		if(!saveData->isFF8() && !saveData->isDelete()) {
//			saveIcon->setData(saveData->saveIcon());//TODO
			update(); // update icon
		}
	}
}

void SavecardView::restore(int saveID)
{
	if(!_data)	return;
	SaveData *saveData = _data->getSave(saveID);
	if(!saveData)	return;

	int reponse = QMessageBox::question(this, tr("Sauvegarde supprim�e"), tr("Cette sauvegarde a �t� supprim�e, voulez-vous tenter de la r�parer ? (succ�s non garanti)"), tr("Oui"), tr("Non"));
	if(reponse != 0)  return;

	HeaderDialog dialog(saveData, this, HeaderDialog::RestoreView);

	if(dialog.exec() == QDialog::Accepted)
	{
		saveData->restore();
		emit changed();
	}
}

int SavecardView::saveID(const QPoint &pos) const
{
	if(pos.x() < 0
			|| pos.x() >= width()
			|| pos.y() < 0
			|| pos.y() >= height())
		return -1;

	return pos.y() / saveHeight();
}

QRect SavecardView::saveRect(int saveID) const
{
	QSize size = saveSize();
	return QRect(QPoint(36, saveID * size.height()), size);
}

QSize SavecardView::sizeHint() const
{
	if(!_data)	return QSize();
	return QSize(36 + saveWidth() + 36, saveHeight() * _data->getSaves().size());
}

QSize SavecardView::minimumSizeHint() const
{
	return sizeHint();
}

/*void SavecardView::refreshIcon()
{
	if(!saveData->isDelete()) {
		repaint(width()/2 - 372 + 36 + 36, 43, 16, 16);
	}
}*/

void SavecardView::renderSave(QPainter *painter, SaveData *saveData)
{
	QPixmap menuBg(QString(":/images/menu-fond%1.png").arg(!saveData->isTheLastEdited() && !saveData->isDelete() ? "" : "2"));
	QPixmap menuTitle(":/images/numbers_title.png");

	renderSave(painter, saveData, menuBg, menuTitle);
}

void SavecardView::renderSave(QPixmap *pixmap, SaveData *saveData)
{
	QPainter p(pixmap);
	renderSave(&p, saveData);
}

void SavecardView::renderSave(QPainter *painter, SaveData *saveData, const QPixmap &menuBg, const QPixmap &fontPixmap)
{
	painter->save();

	painter->setBrush(QBrush(menuBg));
	drawFrame(painter, saveWidth(), saveHeight());

	QString title = QString("%1").arg(saveData->id()+1, 2, 10, QChar('0'));
	painter->drawPixmap(4, 0, fontPixmap.copy(title.at(0).digitValue()*16, 0, 16, 22));
	painter->drawPixmap(20, 0, fontPixmap.copy(title.at(1).digitValue()*16, 0, 16, 22));

	if(saveData->isFF8())
	{
		if(saveData->descData().party[0] != 255)
			painter->drawPixmap(44, 4, QPixmap(QString(":/images/icons/perso%1.png").arg(saveData->descData().party[0] & 15)));
		if(saveData->descData().party[1] != 255)
			painter->drawPixmap(112, 4, QPixmap(QString(":/images/icons/perso%1.png").arg(saveData->descData().party[1] & 15)));
		if(saveData->descData().party[2] != 255)
			painter->drawPixmap(180, 4, QPixmap(QString(":/images/icons/perso%1.png").arg(saveData->descData().party[2] & 15)));

		int persoIndex = saveData->descData().party[0] != 255 ? saveData->descData().party[0] : (saveData->descData().party[1] != 255 ? saveData->descData().party[1] : saveData->descData().party[2]);
		bool langIndep = persoIndex==SQUALL || persoIndex==RINOA || persoIndex==GRIEVER || persoIndex==BOKO || persoIndex==ANGELO;
		FF8Text::drawTextArea(painter, QPoint(271, 8), saveData->perso(persoIndex), langIndep ? (saveData->isJp() ? 2 : 1) : 0);
		FF8Text::drawTextArea(painter, QPoint(271, 36), tr("NV%1").arg(saveData->descData().nivLeader,3,10,QChar(' ')), 1);

		QImage numberPixmap(":/images/numbers.png");

		QPixmap disc(QString(":/images/disc_%1.png").arg(Config::value("lang")=="fr" ? "fr" : "en"));
		painter->drawPixmap(391, 38, disc);
		num2pix(painter, &numberPixmap, 395+disc.width(), 38, saveData->descData().disc+1);
		painter->drawPixmap(511, 16, QPixmap(QString(":/images/play_%1.png").arg(Config::value("lang")=="fr" ? "fr" : "en")));

		int hour = Config::hour(saveData->descData().time, saveData->freqValue());
		int color = (hour>=100) + (hour>=200) + (hour>=300) + (hour>=400) + (hour>=500);
		num2pix(painter, &numberPixmap, 576, 16, hour, 2, QChar(' '), color);

		QImage deux_points(":/images/deux-points.png");
		colors(&deux_points, color);
		painter->drawImage(612, 18, deux_points);
		num2pix(painter, &numberPixmap, 624, 16, Config::min(saveData->descData().time, saveData->freqValue()), 2, QChar('0'), color);
		painter->drawPixmap(640, 44, QPixmap(":/images/gils.png"));
		num2pix(painter, &numberPixmap, 511, 40, saveData->descData().gils, 8);

		painter->translate(256, 62);
		painter->setBrush(QBrush());
		drawFrame(painter, 416, 44);

		FF8Text::drawTextArea(painter, QPoint(12, 12), saveData->descData().locationID<251 ? Data::locations().at(saveData->descData().locationID) : QString("??? (%1)").arg(saveData->descData().locationID));
	}
	else
	{
		if(saveData->isDelete())
		{
			// Available block
			FF8Text::drawTextArea(painter, QPoint(36, 43), tr("Bloc Disponible"));
		}
		else
		{
			// Icon + description
//			painter->drawPixmap(36, 43, saveIcon->pixmap());
			painter->drawPixmap(36, 43, saveData->saveIcon().icon());
			QString short_desc = saveData->getShortDescription();
			if(!short_desc.isEmpty())
			{
				painter->setPen(Qt::white);
				painter->drawText(68, 59, short_desc);
			}
		}
	}

	painter->restore();

	// Save modification state
	if(saveData->isModified() || saveData->wasModified()) {
		QPen pen(saveData->isModified() ? QColor(0xd1,0x1d,0x1d) : QColor(0x1d,0xd1,0x1d), 3);
		painter->setPen(pen);
		painter->drawLine(670, 3, 670, 103);
	}
}

void SavecardView::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	painter.fillRect(event->rect(), palette().color(QPalette::Window));

	if(_data) {
		painter.translate(36, 0);

		int curSaveID = 0, minSaveID, maxSaveID;
		minSaveID = saveID(event->rect().topLeft());
		maxSaveID = saveID(event->rect().bottomLeft());

		QPixmap menuBg(":/images/menu-fond.png");
		QPixmap menuBg2(":/images/menu-fond2.png");
		QPixmap menuTitle(":/images/numbers_title.png");

		foreach(SaveData *saveData, _data->getSaves()) {
			if(blackID != curSaveID &&
					curSaveID >= minSaveID && curSaveID <= maxSaveID) {
				renderSave(&painter, saveData, !saveData->isTheLastEdited() && !saveData->isDelete() ? menuBg : menuBg2, menuTitle);

				if(cursorID == curSaveID) {
					painter.drawPixmap(-36, 16, QPixmap(":/images/cursor.png"));
				}
			}
			if(dropIndicatorID == curSaveID) {
				if(isExternalDrag) {
					QPen pen(Qt::white, 3);
					painter.setPen(pen);
					painter.setBrush(QBrush());
					painter.drawRect(0, 2, saveWidth(), saveHeight()-4);
				} else {
					QPen pen(Qt::white, 8);
					painter.setPen(pen);
					painter.drawLine(0, 0, saveWidth(), 0);
				}
			}

			painter.translate(0, saveHeight());
			curSaveID++;
		}
	}
}

void SavecardView::drawFrame(QPainter *painter, int width, int height)
{
	QPen pen(QColor(41,41,41));
	pen.setWidth(2);
	pen.setJoinStyle(Qt::MiterJoin);
	painter->setPen(pen);
	painter->drawRect(1, 1, width-2, height-2);

	painter->setPen(QColor(132,132,132));
	painter->drawLine(4, 2, width-5, 2);
	painter->drawLine(4, 3, width-5, 3);
	painter->drawLine(2, 4, 2, height-5);
	painter->drawLine(3, 4, 3, height-5);

	painter->setPen(QColor(58,58,58));
	painter->drawLine(4, height-4, width-3, height-4);
	painter->drawLine(4, height-3, width-3, height-3);
	painter->drawLine(width-4, 4, width-4, height-5);
	painter->drawLine(width-3, 4, width-3, height-5);

	painter->setPen(QColor(99,99,99));
	painter->drawLine(2, 2, 3, 2);
	painter->drawLine(2, 3, 3, 3);

	painter->setPen(QColor(82,82,82));
	painter->drawLine(width-4, 2, width-3, 2);
	painter->drawLine(width-4, 3, width-3, 3);
	painter->drawLine(2, height-4, 3, height-4);
	painter->drawLine(2, height-3, 3, height-3);
}

void SavecardView::num2pix(QPainter *painter, QImage *numberPixmap, int x, int y, quint32 num, quint8 space, QChar fill, int color)//Dessine les nombres pour l'affichage des gils et du temps
{
	colors(numberPixmap, color);
	QString strNum = QString("%1").arg(num, space, 10, fill).right(space);

	for(quint8 i=0 ; i<space ; ++i)
	{
		QChar c = strNum.at(i);
		if(c.isDigit())
			painter->drawImage(x+16*i, y, numberPixmap->copy(14*c.digitValue(), 0, 14, 16));
	}
}

void SavecardView::colors(QImage *image, int color)
{
	switch(color) {
	case 1://Red
		image->setColorTable(QVector<QRgb>() << qRgb(0xff,0x18,0x18) << qRgb(0x20,0x20,0x20) << qRgb(0x8b,0x18,0x18) << 0);
		return;
	case 2://Blue
		image->setColorTable(QVector<QRgb>() << qRgb(0x6a,0xb4,0xee) << qRgb(0x29,0x29,0x29) << qRgb(0x52,0x73,0x94) << 0);
		return;
	case 3://Yellow
		image->setColorTable(QVector<QRgb>() << qRgb(0xde,0xde,0x08) << qRgb(0x29,0x29,0x29) << qRgb(0x83,0x83,0x18) << 0);
		return;
	case 4://Green
		image->setColorTable(QVector<QRgb>() << qRgb(0x00,0xff,0x00) << qRgb(0x39,0x39,0x31) << qRgb(0x00,0xbd,0x00) << 0);
		return;
	case 5://Purple
		image->setColorTable(QVector<QRgb>() << qRgb(0xff,0x00,0xff) << qRgb(0x29,0x29,0x29) << qRgb(0x94,0x10,0x94) << 0);
		return;
	default:
		image->setColorTable(QVector<QRgb>() << qRgb(0xee,0xee,0xee) << qRgb(0x41,0x41,0x31) << qRgb(0xa4,0xa4,0xa4) << 0);
		return;
	}
}

void SavecardView::mousePressEvent(QMouseEvent *event)
{
	int xStart = 36;
	int xEnd = width()-xStart;

	if(event->x() < xStart || event->x() > xEnd) {
		return;
	}

	if(event->button() == Qt::LeftButton) {
		mouseMove = 1;
		startPos = event->pos();
	}
}

void SavecardView::mouseMoveEvent(QMouseEvent *event)
{
	// Change cursor position
	int cursorID = saveID(event->pos());
	if(this->cursorID != cursorID) {
		this->cursorID = cursorID;
		update();
	}

	if(!mouseMove || !(event->buttons() & Qt::LeftButton)
			|| (startPos - event->pos()).manhattanLength() < QApplication::startDragDistance())
		return;

	mouseMove = 2;

	SaveData *saveData = _data->getSave(saveID(startPos));
	if(!saveData)	return;

	QDrag *drag = new QDrag(this);
	QMimeData *mimeData = new QMimeData;

	_dragStart = saveData->id();
	mimeData->setData("application/ff8save", saveData->save() + saveData->MCHeader());
	drag->setMimeData(mimeData);
	drag->setHotSpot(QPoint(event->pos().x() - 36, event->pos().y() % saveHeight()));
	QPixmap pixmap(saveSize());
	renderSave(&pixmap, saveData);
	drag->setPixmap(pixmap);

	blackID = saveData->id();
	update();
	drag->exec(Qt::MoveAction, Qt::MoveAction);// !Qt event loop blocked on Windows!
	blackID = -1;
	update();
}

void SavecardView::mouseReleaseEvent(QMouseEvent *event)
{
	if(blackID != -1) {
		blackID = -1;
		update();
	}
	if(mouseMove == 2) {
		mouseMove = 0;
		return;
	} else {
		mouseMove = 0;
	}

	int xStart = 36;
	int xEnd = width()-xStart;

	if(event->x() < xStart || event->x() > xEnd) {
		return;
	}

	SaveData *saveData = _data->getSave(saveID(event->pos()));
	if(!saveData)	return;

	if(event->button() == Qt::LeftButton)
	{
		if(saveData->isFF8()) {
			if(saveData->isDelete())
				restore(saveData->id());
			else
				edit(saveData->id());
		}
		else
		{
			if(saveData->isDelete()) {
				contextMenuEvent(new QContextMenuEvent(QContextMenuEvent::Other, event->pos(), event->globalPos(), event->modifiers()));
				return;
			}
			properties(saveData->id());
		}
	}
	else if(event->button() == Qt::MidButton)
	{
		if(saveData->isDelete()) {
			contextMenuEvent(new QContextMenuEvent(QContextMenuEvent::Other, event->pos(), event->globalPos(), event->modifiers()));
			return;
		}
		properties(saveData->id());
	}
}

void SavecardView::contextMenuEvent(QContextMenuEvent *event)
{
	int xStart = 36;
	int xEnd = width()-xStart;

	if(event->x() < xStart || event->x() > xEnd) {
		return;
	}

	SaveData *saveData = _data->getSave(saveID(event->pos()));
	if(!saveData)	return;

	startPos = event->pos();

	QMenu menu(this);
	if(!saveData->isDelete() && saveData->isFF8()) {
		menu.setDefaultAction(menu.addAction(tr("&Modifier..."), this, SLOT(edit())));
		menu.addAction(tr("&Exporter en sauv. PC..."), this, SLOT(exportPC()));
	}
	menu.addAction(tr("&Nouvelle partie"), this, SLOT(newGame()));
	if(!saveData->isDelete()) {
		menu.addAction(tr("&Vider"), this, SLOT(removeSave()));
		QAction *action = menu.addAction(tr("&Propri�t�s..."), this, SLOT(properties()));
		if(!saveData->isFF8()) {
			menu.setDefaultAction(action);
		}

	}
	menu.exec(event->globalPos());
}

void SavecardView::changeEvent(QEvent *event)
{
	if(event->type() == QEvent::LanguageChange) {
		update();
		event->accept();
	} else {
		QWidget::changeEvent(event);
	}
}

void SavecardView::dragEnterEvent(QDragEnterEvent *event)
{
	if(event->mimeData()->hasFormat("application/ff8save")) {
		isExternalDrag = event->source() == 0; // external source
		event->acceptProposedAction();
	}
}

void SavecardView::dragMoveEvent(QDragMoveEvent *event)
{
	int saveID = this->saveID(event->pos());
	if(saveID >= 0) {
		setDropIndicator(saveID + int(event->pos().y() % saveHeight() > saveHeight()/2));
		_parent->scrollToDrag(event->pos());
	}
}

void SavecardView::dragLeaveEvent(QDragLeaveEvent *)
{
	if(dropIndicatorID != -1) {
		dropIndicatorID = -1;
		update();
	}
}

void SavecardView::dropEvent(QDropEvent *event)
{
	lastDropID = dropIndicatorID;
	dragLeaveEvent(0);

	event->acceptProposedAction();
	isExternalDrag = event->source() == 0;
	if(isExternalDrag) {
		if(lastDropData)	delete lastDropData;
		lastDropData = new QByteArray(event->mimeData()->data("application/ff8save"));
	}
	QTimer::singleShot(0, this, SLOT(emitDropped()));// defer function call
}

void SavecardView::emitDropped()
{
	if(isExternalDrag) {
		replaceSaveData(lastDropID, *lastDropData);
		delete lastDropData;
		lastDropData = 0;
	} else {
		moveDraggedSave(lastDropID);
	}
}