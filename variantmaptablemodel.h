#ifndef VARIANTMAPTABLEMODEL_H
#define VARIANTMAPTABLEMODEL_H

#include <QAbstractTableModel>

class AbstractColumnRole
{
public:
    AbstractColumnRole(QString name);
    virtual ~AbstractColumnRole() = default;
    QString name() { return _name; }
    virtual QVariant colData(const QVariantMap &rowData, int role = Qt::DisplayRole) = 0;
private:
    QString _name;
};

using AbstractColumn = AbstractColumnRole;
using AbstractRole = AbstractColumnRole;

class SimpleColumn : public AbstractColumnRole
{
public:
    SimpleColumn(QString name);

    QVariant colData(const QVariantMap &rowData, int role) override;
};

class FullnameColumn : public AbstractColumnRole
{
public:
    FullnameColumn(QString name);

    QVariant colData(const QVariantMap &rowData, int role) override;
};

class VariantMapTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    static const QString MODULE_NAME;   // "VariantMapTable"
    static const bool IS_QML_REG;

    VariantMapTableModel(QObject *parent = nullptr);
    void registerColumn(AbstractColumn *column);
    void registerRole(AbstractRole *role);
    void addRow(QVariantMap rowData);

    int idByRow(int row) const;
    int colByName(QString name) const;
    QString nameByCol(int col) const;
    bool getWithHeading() const;
    void setWithHeading(bool value);
    bool getForListViewFormat() const;
    void setForListViewFormat(bool forListViewFormat);
    int calcRow(const QModelIndex &index) const;
private:
    QList<int> _rowIndex;
    QHash<int, QVariantMap> _dataHash;
    QList<AbstractColumn*> _columns;
    QList<AbstractRole*> _roles;
    mutable QHash<int, QByteArray> _rolesId;
    bool _withHeading = false;
    bool _forListViewFormat = false;

public:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;
};

#endif // VARIANTMAPTABLEMODEL_H
