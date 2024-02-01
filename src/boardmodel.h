#pragma once

#include <QAbstractListModel>
#include <QSize>

class BoardModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int side READ side WRITE setSide NOTIFY sideChanged)
    Q_PROPERTY(QSize boxSize READ boxSize WRITE setBoxSize NOTIFY boxSizeChanged)

public:
    enum DataRole
    {
        WidthRole = Qt::UserRole,
        HeightRole = Qt::UserRole + 1,
        CellsRole = Qt::UserRole + 2,
        ColumnRole = Qt::UserRole + 3,
        RowRole = Qt::UserRole + 4
    };

public:
    BoardModel(QObject *parent = nullptr);

    int side() const { return m_side; }
    void setSide(int side);

    QSize boxSize() const { return m_boxSize; }
    void setBoxSize(QSize boxSize);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;

signals:
    void sideChanged(int side);
    void boxSizeChanged(QSize boxSize);

public slots:
    void fill();
    void select(int x, int y);

private:
    bool fillCell(int index);
    void eliminate();
    void update();

private:
    int m_side;
    QSize m_boxSize;
    QVector<int> m_cells;
    int m_boxRows;
    int m_boxColumns;
    int m_boxCount;
    QVector<int> m_filled;
    QVector<int> m_selection;

};
