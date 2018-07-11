﻿//InfoCardTableModel.cpp by Emercoin developers
#include "pch.h"
#include "InfoCardTableModel.h"
#include "ShellImitation.h"
#include "Settings.h"
#include "OpenSslExecutable.h"
#include "InfoCardHighlighter.h"

InfoCardTableModel::InfoCardTableModel(QObject*parent): QAbstractTableModel(parent) {
	reload();
}
InfoCardTableModel::~InfoCardTableModel() {
	qDeleteAll(_rows);
}
QString InfoCardTableModel::Item::logFilePath()const {
	return pathByExt("log");
}
InfoCardTableModel::Item* InfoCardTableModel::itemBy(int row)const {
	if(row<0 || row>=_rows.count())
		return 0;
	return _rows[row];
}
QString InfoCardTableModel::Item::loadFromFile(const QFileInfo & entry) {//QString::isEmpty -> ok
	_file = entry.filePath();
	_baseName = entry.baseName();
	_dir = entry.dir();
	QFile file(_file);
	if(!file.open(QFile::ReadOnly))
		return file.errorString();
	const QByteArray arr = file.readAll();
	_text = arr;
	parse(_text);
	{
		/*QTextDocument doc;
		InfoCardHighlighter hi(&doc);
		hi._key.setFontWeight(QFont::Bold);
		hi._comment.setFontItalic(true);
		doc.setPlainText(_text);
		_html = doc.toHtml();*/
		_html = _text;
	}
	return QString();
}
using Shell = ShellImitation;
QString InfoCardTableModel::Item::pathByExt(const QString & extension)const {
	return _dir.absoluteFilePath(_baseName + '.' + extension);
}
void InfoCardTableModel::Item::add(const QString & key, const QString & value, bool replace) {
	InfoCard::add(key, value, replace);
	if(key=="Import")
		return;
	if(!_displayedText.isEmpty())
		_displayedText += "; ";
	_displayedText += key % ": " % value;
}
QString InfoCardTableModel::Item::removeFiles() {
	for(auto ext: QString("info|infoz|log").split('|')) {
		QString path = pathByExt(ext);
		if(QFile::exists(path)) {
			if(!QFile::remove(path)) {
				return QObject::tr("Can't remove %1").arg(path);
			}
		}
	}
	return QString();
}
void InfoCardTableModel::removeRows(const QModelIndexList & rows) {
	if(rows.isEmpty())
		return;
	int row = rows[0].row();
	if(row < 0 || row >= _rows.count()) {
		Q_ASSERT(0);
		return;
	}
	Item& r = *_rows[row];
	QString error = r.removeFiles();
	if(!error.isEmpty()) {
		reload();
		QMessageBox::critical(0, tr("Error"), tr("Error removing files: %1").arg(error));
		return;
	}
	beginRemoveRows(QModelIndex(), row, row);
	_rows.removeAt(row);
	endRemoveRows();
	delete &r;
}
void InfoCardTableModel::reload() {
	beginResetModel();
	_rows.clear();
	QDir dir = Settings::certDir();
	const QFileInfoList list = dir.entryInfoList(QStringList() << "*.info", QDir::Files, QDir::Name);
	for(const QFileInfo & entry : list) {
		QScopedPointer<Item> item(new Item);
		const QString code = item->loadFromFile(entry);
		if(code.isEmpty()) {
			_rows << item.take();
		} else {
			QMessageBox::critical(0, tr("Can't load template file"), code);
		}
	}
	endResetModel();
}
int InfoCardTableModel::rowCount(const QModelIndex& index)const {
	return _rows.count();
}
int InfoCardTableModel::columnCount(const QModelIndex& index)const {
	return ColEnd;
}
QVariant InfoCardTableModel::headerData(int section, Qt::Orientation orientation, int role)const {
	if(orientation == Qt::Vertical) {
		if(role == Qt::DisplayRole || role == Qt::ToolTipRole)
			return section + 1;
	} else {
		if(role == Qt::DisplayRole || role == Qt::ToolTipRole) {
			static_assert(ColEnd == 1, "update switch");
			switch(section) {
				case ColText: return tr("Values");
			}
		}
	}
	return QVariant();
}
Qt::ItemFlags InfoCardTableModel::flags(const QModelIndex &index) const {

}
QVariant InfoCardTableModel::data(const QModelIndex &index, int role) const {
	int row = index.row();
	const Item * item = itemBy(row);
	if(!item)
		return {};
	if(role == Qt::DisplayRole || role == Qt::ToolTipRole) {
		static_assert(ColEnd == 1, "update switch");
		switch(index.column()) {
			case ColText:
				if(role == Qt::ToolTipRole)
					return item->_html;
				return item->_displayedText;
			//case ColDateTime: return item._InfoCardId;
		}
	}
	return QVariant();
}
int InfoCardTableModel::indexByFile(const QString & s)const {
	for(int i = 0; i < _rows.count(); ++i) {
		if(_rows[i]->_file==s)
			return i;
	}
	return -1;
}