#include "variantmapmodel.h"

#include "QQmlEngine"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

bool registerMe()
{
    qmlRegisterType<VariantMapModel>(VariantMapModel::MODULE_NAME.toUtf8(), 1, 0, "VariantMapModel");
    return true;
}

const QString VariantMapModel::MODULE_NAME = "VariantMap";

const bool VariantMapModel::IS_QML_REG = registerMe();


VariantMapModel::VariantMapModel(QObject *parent)
    : QAbstractTableModel (parent) { }

VariantMapModel::VariantMapModel(bool isList, bool autoId, bool withHeading, QObject *parent)
    : QAbstractTableModel (parent), _forListViewFormat(isList), _autoId(autoId),
      _withHeading(withHeading) { }

void VariantMapModel::registerColumn(AbstractColumn *column)
{
    // todo: проверки на повторяемость и тд
    _columns.append(column);
}

void VariantMapModel::registerRole(AbstractRole *role)
{
    // todo: можно избавиться от этой ф-ции, а добавлять все лишние роли при addRow
    // todo: проверки на повторяемость и тд
    _roles.append(role);
}

void VariantMapModel::addRow(QVariantMap rowData)
{
    int id = _autoId ? ++_idRow : rowData.value(_idStr).toInt();
    beginInsertRows(QModelIndex(), _rowIndex.count(), _rowIndex.count());
    _rowIndex.append(id);
    _dataHash.insert(id, rowData);
    endInsertRows();
}

QVariantMap VariantMapModel::getRowData(int row) const
{
    int id = idByRow(row);
    return _dataHash.value(id);
}

int VariantMapModel::idByRow(int row) const
{
    return _rowIndex.at(row);
}

int VariantMapModel::colByName(QString name) const
{
    qDebug() << __PRETTY_FUNCTION__ << "вроде не нужна";
    for (int col = 0; col < _columns.count(); ++col) {
        if (nameByCol(col) == name)
            return col;
    }
    return -1;
}

QString VariantMapModel::nameByCol(int col) const
{
    return _columns.at(col)->name();
}

bool VariantMapModel::getWithHeading() const
{
    return _withHeading;
}

void VariantMapModel::setWithHeading(bool value)
{
    _withHeading = value;
}

int VariantMapModel::calcRow(const QModelIndex &index) const
{
    return index.row() - _withHeading;
}

bool VariantMapModel::isHeadingRow(const QModelIndex &index) const
{
    return calcRow(index) < 0;
}

QJsonValue VariantMapModel::toJson() const
{
    QJsonArray jArr;
    for (int row = 0; row < _rowIndex.count(); ++row) {
        auto rowData = getRowData(row);
        QJsonObject jRow = QJsonObject::fromVariantMap(rowData);
        jArr.append(jRow);
    }
    return QJsonValue(jArr);
}

QCborValue VariantMapModel::toCbor() const
{
    return QCborValue::fromJsonValue(toJson());
}

QByteArray VariantMapModel::toByteArray(bool isJson) const
{
    if (isJson) {
        return QJsonDocument(toJson().toObject()).toJson();
    }
    return toCbor().toCbor();
}

void VariantMapModel::fromJson(QJsonValue jValue)
{
    QJsonArray jArr = jValue.toArray();
    for (const auto& jRowRef: jArr) {
        QVariantMap item = jRowRef.toObject().toVariantMap();
        addRow(item);
    }
}

void VariantMapModel::fromCbor(QCborValue cborValue)
{
    QJsonArray jArr = cborValue.toJsonValue().toArray();
    for (const auto& jRowRef: jArr) {
        QVariantMap item = jRowRef.toObject().toVariantMap();
        addRow(item);
    }
}

void VariantMapModel::fromByteArray(QByteArray buff, bool isJson)
{
    if (isJson) {
        fromJson(QJsonDocument::fromJson(buff).array());
    } else {
        fromCbor(QCborValue::fromCbor(buff));
    }
}

bool VariantMapModel::autoId() const
{
    return _autoId;
}

void VariantMapModel::setAutoId(bool autoId)
{
    _autoId = autoId;
}

QString VariantMapModel::getIdStr() const
{
    return _idStr;
}

void VariantMapModel::setIdStr(const QString &id)
{
    _idStr = id;
}

bool VariantMapModel::getForListViewFormat() const
{
    return _forListViewFormat;
}

void VariantMapModel::setForListViewFormat(bool forListViewFormat)
{
    _forListViewFormat = forListViewFormat;
}

int VariantMapModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return _rowIndex.count() + _withHeading;
}

int VariantMapModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return _columns.count();
}

QVariant VariantMapModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    if (role > Qt::UserRole && _forListViewFormat) {
        return data(this->index(calcRow(index), role - Qt::UserRole), Qt::DisplayRole);
    }
    if (isHeadingRow(index)) {
        if (role == Qt::DisplayRole) {
            return _columns.at(index.column())->name();
        } else {
            return QVariant();
        }
    }
    QVariantMap rowData = getRowData(calcRow(index));
    if (role == Qt::DisplayRole) {
        return _columns.at(index.column())->colData(rowData, role);
    } else {
        qDebug() << rowData[_rolesId[role]];
        return rowData[_rolesId[role]];
    }
}

bool VariantMapModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || isHeadingRow(index)) {
        return false;
    }
    if (role == Qt::EditRole) {
        int id = idByRow(calcRow(index));
        _dataHash[id].insert(nameByCol(index.column()), value);
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

Qt::ItemFlags VariantMapModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QHash<int, QByteArray> VariantMapModel::roleNames() const
{
    _rolesId = QAbstractTableModel::roleNames();
    for (int i = 0; i < _columns.size(); ++i) {
        _rolesId.insert(Qt::UserRole + i, _columns.at(i)->name().toUtf8());
    }
    for (int i = 0; i < _roles.size(); ++i) {
        _rolesId.insert(Qt::UserRole + _columns.size() + i, _roles.at(i)->name().toUtf8());
    }
    return _rolesId;
}

SimpleColumn::SimpleColumn(QString name) : AbstractColumnRole (name)
{

}

QVariant SimpleColumn::colData(const QVariantMap &rowData, int role)
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    return rowData.value(name());
}

AbstractColumnRole::AbstractColumnRole(QString name) : _name(name)
{

}

FullnameColumn::FullnameColumn(QString name) : AbstractColumnRole (name)
{

}

QVariant FullnameColumn::colData(const QVariantMap &rowData, int role)
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }
    return rowData.value("firstname").toString() + " " + rowData.value("lastname").toString();
}