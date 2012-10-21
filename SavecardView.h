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

#ifndef DEF_SAVECARDVIEW
#define DEF_SAVECARDVIEW

#include <QtGui>
#include "SavecardData.h"
#include "SavecardWidget.h"

class SavecardWidget;

class SavecardView : public QWidget
{
	Q_OBJECT

public:
	SavecardView(SavecardWidget *parent=0);
	virtual ~SavecardView();

	void clear();

	SavecardData *savecard() const;
	void setSavecard(SavecardData *save);

	void moveCursor(int saveID);

	QRect saveRect(int saveID) const;
	inline QSize saveSize() const { return QSize(saveWidth(), saveHeight()); }
	inline int saveWidth() const { return 672; }
	inline int saveHeight() const {	return 106; }
	virtual QSize sizeHint() const;
	virtual QSize minimumSizeHint() const;
	// public function to draw a save preview
	void renderSave(QPainter *painter, SaveData *saveData);
	void renderSave(QPixmap *pixmap, SaveData *saveData);
	static void drawFrame(QPainter *painter, int width, int height);
	static void num2pix(QPainter *painter, QImage *numberPixmap, int x, int y, quint32 num, quint8 space=1, QChar fill=QChar(' '), int color=0);
public slots:
	void properties(int saveID=-1);
private slots:
	void notifyFileChanged(const QString &path);
	void edit(int saveID=-1);
	void exportPC(int saveID=-1);
	void newGame(int saveID=-1);
	void removeSave(int saveID=-1);
//	void refreshIcon();
	void emitDropped();
signals:
	void changed();
	void released(SaveData *saveData);
private:
	void moveDraggedSave(int saveID);
	void replaceSaveData(int saveID, const QByteArray &mimeData);
	void setDropIndicator(int saveID);
	int saveID(const QPoint &pos) const;
	void restore(int saveID);
	void renderSave(QPainter *painter, SaveData *saveData, const QPixmap &menuBg, const QPixmap &fontPixmap);
	static void colors(QImage *image, int color);

	int cursorID, blackID, dropIndicatorID;
	bool isExternalDrag;
	int _dragStart;
	bool notify;
	SavecardData *_data;
	SavecardWidget *_parent;
//	SaveIcon *saveIcon;
	int mouseMove;
	QPoint startPos;
	QByteArray *lastDropData;
	int lastDropID;
protected:
	virtual void paintEvent(QPaintEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void contextMenuEvent(QContextMenuEvent *event);
	virtual void changeEvent(QEvent *event);
	virtual void dragEnterEvent(QDragEnterEvent *event);
	virtual void dragMoveEvent(QDragMoveEvent *event);
	virtual void dragLeaveEvent(QDragLeaveEvent *event);
	virtual void dropEvent(QDropEvent *event);
};

#endif // DEF_SAVECARDVIEW