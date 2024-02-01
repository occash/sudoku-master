#include "boardmodel.h"
#include "board.h"

#include <QDebug>
#include <random>
#include <set>

namespace
{
    std::random_device device;
    std::mt19937 generator(device());
}

BoardModel::BoardModel(QObject *parent) :
    QAbstractListModel{ parent },
    m_side{ 9 },
    m_boxSize{ 3, 3 }
{
    update();
}

void BoardModel::setSide(int side)
{
    if (m_side == side)
        return;

    m_side = side;
    emit sideChanged(m_side);

    update();
}

void BoardModel::setBoxSize(QSize boxSize)
{
    if (m_boxSize == boxSize)
        return;

    m_boxSize = boxSize;
    emit boxSizeChanged(m_boxSize);

    update();
}

QHash<int, QByteArray> BoardModel::roleNames() const
{
    return{
        { WidthRole, "width" },
        { HeightRole, "height" },
        { CellsRole, "cells" },
        { ColumnRole, "column" },
        { RowRole, "row" }
    };
}

int BoardModel::rowCount(const QModelIndex &) const
{
    return m_boxCount;
}

QVariant BoardModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return{};

    if (index.row() >= m_boxCount || index.row() < 0)
        return{};

    const int id = index.row();

    switch (role)
    {
    case WidthRole:
        return m_boxSize.width();
    case HeightRole:
        return m_boxSize.height();
    case CellsRole:
    {
        const int by = id / m_boxColumns;
        const int bx = id % m_boxColumns;
        const int xb = bx * m_boxSize.width();
        const int yb = by * m_boxSize.height();

        QVariantList result;

        for (int y = 0; y < 3; ++y) {
            for (int x = 0; x < 3; ++x) {
                const int gx = bx * m_boxRows + x;
                const int gy = by * m_boxColumns + y;
                const int cellIndex = gx + gy * m_side;
                QVariantMap value;
                value["value"] = m_cells[cellIndex];
                value["column"] = x;
                value["row"] = y;
                value["x"] = xb + x;
                value["y"] = yb + y;
                value["selection"] = m_selection[cellIndex];
                value["filled"] = m_filled[cellIndex];
                result.append(value);
            }
        }

        return result;
    }
    case ColumnRole:
        return id % m_boxColumns;
    case RowRole:
        return id / m_boxColumns;
    }

    return{};
}

void BoardModel::fill()
{
    beginResetModel();

    const int cellCount = m_side * m_side;
    m_selection.fill(-1, cellCount);
    m_cells.fill(-1, cellCount);

    if (!fillCell(0))
        qDebug() << "Failed to generate board";

    for (int iy = 0; iy < m_side; ++iy) {
        auto log = qDebug();

        for (int ix = 0; ix < m_side; ++ix) {
            const int i = ix + iy * m_side;
            log << m_cells[i] + 1;
        }
    }

    m_filled.fill(0, cellCount);
    eliminate();

    endResetModel();
}

void BoardModel::select(int x, int y)
{
    beginResetModel();

    const int count = m_side * m_side;
    const int index = x + y * m_side;
    const int value = m_cells[index];

    m_selection.fill(-1, m_side * m_side);

    for (int iy = 0; iy < m_side; ++iy) {
        for (int ix = 0; ix < m_side; ++ix) {
            const int i = ix + iy * m_side;

            if (m_cells[i] == value)
                m_selection[i] = 0;
            else if (ix == x || iy == y)
                m_selection[i] = 1;
        }
    }

    endResetModel();

    /*auto topLeft = createIndex(0, 0);
    auto bottomRight = createIndex(m_boxRows - 1, m_boxColumns - 1);

    emit dataChanged(topLeft, bottomRight);*/
}

bool BoardModel::fillCell(int index)
{
    const int y = index / m_side;
    const int x = index % m_side;
    std::set<int> values;

    for (int ix = 0; ix < x; ++ix) {
        const int index = ix + y * m_side;
        values.insert(m_cells.at(index));
    }

    for (int iy = 0; iy < y; ++iy) {
        const int index = x + iy * m_side;
        values.insert(m_cells.at(index));
    }

    const int xb = (x / m_boxSize.width()) * m_boxSize.width();
    const int xe = xb + m_boxSize.width();
    const int yb = (y / m_boxSize.height()) * m_boxSize.height();
    const int ye = yb + m_boxSize.height();

    for (int iy = yb; iy < ye; ++iy) {
        for (int ix = xb; ix < xe; ++ix) {
            if (iy == y && ix == x)
                goto filter;

            const int index = ix + iy * m_side;
            values.insert(m_cells.at(index));
        }
    }

filter:

    std::set<int> valid;

    for (int i = 0; i < m_side; ++i)
        valid.insert(i);

    std::vector<int> options;
    std::set_difference(
        valid.begin(), valid.end(),
        values.begin(), values.end(),
        std::inserter(options, options.begin())
    );
    std::shuffle(options.begin(), options.end(), generator);

    int &value = m_cells[index];

    for (const auto &option : options) {
        value = option;

        if (index == m_cells.size() - 1 || fillCell(index + 1))
            return true;
    }

    value = -1;
    return false;
}

void BoardModel::eliminate()
{
    // Step 1. Find the longest chain
    struct Coord
    {
        int x;
        int y;

        bool operator==(const Coord &o) const
        {
            return x == o.x && y == o.y;
        }

        bool operator<(const Coord &o) const
        {
            return x < o.x || (x == o.x && y < o.y);
        }
    };

    using Chain = std::set<Coord>;
    using Set = std::set<Coord>;
    using CandidateSet = std::map<Coord, std::set<int>>;

    auto findInBoard = [this](int value, const std::set<Coord> &exclude) -> Coord {
        for (int y = 0; y < m_side; ++y) {
            for (int x = 0; x < m_side; ++x) {
                const Coord c{ x, y };

                if (exclude.find(c) != exclude.end())
                    continue;

                const int index = x + y * m_side;

                if (m_cells[index] == value)
                    return{ x, y };
            }
        }

        return{ 0, 0 };
    };

    auto findInRow = [this](int y, int value) -> int {
        for (int x = 0; x < m_side; ++x) {
            const int index = x + y * m_side;

            if (m_cells[index] == value)
                return x;
        }

        return 0;
    };

    auto findInColumn = [this](int x, int value) -> int {
        for (int y = 0; y < m_side; ++y) {
            const int index = x + y * m_side;

            if (m_cells[index] == value)
                return y;
        }

        return 0;
    };

    size_t longest{ 0 };
    int elem1{ -1 };
    int elem2{ -1 };
    Set S11;
    Set S12;

    for (int i = 0; i < m_side; ++i) {
        for (int j = i + 1; j < m_side; ++j) {
            size_t count{ 0 };
            Set exclude;
            std::vector<Chain> chains;
            bool current{ false };

            qDebug() << "Entries:" << i << j;

            do {
                const Coord c0 = findInBoard(i, exclude);
                const int entries[]{ i, j };
                enum Axis { XAxis, YAxis } axis = YAxis;
                Coord c = c0;
                size_t length{ 0 };
                Chain chain{ c };

                exclude.insert(c);

                while (true) {
                    const int entry = entries[axis];
                    const Coord c1 = c;

                    if (axis == XAxis) {
                        c.x = findInRow(c.y, entry);
                        length += abs(c.x - c1.x);
                    } else {
                        c.y = findInColumn(c.x, entry);
                        length += abs(c.y - c1.y);
                    }

                    ++count;
                    chain.insert(c);
                    exclude.insert(c);
                    axis = (Axis)!axis;

                    if (c == c0)
                        break;
                }

                chains.push_back(chain);

                if (length > longest) {
                    longest = length;
                    elem1 = i;
                    elem2 = j;
                    current = true;
                }

                qDebug() << "Chain:" << length;
            } while (count < m_side * 2);

            if (current) {
                S11.clear();
                S12.clear();

                for (const auto &chain : chains) {
                    auto iter = chain.begin();
                    S11.insert(*iter);
                    S12.insert(++iter, chain.end());
                }
            }
        }
    }

    qDebug() << "The longest chain:" << elem1 << elem2 << longest;

    // Strategies
    auto crme = [&](const Set &S11, const Coord &coord) -> std::set<int> {
        std::set<int> candidates;
        
        for (int i = 0; i < m_side; ++i)
            candidates.insert(i);

        for (int x = 0; x < m_side; ++x) {
            if (x == coord.x)
                continue;

            auto i = S11.find({ x, coord.y });

            if (i != S11.end()) {
                const int index = (*i).x + (*i).y * m_side;
                const int value = m_cells[index];
                candidates.erase(value);
            }
        }

        for (int y = 0; y < m_side; ++y) {
            if (y == coord.y)
                continue;

            auto i = S11.find({ coord.x, y });

            if (i != S11.end()) {
                const int index = (*i).x + (*i).y * m_side;
                const int value = m_cells[index];
                candidates.erase(value);
            }
        }

        const int xb = (coord.x / m_boxSize.width()) * m_boxSize.width();
        const int xe = xb + m_boxSize.width();
        const int yb = (coord.y / m_boxSize.height()) * m_boxSize.height();
        const int ye = yb + m_boxSize.height();

        for (int y = yb; y < ye; ++y) {
            for (int x = xb; x < xe; ++x) {
                if (x == coord.x && y == coord.y)
                    continue;

                auto i = S11.find({ x, y });

                if (i != S11.end()) {
                    const int index = (*i).x + (*i).y * m_side;
                    const int value = m_cells[index];
                    candidates.erase(value);
                }
            }
        }

        return candidates;
    };

    // Step 2. Apply greedy elimination algorithm
    do {
        std::vector<Coord> valid;

        for (int y = 0; y < m_side; ++y) {
            for (int x = 0; x < m_side; ++x) {
                const Coord coord{ x, y };

                if (S11.find(coord) != S11.end() ||
                    S12.find(coord) != S12.end())
                    continue;

                valid.push_back({ x, y });
            }
        }

        std::shuffle(valid.begin(), valid.end(), generator);

        for (int i = 0; i < m_side; ++i)
            S12.insert(valid[i]);

        Set St;

        while (!S12.empty()) {
            CandidateSet candidates;
            Set S11temp{ S11 };
            bool found{ false };

            // Find candidates
            do {
                found = false;

                // Apply Simple CRME first
                for (int y = 0; y < m_side; ++y) {
                    for (int x = 0; x < m_side; ++x) {
                        const Coord entry{ x, y };

                        if (S11temp.find(entry) != S11temp.end())
                            continue;

                        auto values = crme(S11temp, entry);

                        if (values.size() == 1) {
                            S11temp.insert(entry);
                            found = true;
                        }

                        candidates[entry] = values;
                    }
                }

                // Look for lone rangers
                for (int y = 0; y < m_side; ++y) {
                    std::map<int, Set> rangers;

                    for (int x = 0; x < m_side; ++x) {
                        const Coord entry{ x, y };

                        if (S11temp.find(entry) != S11temp.end())
                            continue;

                        for (const auto &value : candidates[entry])
                            rangers[value].insert(entry);
                    }

                    for (const auto &ranger : rangers)
                        if (ranger.second.size() == 1) {
                            const Coord entry = *ranger.second.begin();
                            candidates[entry] = { ranger.first };
                            S11temp.insert(entry);
                            found = true;
                        }
                }

                for (int x = 0; x < m_side; ++x) {
                    std::map<int, Set> rangers;

                    for (int y = 0; y < m_side; ++y) {
                        const Coord entry{ x, y };

                        if (S11temp.find(entry) != S11temp.end())
                            continue;

                        for (const auto &value : candidates[entry])
                            rangers[value].insert(entry);
                    }

                    for (const auto &ranger : rangers)
                        if (ranger.second.size() == 1) {
                            const Coord entry = *ranger.second.begin();
                            candidates[entry] = { ranger.first };
                            S11temp.insert(entry);
                            found = true;
                        }
                }

                for (int by = 0; by < m_boxRows; ++by) {
                    for (int bx = 0; bx < m_boxColumns; ++bx) {
                        const int xb = bx * m_boxSize.width();
                        const int xe = xb + m_boxSize.width();
                        const int yb = by * m_boxSize.height();
                        const int ye = yb + m_boxSize.height();
                        std::map<int, Set> rangers;

                        for (int y = yb; y < ye; ++y) {
                            for (int x = xb; x < xe; ++x) {
                                const Coord entry{ x, y };

                                if (S11temp.find(entry) != S11temp.end())
                                    continue;

                                for (const auto &value : candidates[entry])
                                    rangers[value].insert(entry);
                            }
                        }

                        for (const auto &ranger : rangers)
                            if (ranger.second.size() == 1) {
                                const Coord entry = *ranger.second.begin();
                                candidates[entry] = { ranger.first };
                                S11temp.insert(entry);
                                found = true;
                            }
                    }
                }
            } while (found);

            for (const auto &entry : S11temp)
                candidates[entry] = { -1 };

            size_t most{ 0 };
            Coord c;

            for (const auto &entry : S12) {
                const auto candidate = candidates[entry];
                const size_t count = candidate.size();

                if (count == 1) {
                    c = entry;
                    most = 0;
                    break;
                }

                if (count > most) {
                    c = entry;
                    most = count;
                }
            }

            if (most > 0) {
                S11.insert(c);
                S12.erase(c);
            } else {
                St.insert(c);
                S12.erase(c);
            }
        }

        std::copy(St.begin(), St.end(), std::inserter(S12, S12.begin()));
    } while (S11.size() + S12.size() != m_side * m_side);

    for (const auto &c : S11) {
        const int index = c.x + c.y * m_side;
        m_filled[index] = 1;
    }
}

void BoardModel::update()
{
    beginResetModel();

    const int cellCount = m_side * m_side;
    m_boxRows = m_side / m_boxSize.height();
    m_boxColumns = m_side / m_boxSize.width();
    m_boxCount = m_boxRows * m_boxColumns;
    m_cells.fill(-1, cellCount);
    m_filled.fill(0, cellCount);
    m_selection.fill(-1, cellCount);

    endResetModel();
}
